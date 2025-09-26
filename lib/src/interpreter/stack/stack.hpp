#pragma once
#ifndef SY_INTERPRETER_STACK_STACK_HPP_
#define SY_INTERPRETER_STACK_STACK_HPP_

#include "../../core.h"
#include "frame.hpp"
#include "node.hpp"
#include <optional>
#include <tuple>
#include <type_traits>

namespace sy {
class Type;
class Function;
class CallStack;
} // namespace sy

struct Bytecode;

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
    [[nodiscard]] FrameGuard pushFrame(uint32_t frameLength, uint16_t alignment, void* retValDst);

    [[nodiscard]] FrameGuard pushFunctionFrame(const sy::Function* function, void* retValDst);

    [[nodiscard]] sy::CallStack callStack() const;

    [[nodiscard]] const Bytecode* getInstructionPointer();

    void setInstructionPointer(const Bytecode* bytecode);

    /// @return non-null pointer to the value at the specific offset within the current stack frame.
    template <typename T>[[nodiscard]] T* frameValueAt(const uint16_t offset);

    /// @return non-null pointer to the value at the specific offset within the current stack frame.
    template <typename T>[[nodiscard]] const T* frameValueAt(const uint16_t offset) const;

    /// @return The type within the current stack frame at `offset`. The underlying `const sy::Type*` may be nullptr.
    [[nodiscard]] Node::TypeOfValue typeAt(const uint16_t offset) const;

    /// Sets the type at `offset` to `type`. If `type` is not a null type, will also set the following slots to
    /// nullptr provided that the type requires that many slots to store an object.
    void setTypeAt(const Node::TypeOfValue type, const uint16_t offset);

    [[nodiscard]] void* returnDst();

    /// @brief Pushes the argument onto the script stack. Potentially adds another stack node if the frame
    /// and arguments wouldn't fit within the current node.
    /// @param argMem Memory to copy the argument data from.
    /// @param type The type of the argument.
    /// @param offset The offset within the next stack frame to set it at.
    /// @return The new starting offset for the next argument
    uint16_t pushScriptFunctionArg(const void* argMem, const sy::Type* type, uint16_t offset,
                                   const uint32_t frameLength, const uint16_t frameAlign);

    [[nodiscard]] std::optional<Frame> getCurrentFrame();

  private:
    void addOneNode(const uint32_t requiredFrameLength);

    friend class FrameGuard;

    /// Pops the current frame from the stack, and restores the old one. Does not unwind the stack.
    /// # Debug Asserts
    /// Expects there to be an old stack to restore. For example, calling `pushFrame(...)` once, and then calling
    /// `popFrame(...)` twice is an error. The first pop is fine, but the second has no frame to restore.
    void popFrame();

  private:
    const Bytecode* instructionPointer = nullptr;
    Node* nodes = nullptr;
    size_t nodesLen = 0;
    size_t nodesCapacity = 0;
    size_t currentNode = 0;
    const sy::Function** callstackFunctions = nullptr;
    uint16_t callstackLen = 0;
    uint16_t callstackCapacity = 0;
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

template <typename T> inline T* Stack::frameValueAt(const uint16_t offset) {
    return this->nodes[this->currentNode].frameValueAt<T>(offset);
}

template <typename T> inline const T* Stack::frameValueAt(const uint16_t offset) const {
    return this->nodes[this->currentNode].frameValueAt<T>(offset);
}

#endif // SY_INTERPRETER_STACK_STACK_HPP_
