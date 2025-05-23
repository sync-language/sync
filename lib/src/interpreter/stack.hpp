#pragma once
#ifndef SY_INTERPRETER_STACK_HPP_
#define SY_INTERPRETER_STACK_HPP_

#include "../core.h"
#include <type_traits>

// Developers are allowed to change this value.
#ifndef SY_MAX_STACK_SIZE
/// The maximum stack size of the default stack on the thread. This value can be changed when compiling Sync.
/// 1 MB default (slots * 8 bytes per slot).
#define SY_MAX_STACK_SIZE (1 << 20)
#endif

namespace detail {
   constexpr size_t DEFAULT_STACK_SLOT_SIZE = SY_MAX_STACK_SIZE / sizeof(void*);
}

namespace sy {
    struct Type;
    class Function;
    class CallStack;
}

struct Bytecode;

class FrameGuard;

class Stack {
public:

    static constexpr size_t BITS_PER_STACK_OPERAND = 16;
    static constexpr size_t MAX_FRAME_LEN = 1 << BITS_PER_STACK_OPERAND;

    struct Frame {
        size_t basePointerOffset = 0;
        size_t frameLength = 0; // TODO maybe should be size_t or uint32_t, as UINT16_MAX should probably be a valid index
        void* retValueDst = nullptr;
        const sy::Function* function = nullptr;
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
    [[nodiscard]] FrameGuard pushFrame(size_t frameLength, uint16_t alignment, void* retValDst);

    /// Sets the function of the current stack frame.
    void setFrameFunction(const sy::Function* function);

    [[nodiscard]] FrameGuard pushFunctionFrame(const sy::Function* function, void* retValDst);

    [[nodiscard]] sy::CallStack callStack() const;

    [[nodiscard]] size_t slots() const { return this->raw.slots; }

    [[nodiscard]] size_t bytesUsed() const { return this->slots() * sizeof(void*); }

    [[nodiscard]] const Bytecode* getInstructionPointer();

    void setInstructionPointer(const Bytecode* bytecode);

    /// Will never return `nullptr`.
    void* valueMemoryAt(uint16_t offset);

    template<typename T>
    T& valueAt(uint16_t offset) {
        void* valMem = valueMemoryAt(offset);
        return *reinterpret_cast<typename std::remove_const<T>::type*>(valMem);
    }

    /// May return `nullptr`.
    const sy::Type* typeAt(uint16_t offset);

    /// Sets the type at a specific offset within the stack frame to be a valid type.
    /// @param type Non-null pointer.
    /// @param offset Offset within the stack frame
    void setTypeAt(const sy::Type* type, uint16_t offset);

    /// Sets the type at a specific offset within the stack frame to be a valid type.
    /// The type is flagged as non-owning, and thus will not be destroyed when the stack is unwinded.
    /// @param type Non-null pointer.
    /// @param offset Offset within the stack frame
    void setNonOwningTypeAt(const sy::Type* type, uint16_t offset);

    /// Sets the type at `offset` to be `nullptr`, marking it as invalid.
    void setNullTypeAt(uint16_t offset);

    /// @returns If the type at `offset` is an owned type, and thus would be destroyed when the stack is unwinded.
    bool isOwnedTypeAt(uint16_t offset);

    void* returnDst();

    /// @brief Attempts to push an argument to the stack frame after the current one.
    /// @param argMem Memory to copy the argument data from.
    /// @param type The type of the argument.
    /// @param offset The offset within the next stack frame to set it at.
    /// @return `true` if the arg was copied successfully copied, or `false` if a stack overflow would occur.
    [[nodiscard]] bool pushScriptFunctionArg(const void* argMem, const sy::Type* type, uint16_t offset);

    [[nodiscard]] const Frame& currentFrame() const { return this->raw.currentFrame; }

    using stack_value_t = void*;

    struct Raw {
        const Bytecode* instructionPointer;
        /// Offset from `values` and `types` indicated where the next frame should start
        size_t                  nextBaseOffset;
        Frame                   currentFrame;
        /// Allocated as pages
        stack_value_t*          values;
        /// Allocates as pages
        uintptr_t*           types;
        /// Does not need to be the amount of pages, or total number of bytes in the pages for `stack` and `types.
        /// For both `mmap` and `VirtualAlloc`, the length argument does not need to be a page multiple, as the C
        /// runtime library will handle it accordingly.
        size_t                  slots;
        const sy::Function**    callstackFunctions;
        size_t                  callstackLen;
        size_t                  callstackCapacity;
    };

private:

    /// The index used to read the instruction pointer of the previous frame.
    /// From `Frame::basePointerOffset - OLD_FRAME_INFO_RESERVED_SLOTS`, from array `Raw::values`.
    static constexpr size_t OLD_INSTRUCTION_POINTER = 0;
    /// The index used to read the frame length of the previous frame.
    /// From `Frame::basePointerOffset - OLD_FRAME_INFO_RESERVED_SLOTS`, from array `Raw::values`.
    static constexpr size_t OLD_FRAME_LENGTH = 1;
    /// The index used to read the return value destination of the previous frame. 
    /// From `Frame::basePointerOffset - OLD_FRAME_INFO_RESERVED_SLOTS`, from array `Raw::types`.
    static constexpr size_t OLD_RETURN_VALUE_DST = 0;
    /// The index used to read the function of the previous frame. 
    /// From `Frame::basePointerOffset - OLD_FRAME_INFO_RESERVED_SLOTS`, from array `Raw::types`.
    static constexpr size_t OLD_FUNCTION = 1;
    /// The amount of slots the old stack frame info needs to store itself within the bounds of the new frame.
    static constexpr size_t OLD_FRAME_INFO_RESERVED_SLOTS = 2;

    /// Instances of `sy::Type` are required to have alignment of `alignof(void*)`, therefore on all target platforms,
    /// the lowest bit will be zeroed, and thus can be used as a flag bit, conserving memory.
    static constexpr uintptr_t TYPE_NOT_OWNED_FLAG = 0b1;

    // Maybe good idea to not store the instruction pointer itself, but the offset within the bytecode.
    // That way it can occupy 48 bits rather than a full pointer, and then the other 16 bits can be the frame length.
    // This allows storing the previous function, while saving room for 1 more another old frame data store.

    bool extendCurrentFrameForNextFrameAlignment(uint16_t alignment);

    void setOptionalTypeAt(const sy::Type* type, uint16_t offset, bool isRef);

    void storeCurrentFrameInfoInNext();

    Frame previousFrame() const;

    const Bytecode* previousInstructionPointer() const;

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