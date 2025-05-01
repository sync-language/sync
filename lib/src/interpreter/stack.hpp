#pragma once
#ifndef SY_INTERPRETER_STACK_HPP_
#define SY_INTERPRETER_STACK_HPP_

#include "../core.h"

// Developers are allowed to change this value.
#ifndef SY_MAX_STACK_SIZE
/// The maximum stack size of the default stack on the thread. This value can be changed when compiling Sync.
/// 1 MB default (slots * 8 bytes per slot).
#define SY_MAX_STACK_SIZE (1 << 17)
#endif

namespace detail {
   constexpr size_t DEFAULT_STACK_SLOT_SIZE = SY_MAX_STACK_SIZE / sizeof(void*);
}

namespace sy {
    struct Type;
}

struct Bytecode;

class FrameGuard;

class Stack {
public:

    static constexpr size_t BITS_PER_STACK_OPERAND = 13;
    static constexpr size_t MAX_FRAME_LEN = 1 << BITS_PER_STACK_OPERAND;

    struct Frame {
        size_t basePointerOffset = 0;
        size_t frameLength = 0;
        void* retValueDst = nullptr;
        #if _DEBUG
        bool validated = false;
        #endif
    };

    Stack();

    /// @param slots Must be at least 3, as that's the minimum amount required to store previous (empty) frame info,
    /// and a singular value up to 8 bytes in size.
    Stack(size_t slots);

    ~Stack();

    static Stack& getThisThreadDefaultStack();

    static Stack& getActiveStack();

    /// Pushes a new frame onto the stack, given a length in slots, as well as return value and type destinations.
    /// Can stack overflow. Since functions track their return types, only the return value destination is required.
    /// @param frameLength The amount of slots the frame requires. Internally, some more will be added
    /// @param alignment The alignment required for the frame. If less than 16, set to 16. Must be a multiple of 2.
    /// 0 is a valid input. See types/function.h and types/function.hpp as to why.
    /// @param retValDst Memory address where the return value of a function should be copied to. Can be `nullptr`,
    /// meaning no return value destination.
    /// @return `true` if successful, otherwise `false` in which pushing the frame would overflow the stack.
    [[nodiscard]] FrameGuard pushFrame(size_t frameLength, size_t alignment, void* retValDst);

    [[nodiscard]] size_t slots() const { return this->raw.slots; }

    [[nodiscard]] size_t bytesUsed() const { return this->slots() * sizeof(void*); }

    using stack_value_t = void*;
    using stack_type_t = void*;

    struct Raw {
        const Bytecode* instructionPointer;
        /// Offset from `values` and `types` indicated where the next frame should start
        size_t          nextBaseOffset;
        Frame           currentFrame;
        /// Allocated as pages
        stack_value_t*  values;
        /// Allocates as pages
        stack_type_t*   types;
        /// Does not need to be the amount of pages, or total number of bytes in the pages for `stack` and `types.
        /// For both `mmap` and `VirtualAlloc`, the length argument does not need to be a page multiple, as the C
        /// runtime library will handle it accordingly.
        size_t          slots;
    };

private:

    /// The index used to read the instruction pointer of the previous frame.
    /// From `Frame::basePointerOffset - OLD_FRAME_INFO_RESERVED_SLOTS`, from array `Raw::values`.
    static constexpr size_t OLD_INSTRUCTION_POINTER = 0;
    /// The index used to read the instruction pointer of the previous frame.
    /// From `Frame::basePointerOffset - OLD_FRAME_INFO_RESERVED_SLOTS`, from array `Raw::values`.
    static constexpr size_t OLD_FRAME_LENGTH = 1;
    /// The index used to read the instruction pointer of the previous frame. 
    /// From `Frame::basePointerOffset - OLD_FRAME_INFO_RESERVED_SLOTS`, from array `Raw::types`.
    static constexpr size_t OLD_RETURN_VALUE_DST = 0;
    /// The amount of slots the old stack frame info needs to store itself within the bounds of the new frame.
    static constexpr size_t OLD_FRAME_INFO_RESERVED_SLOTS = 2;

    bool extendCurrentFrameForNextFrameAlignment(size_t alignment);

    friend class FrameGuard;

    /// Pops the current frame from the stack, and restores the old one. Does not unwind the stack.
    /// # Debug Asserts
    /// Expects there to be an old stack to restore. For example, calling `pushFrame(...)` once, and then calling
    /// `popFrame(...)` twice is an error. The first pop is fine, but the second has no frame to restore.
    void popFrame();

    bool currentFrameValidated() const;
    
private:

    Raw raw;
};

/// An RAII guard over a stack frame, to automatically handle popping the frame, and checking for stack overflow.
/// The `checkOverflow(...)` member function MUST be called. Upon calling, it signals to the stack that the frame
/// has been validated, and thus can be used.
class FrameGuard {
public:
    ~FrameGuard();
    FrameGuard(FrameGuard&& other);
    FrameGuard& operator=(FrameGuard&& other);

    FrameGuard(const FrameGuard&) = delete;
    FrameGuard& operator=(const FrameGuard&) = delete;

    /// This must be called. If not, the stack frame will not be validated. See `Stack::Frame::validated`
    /// @returns `true` if the push operation would have overflowed the stack (for early return), otherwise `false`.
    bool checkOverflow() const;

private:
    friend class Stack;
    FrameGuard() = default;
    FrameGuard(Stack* inStack) : stack(inStack) {}
private:
    Stack* stack = nullptr;
};

#endif // SY_INTERPRETER_STACK_HPP_