#pragma once
#ifndef SY_INTERPRETER_STACK_HPP_
#define SY_INTERPRETER_STACK_HPP_

#include "../core.h"
#include <type_traits>
#include <optional>
#include <tuple>

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
    class Type;
    class Function;
    class CallStack;
}

struct Bytecode;

class FrameGuard;

class Stack {
public:
    static constexpr size_t BITS_PER_STACK_OPERAND = 16;
    static constexpr size_t MAX_FRAME_LEN = 1 << BITS_PER_STACK_OPERAND;

    struct Frame;

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

    struct Frame {
        // Sync only compiles on targets with full support for 64 bit integers (not necessarily 64 bit architectures
        // due to the existence of wasm32). As a result, frame metadata is stored as 4, 64 bit integer (2 + 2).

        uint32_t basePointerOffset;
        uint32_t frameLength;
        uint16_t functionIndex;
        void*    retValueDst;

        static std::optional<uint32_t> frameExtendAmountForAlignment(
            const uint32_t totalSlots, const uint32_t nextBaseOffset, const uint16_t alignment);

        /// @brief Attempts to read the frame metadata of the previous stack frame.
        /// @param valuesMem The memory region that contains the first half of the frame metadata
        /// @param typesMem The memory region that contains the second half of the frame metadata
        /// @return A valid frame if there is a frame to return to (has a valid instruction pointer to return to)
        /// along with the corresponding instruction pointer, otherwise an empty optional.
        static std::optional<std::tuple<Frame, const Bytecode*>> readFromMemory(
            const uint64_t* valuesMem, const uintptr_t* typesMem);

        void storeInMemory(uint64_t* valuesMem, uintptr_t* typesMem, const Bytecode* instructionPointer) const;

        static void storeNullFrameInMemory(uint64_t* valueMem, uintptr_t* typesMem);

        static const Bytecode* readOldInstructionPointer(const size_t* valuesMem);

        static void storeOldInstructionPointer(size_t* valuesMem, const Bytecode* instructionPointer);
        
        /// The amount of slots the old stack frame info needs to store itself within the bounds of the new frame.
        static constexpr size_t OLD_FRAME_INFO_RESERVED_SLOTS = 2;

    private:

        /// The index used to read the instruction pointer of the previous frame.
        /// From `Frame::basePointerOffset - OLD_FRAME_INFO_RESERVED_SLOTS`, from array `Raw::values`.
        static constexpr size_t OLD_INSTRUCTION_POINTER = 0;
        /// The index used to read the frame length of the previous frame.
        /// From `Frame::basePointerOffset - OLD_FRAME_INFO_RESERVED_SLOTS`, from array `Raw::values`.
        static constexpr size_t OLD_FRAME_LENGTH_AND_FUNCTION_INDEX = 1;
        /// The index used to read the return value destination of the previous frame. 
        /// From `Frame::basePointerOffset - OLD_FRAME_INFO_RESERVED_SLOTS`, from array `Raw::types`.
        static constexpr size_t OLD_RETURN_VALUE_DST = 0;
        /// The index used to read the function of the previous frame. 
        /// From `Frame::basePointerOffset - OLD_FRAME_INFO_RESERVED_SLOTS`, from array `Raw::types`.
        static constexpr size_t OLD_BASE_POINTER_OFFSET = 1;
    };

    
    struct Node {
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

        /// @brief Calculates what the next base offset needs to be to satisfy the byte alignment of a new
        /// stack frame. The difference between the return value and `Node::nextBaseOffset` member variable can be used
        /// to calculate how much the current frame needs to increase it's length by.
        /// @param currentNextBaseOffset Typically is `this->nextBaseOffset`, 
        /// however is passed as an argument for testing
        /// @param byteAlign Byte alignment of the next base offset
        /// @return The aligned next base offset, which is always greater than or equal to 
        /// `Stack::Frame::OLD_FRAME_INFO_RESERVED_SLOTS`
        static uint32_t requiredBaseOffsetForByteAlignment(uint32_t currentNextBaseOffset, uint16_t byteAlign);

        /// By default, values use 1KB.
        /// On targets with 64 bit pointers, the types minimum allocation is 1KB. On targets with 32 bit pointers,
        /// such as wasm32, the types minimum allocation is 512B.
        static constexpr size_t MIN_SLOTS = 128;
        
        /// Values are aligned to either their smaller-than-page allocation size, or are page aligned.
        /// Alignments greater than page alignment makes no sense.
        static constexpr size_t MIN_VALUES_ALIGNMENT = 128 * alignof(uint64_t);

    private:

        struct Allocation {
            uint64_t*   values;
            uintptr_t*  types;
            uint32_t    slots;
        };

        static Allocation allocateStack(const uint32_t minSlotSize);

        static void freeStack(Allocation& allocation);

    };

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
class FrameGuard {
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

#endif // SY_INTERPRETER_STACK_HPP_