#pragma once
#ifndef SY_INTERPRETER_STACK_NODE_HPP_
#define SY_INTERPRETER_STACK_NODE_HPP_

#include "../../core.h"
#include "frame.hpp"
#include <cstddef>
#include <optional>

struct Bytecode;
namespace sy {
class Type;
}

class Node final {
  public:
    class TypeOfValue {
      public:
        TypeOfValue() = default;
        TypeOfValue(const sy::Type* type, bool owned);
        TypeOfValue(std::nullptr_t) : mask_(0){};
        TypeOfValue& operator=(std::nullptr_t);

        const sy::Type* get() const;
        operator const sy::Type *() const;

        void set(const sy::Type* type, bool owned);

        bool operator==(const TypeOfValue& other) const;
        bool operator==(const sy::Type* otherType) const;

        bool isOwned() const;

      private:
        /// Instances of `sy::Type` are required to have alignment of `alignof(void*)`, therefore on all target
        /// platforms, the lowest bit will be zeroed, and thus can be used as a flag bit, conserving memory.
        static constexpr uintptr_t TYPE_NOT_OWNED_FLAG = 0b1;

        uintptr_t mask_ = 0;
    };

    /// See `Node::MIN_SLOTS`. Is aligned to `Node::MIN_VALUES_ALIGNMENT` or page alignment.
    uint64_t* values = nullptr;
    /// See `Node::MIN_SLOTS`. Is aligned to `ALLOC_CACHE_ALIGN` or page alignment.
    TypeOfValue* types = nullptr;
    /// `slots * sizeof(uint64_t)` is the amount of bytes occupied by all of the memory allocated for
    /// each of `values` and `types`.
    uint32_t slots = 0;
    ///
    uint32_t nextBaseOffset = Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
    /// If `currentFrame.has_value() == true`, then this node is currently in use,
    /// preventing certain operations such as `reallocate(...)`.
    std::optional<Frame> currentFrame{};
    /// How many frames this node has. If `frameDepth > 0`, then this node is in use,
    /// preventing certain operations such as `reallocate(...)`.
    uint16_t frameDepth = 0;

    Node(const uint32_t minSlotSize);

    ~Node() noexcept;

    Node(Node&& other) noexcept;

    Node& operator=(Node&& other) = delete;

    Node(const Node& other) = delete;

    Node& operator=(const Node& other) = delete;

    [[nodiscard]] bool isInUse() const;

    /// @brief Forcibly reallocates this node to either grow or shrink it's allocation size
    /// @param minSlotSize Can be less than `this->slots`.
    void reallocate(const uint32_t minSlotSize);

    [[nodiscard]] bool hasEnoughSpaceForFrame(const uint32_t frameLength, const uint16_t alignment) const;

    /// @brief Attempts to push a new frame onto this node while the node is currently in use,
    /// meaning reallocation is not possible.
    /// @param frameLength The length of the frame
    /// @param byteAlign The alignment in bytes
    /// @param retValDst The return value destination. May be `nullptr`.
    /// @param currentFrame The current frame. If this node is in use, and `previousFrame.has_value() == true`,
    /// expects that `previousFrame.value()` is equivalent to `this->currentFrame.value()`.
    /// @param instructionPointer The instruction pointer that was being executed. May be `nullptr`. NOTE If
    /// `instructionPointer == nullptr`, then it is assumed that there is no previous frame.
    /// @return `true` the frame was successfully pushed onto this node.
    [[nodiscard]] bool pushFrameNoReallocate(const uint32_t frameLength, const uint16_t byteAlign,
                                             void* const retValDst, const Bytecode* const instructionPointer);

    /// @brief Pushes a frame onto this node from a previous node. Expects this node to not be in use,
    /// allowing reallocation.
    /// @param frameLength The length of the frame
    /// @param byteAlign The alignment in bytes
    /// @param retValDst The return value destination. May be `nullptr`.
    /// @param previousFrame The frame that was used previously. Should be from another node, or std::nullopt
    /// @param instructionPointer The instruction pointer that was being executed. May be `nullptr`. NOTE If
    /// `instructionPointer == nullptr`, then it is assumed that there is no previous frame, and when calling
    /// `Node::popFrame()`, will not return a frame and instruction pointer.
    void pushFrameAllowReallocate(const uint32_t frameLength, const uint16_t byteAlign, void* const retValDst,
                                  std::optional<Frame> previousFrame, const Bytecode* const instructionPointer);

