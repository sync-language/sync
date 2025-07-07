#pragma once
#ifndef SY_INTERPRETER_STACK_STACK_HPP_
#define SY_INTERPRETER_STACK_STACK_HPP_

#include "../../core.h"
#include "frame.hpp"
#include <type_traits>
#include <optional>
#include <tuple>

namespace sy {
    class Type;
    class Function;
    class CallStack;
}

struct Bytecode;
class Node;

class FrameGuard;

class Stack SY_CLASS_FINAL {
public:
    static constexpr size_t BITS_PER_STACK_OPERAND = 16;
    static constexpr size_t MAX_FRAME_LEN = 1 << BITS_PER_STACK_OPERAND;

    Stack() = default;

    ~Stack() noexcept;

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
    [[nodiscard]] FrameGuard pushFrame(uint32_t frameLength, uint16_t alignment, void* retValDst);

    [[nodiscard]] FrameGuard pushFunctionFrame(const sy::Function* function, void* retValDst);

    [[nodiscard]] sy::CallStack callStack() const;

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
    const sy::Type* typeAt(uint16_t offset) const;

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
    bool isOwnedTypeAt(uint16_t offset) const;

    void* returnDst();

    /// @brief Pushes the argument onto the script stack. Potentially adds another stack node if the frame
    /// and arguments wouldn't fit within the current node.
    /// @param argMem Memory to copy the argument data from.
    /// @param type The type of the argument.
    /// @param offset The offset within the next stack frame to set it at.
    /// @return `true` if the argument was pushed into the current stack node. `false` if pushed to one after.
    [[nodiscard]] bool pushScriptFunctionArg(
        const void* argMem,
        const sy::Type* type,
        uint16_t offset,
        const uint32_t frameLength,
        const uint16_t frameAlign
    );

    [[nodiscard]] std::optional<Frame*> getCurrentFrame();

private:
    /// Instances of `sy::Type` are required to have alignment of `alignof(void*)`, therefore on all target platforms,
    /// the lowest bit will be zeroed, and thus can be used as a flag bit, conserving memory.
    static constexpr uintptr_t TYPE_NOT_OWNED_FLAG = 0b1;

    void addOneNode(const uint32_t requiredFrameLength);

    // Maybe good idea to not store the instruction pointer itself, but the offset within the bytecode.
    // That way it can occupy 48 bits rather than a full pointer, and then the other 16 bits can be the frame length.
    // This allows storing the previous function, while saving room for 1 more another old frame data store.

    void setOptionalTypeAt(const sy::Type* type, uint16_t offset, bool isRef);

    const Bytecode* previousInstructionPointer() const;

    friend class FrameGuard;

    /// Pops the current frame from the stack, and restores the old one. Does not unwind the stack.
    /// # Debug Asserts
    /// Expects there to be an old stack to restore. For example, calling `pushFrame(...)` once, and then calling
    /// `popFrame(...)` twice is an error. The first pop is fine, but the second has no frame to restore.
    void popFrame();
    
private:

    const Bytecode*         instructionPointer = nullptr;
    Frame                   currentFrame = {0};
    Node*                   nodes = nullptr;
    size_t                  nodesLen = 0;
    size_t                  nodesCapacity = 0;
    size_t                  currentNode = 0;
    const sy::Function**    callstackFunctions = nullptr;
    uint16_t                callstackLen = 0;
    uint16_t                callstackCapacity = 0;
};

/// An RAII guard over a stack frame, to automatically handle popping the frame, and checking for stack overflow.
/// The `checkOverflow(...)` member function MUST be called. Upon calling, it signals to the stack that the frame
/// has been validated, and thus can be used.
class FrameGuard final {
public:
    ~FrameGuard();
    FrameGuard(FrameGuard&& other);
    FrameGuard& operator=(FrameGuard&& other);

    FrameGuard(const FrameGuard&) = delete;
    FrameGuard& operator=(const FrameGuard&) = delete;

private:
    friend class Stack;
    FrameGuard(Stack* inStack) : stack(inStack) {}
private:
    Stack* stack;
};

#endif // SY_INTERPRETER_STACK_STACK_HPP_