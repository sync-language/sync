#pragma once
#ifndef SY_INTERPRETER_STACK_NODE_HPP_
#define SY_INTERPRETER_STACK_NODE_HPP_

#include "../../core/core.h"
#include "../../mem/allocator.hpp"
#include "../../types/option/option.hpp"
#include "../../types/result/result.hpp"
#include <cstddef>

namespace sy {
struct Bytecode;
class Type;

class Frame {
  public:
    uint32_t basePointerOffset = 0;
    uint16_t frameLength = 0;
    uint16_t functionIndex = 0;
    void* retValueDst = nullptr;

    Frame() = default;

    /// The amount of slots the old stack frame info needs to store itself within the bounds of the
    /// new frame.
    static constexpr size_t OLD_FRAME_INFO_RESERVED_SLOTS = 1;
};

struct FrameInstructionPair {
    Frame frame;
    const Bytecode* instructionPointer;
};

/// An individual "slot" in the interpreter stack. It's the minimum amount of bytes any value can
/// occupy, including one byte values like 8 bit intgers or booleans. This should be freely case
/// back and forth to whatever data type you require using `reinterpret_cast<T>(value)`. Minimum
/// alignment for function execution is 16 bytes so might as well. Plus in the future makes it easy
/// to auto-vectorize stuff when JIT comes, and 128 bit integers / floats.
class alignas(16) StackValueSlot final {

  public:
  private: // dev note, layout is little endian, but probably not relevant.
    uint64_t low_;
    uint64_t high_;
};

/// An individual "slot" in the interpreter stack for runtime type information, mapping 1:1 to a
/// `StackValueSlot` range. The `Type` instance at this slot can be either `owned` or not. If it is
/// owned (`StackTypeSlot::isOwned() == true`), that means it should have it's destructor called
/// either at the end of a scope, or as part of unwinding. If not (`StackTypeSlot::isOwned() ==
/// false`), that means it shouldn't have the destructor called, and is likely being used as a
/// temporary location.
class alignas(16) StackTypeSlot final {
  public:
    StackTypeSlot() = default;

    StackTypeSlot(Type type, bool owned) noexcept;

    StackTypeSlot(std::nullptr_t) : baseMask_(0), indirection_(0), mutableBits_(0) {}

    /// @return The type at this slot with no ownership information.
    [[nodiscard]] Option<Type> get() const noexcept;

    /// @return `true` if the slot owns this type instance (should have the destructor called on the
    /// corresponding `StackValueSlot` range), otherwise `false`.
    [[nodiscard]] bool isOwned() const noexcept;

    void set(Type type, bool owned) noexcept;

  private:
    /// Instances of `sy::Type` are required to have alignment of `alignof(void*)`, therefore on
    /// all target platforms, the lowest bit will be zeroed, and thus can be used as a flag bit,
    /// conserving memory.
    static constexpr uintptr_t TYPE_OWNED_FLAG = 0b1;

    // matches `Type` layout.

    uintptr_t baseMask_ = 0;
    uint32_t indirection_ = 0;
    uint32_t mutableBits_ = 0;
};

class Node final {
  public:
    enum class PushFailure {
        CanReconstructBigger,
        TooBigAndHasFrames,
    };

    /// @param minSlotSize
    /// @param allocator Only used for the small stack. Otherwise just uses the OS page allocator if
    /// available, then back to `allocator` if not.
    /// @return
    [[nodiscard]] static Result<Node, AllocErr> init(uint32_t minSlotSize,
                                                     Allocator allocator) noexcept;

    ~Node() noexcept;

    Node(Node&& other) noexcept;

    Node& operator=(Node&& other) = delete;

    Node(const Node& other) = delete;

    Node& operator=(const Node& other) = delete;

    [[nodiscard]] bool hasFrames() const;

    /// Check if there is enough space to add this stack frame.
    /// @param frameLength The length of the frame, excluding the
    /// `Frame::OLD_FRAME_INFO_RESERVED_SLOTS` slots required to store the previous frame.
    [[nodiscard]] bool hasEnoughSpaceForFrame(uint16_t frameLength,
                                              uint16_t alignment) const noexcept;

