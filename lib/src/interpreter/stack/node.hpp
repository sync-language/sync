#pragma once
#ifndef SY_INTERPRETER_STACK_NODE_HPP_
#define SY_INTERPRETER_STACK_NODE_HPP_

#include "../../core.h"
#include "frame.hpp"
#include <optional>

struct Bytecode;

class Node SY_CLASS_FINAL {
public:
    /// See `Node::MIN_SLOTS`. Is aligned to `Node::MIN_VALUES_ALIGNMENT` or page alignment.
    uint64_t*               values = nullptr;
    /// See `Node::MIN_SLOTS`. Is aligned to `ALLOC_CACHE_ALIGN` or page alignment.
    uintptr_t*              types = nullptr;
    /// `slots * sizeof(uint64_t)` is the amount of bytes occupied by all of the memory allocated for
    /// each of `values` and `types`.
    uint32_t                slots = 0;
    ///
    uint32_t                nextBaseOffset = Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
    /// If `currentFrame.has_value() == true`, then this node is currently in use,
    /// preventing certain operations such as `reallocate(...)`.
    std::optional<Frame>    currentFrame{};
    /// How many frames this node has. If `frameDepth > 0`, then this node is in use,
    /// preventing certain operations such as `reallocate(...)`.
    uint16_t                frameDepth = 0;

    Node(const uint32_t minSlotSize);

    ~Node() noexcept;

    Node(Node&& other) noexcept;

    Node& operator=(Node&& other) = delete;

    Node(const Node& other) = delete;

    Node& operator=(const Node& other) = delete;

    /// @brief Forcibly reallocates this node to either grow or shrink it's allocation size
    /// @param minSlotSize Can be less than `this->slots`.
    void reallocate(const uint32_t minSlotSize);

    bool hasEnoughSpaceForFrame(const uint32_t frameLength, const uint16_t alignment) const;

    std::optional<Frame> pushFrame(
        uint32_t frameLength, 
        uint16_t alignment,
        void* retValDst, 
        std::optional<Frame*> currentFrame, 
        const Bytecode* instructionPointer
    );

    /// @brief Pushes a frame onto this node from a previous node. Expects this node to not be in use,
    /// allowing reallocation.
    /// @return 
    /// # Debug Asserts
    /// `this->currentFrame.has_value() == false`.
    Frame pushFrameAllowReallocate(
        const uint32_t frameLength,
        const uint16_t alignment,
        void* const retValDst,
        std::optional<Frame*> previousFrame,
        const Bytecode* const instructionPointer
    );

    std::optional<std::tuple<Frame, const Bytecode*, bool>> popFrame(const uint16_t currentFrameLenMinusOne);

    /// Checks if this node needs reallocation for the new frame length and alignment.
    /// If it does, returns a valid option with the new reallocation minimum size, that is guaranteed to fit
    /// the frame length at the specified alignment including with having to shift the base offset for alignment
    /// requirements.
    /// If it does not need reallocation, returns a null option.
    /// # Debug Asserts
    /// `this->currentFrame.has_value() == false`.
    std::optional<uint32_t> shouldReallocate(uint32_t frameLength, uint16_t alignment) const;

    /// By default, values use 1KB.
    /// On targets with 64 bit pointers, the types minimum allocation is 1KB. On targets with 32 bit pointers,
    /// such as wasm32, the types minimum allocation is 512B.
    static constexpr size_t MIN_SLOTS = 128;
    
    /// Values are aligned to either their smaller-than-page allocation size, or are page aligned.
    /// Alignments greater than page alignment makes no sense.
    static constexpr size_t MIN_VALUES_ALIGNMENT = 128 * alignof(uint64_t);
    
SY_CLASS_TEST_PRIVATE: 
    Node() = default;
};

#endif // SY_INTERPRETER_STACK_NODE_HPP_