    /// @brief Pops a frame from this node. If this node owned the previous frame, it's information
    /// is restored into this node, along with returning the frame data. If there was no previous frame,
    /// meaning the frame was the first frame on the entire call stack, then a null option is returned.
    /// @return An optional containing the previous frame data, regardless of whether this node owns
    /// it or not, along with the instruction pointer.
    [[nodiscard]] std::optional<std::tuple<Frame, const Bytecode*>> popFrame();

    /// @brief Attempts to push a script function argument onto this node.
    /// @param argMem Non-null pointer to the arguments memory to be byte-copied from.
    /// @param type Non-null pointer to the type of the argument.
    /// @param offset Memory offset to place the argument. If not properly aligned, will be shifted.
    /// @param frameLength Slot length of the frame for the future function call.
    /// @param frameByteAlign Byte alignment of the frame for the future function call.
    /// @return std::nullopt if cannot fit the argument and it's frame into this node. Otherwise
    /// returns the offset for where the next argument should go.
    [[nodiscard]] std::optional<uint16_t> pushScriptFunctionArg(const void* argMem, const sy::Type* type,
                                                                uint16_t offset, const uint32_t frameLength,
                                                                const uint16_t frameByteAlign);

    /// Checks if this node needs reallocation for the new frame length and alignment.
    /// If it does, returns a valid option with the new reallocation minimum size, that is guaranteed to fit
    /// the frame length at the specified alignment including with having to shift the base offset for alignment
    /// requirements.
    /// If it does not need reallocation, returns a null option.
    /// # Debug Asserts
    /// `this->currentFrame.has_value() == false`.
    [[nodiscard]] std::optional<uint32_t> shouldReallocate(uint32_t frameLength, uint16_t alignment) const;

    /// @return non-null pointer to the value at the specific offset within the current stack frame.
    template <typename T> T* frameValueAt(const uint16_t offset);

    /// @return non-null pointer to the value at the specific offset within the current stack frame.
    template <typename T> const T* frameValueAt(const uint16_t offset) const;

    /// @return The type within the current stack frame at `offset`. The underlying `const sy::Type*` may be nullptr.
    TypeOfValue typeAt(const uint16_t offset) const;

    /// Sets the type at `offset` to `type`. If `type` is not a null type, will also set the following slots to
    /// nullptr provided that the type requires that many slots to store an object.
    void setTypeAt(const TypeOfValue type, const uint16_t offset);

    void setFrameFunction(const uint16_t functionIndex);

    /// By default, values use 1KB.
    /// On targets with 64 bit pointers, the types minimum allocation is 1KB. On targets with 32 bit pointers,
    /// such as wasm32, the types minimum allocation is 512B.
    static constexpr size_t MIN_SLOTS = 128;

    /// Values are aligned to either their smaller-than-page allocation size, or are page aligned.
    /// Alignments greater than page alignment makes no sense.
    static constexpr size_t MIN_VALUES_ALIGNMENT = 128 * alignof(uint64_t);

  private:
    Node() = default;

    void ensureOffsetWithinFrameBounds(const uint16_t offset) const;
};

template <typename T> inline T* Node::frameValueAt(const uint16_t offset) {
    ensureOffsetWithinFrameBounds(offset);
    return reinterpret_cast<T*>(&this->values[offset + this->nextBaseOffset]);
}

template <typename T> inline const T* Node::frameValueAt(const uint16_t offset) const {
    ensureOffsetWithinFrameBounds(offset);
    return reinterpret_cast<const T*>(&this->values[offset + this->nextBaseOffset]);
}

#endif // SY_INTERPRETER_STACK_NODE_HPP_