    /// @param frameLength The length of the frame, excluding the
    /// `Frame::OLD_FRAME_INFO_RESERVED_SLOTS` slots required to store the previous frame.
    /// @return A result. On ok, returns no value. On error, returns a `bool`. If it is
    /// `true`, this `Node` has existing frames, thus cannot be deleted for a new `Node` with a
    /// larger allocation. If it is `false`, this `Node` is not in use, and thus is safe to delete
    /// this `Node` instance and create a new one with enough allocation.
    [[nodiscard]] Result<void, PushFailure>
    pushFrame(uint16_t frameLength, uint16_t requiredByteAlign, void* retValDst,
              Option<FrameInstructionPair> previous) noexcept;

    [[nodiscard]] Option<FrameInstructionPair> popFrame() noexcept;

    /// @return A result. On ok, returns the offset for where the next argument should go (assuming
    /// no alignment padding). On error, returns a `bool`. If it is `true`, this `Node` has existing
    /// frames, thus cannot be deleted for a new `Node` with a larger allocation. If it is `false`,
    /// this `Node` is not in use, and thus is safe to delete this `Node` instance and create a new
    /// one with enough allocation.
    [[nodiscard]] Result<uint16_t, PushFailure>
    pushScriptFunctionArg(void* argMem, Type type, uint16_t offset, uint16_t frameLength,
                          uint16_t frameByteAlign) noexcept;

    /// @return non-null pointer to the value at the specific offset within the current stack frame.
    template <typename T> [[nodiscard]] T* frameValueAt(uint16_t offset);

    /// @return non-null pointer to the value at the specific offset within the current stack frame.
    template <typename T> [[nodiscard]] const T* frameValueAt(uint16_t offset) const;

    /// @return The type within the current stack frame at `offset`. The underlying `const
    /// sy::Type*` may be nullptr.
    [[nodiscard]] StackTypeSlot typeAt(uint16_t offset) const;

    /// Sets the type at `offset` to `type`. If `type` is not a null type, will also set the
    /// following slots to nullptr provided that the type requires that many slots to store an
    /// object.
    void setTypeAt(StackTypeSlot type, uint16_t offset);

    void setFrameFunction(uint16_t functionIndex);

    [[nodiscard]] Option<Frame> frame() const noexcept { return this->currentFrame; }

    [[nodiscard]] uint32_t slotsCount() const noexcept { return this->slots; }

    /// By default, values use 1KB.
    /// On targets with 64 bit pointers, the types minimum allocation is 1KB. On targets with 32 bit
    /// pointers, such as wasm32, the types minimum allocation is 512B.
    static constexpr uint32_t MIN_SLOTS = 64;

  private:
    Node() = default;

    void ensureOffsetWithinFrameBounds(uint16_t offset) const;

    Allocator alloc;
    /// See `Node::MIN_SLOTS`. Is aligned to `ALLOC_CACHE_ALIGN` if `slots <= MIN_SLOTS`, or page
    /// alignment. `slots * sizeof(StackValueSlot)` is the total memory allocated for this.
    StackValueSlot* values = nullptr;
    /// See `Node::MIN_SLOTS`. Is aligned to `ALLOC_CACHE_ALIGN` if `slots <= MIN_SLOTS`, or page
    /// alignment. `slots * sizeof(StackValueSlot)` is the total memory allocated for this.
    StackTypeSlot* types = nullptr;
    /// Total amount of slots allocated for each of `values` and `types`.
    uint32_t slots = 0;
    /// Offset including the slots required to store frame info.
    uint32_t nextBaseOffset = Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
    /// If `currentFrame.has_value() == true`, then this node is currently in use,
    /// preventing certain operations such as `reallocate(...)`.
    Option<Frame> currentFrame{};
    /// How many frames this node has. If `frameDepth > 0`, then this node is in use,
    /// preventing certain operations such as `reallocate(...)`.
    uint16_t frameDepth = 0;
};

template <typename T> inline T* Node::frameValueAt(uint16_t offset) {
    ensureOffsetWithinFrameBounds(offset);
    const auto frame = this->currentFrame.value();
    const uint32_t actualOffset = frame.basePointerOffset + static_cast<uint32_t>(offset);
    return reinterpret_cast<T*>(&this->values[actualOffset]);
}

template <typename T> inline const T* Node::frameValueAt(uint16_t offset) const {
    ensureOffsetWithinFrameBounds(offset);
    const auto frame = this->currentFrame.value();
    const uint32_t actualOffset = frame.basePointerOffset + static_cast<uint32_t>(offset);
    return reinterpret_cast<const T*>(&this->values[actualOffset]);
}

} // namespace sy

#endif // SY_INTERPRETER_STACK_NODE_HPP_
