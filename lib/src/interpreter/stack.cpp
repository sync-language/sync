
#include "../mem/allocator.hpp"
#include "stack.hpp"
#include "../util/assert.hpp"
#include "../mem/os_mem.hpp"
#include "../types/function/function.hpp"
#include "../program/program_internal.hpp"
#if _MSC_VER
#include <new>
#endif

static_assert(sizeof(Stack) == sizeof(Stack::Raw));
static_assert(alignof(Stack) == alignof(Stack::Raw));

namespace detail {
    
    #if _MSC_VER
    constexpr size_t STORAGE_ALIGNMENT = std::hardware_destructive_interference_size;
    #else
    constexpr size_t STORAGE_ALIGNMENT = 64;
    #endif

    alignas(STORAGE_ALIGNMENT) static thread_local Stack::stack_value_t defaultValueStorage[DEFAULT_STACK_SLOT_SIZE];

    alignas(STORAGE_ALIGNMENT) static thread_local Stack::stack_type_t defaultTypeStorage[DEFAULT_STACK_SLOT_SIZE];

    /// Creating this as a compile time known value avoids any problem of constructor ordering before main(),
    /// in the event anyone using this library requires script stuff to be done before main() as well.
    /// Furthermore, we avoid calling the destructor unnecessarily on program termination.
    static thread_local Stack::Raw threadLocalRawStack = {
        nullptr,                // instruction pointer
        0,                      // next base offset
        {0},                    // current frame
        defaultValueStorage,
        defaultTypeStorage,
        DEFAULT_STACK_SLOT_SIZE // has no allocation
    };

    /// For now, this value will never change, however it'll be supported anyways for when coroutines become a thing.
    static thread_local Stack* activeStack = reinterpret_cast<Stack*>(&threadLocalRawStack);
}

Stack::Stack() : Stack(detail::DEFAULT_STACK_SLOT_SIZE)
{}

Stack::Stack(size_t slots)
{
    sy_assert(slots >= 3, 
        "Stack must have at least 3 slots, as that's the smallest amount required to do meaningful work");

    const size_t bytes = slots * sizeof(void*);
    stack_value_t* values = reinterpret_cast<stack_value_t*>(page_malloc(bytes));
    stack_type_t* types = reinterpret_cast<stack_type_t*>(page_malloc(bytes));

    this->raw.currentFrame = Stack::Frame();
    this->raw.instructionPointer = nullptr;
    this->raw.nextBaseOffset = 0;
    this->raw.values = values;
    this->raw.types = types;
    this->raw.slots = slots;
}

Stack::~Stack()
{
    sy_assert(this != &getThisThreadDefaultStack(), "Cannot destruct the thread local default stack");

    page_free(reinterpret_cast<void*>(this->raw.values), this->bytesUsed());
    page_free(reinterpret_cast<void*>(this->raw.types), this->bytesUsed());
}

Stack &Stack::getThisThreadDefaultStack()
{
    return *reinterpret_cast<Stack*>(&detail::threadLocalRawStack);
}

Stack &Stack::getActiveStack()
{
    return *detail::activeStack;
}

FrameGuard Stack::pushFrame(uint16_t frameLength, uint16_t alignment, void *retValDst)
{
    sy_assert(alignment % 2 == 0, "Expected frame alignment to be a multiple of 2");
    sy_assert(frameLength > 0, "Frame length of 0 is useless");

    const uint16_t actualAlignment = alignment < 16 ? 16 : alignment;

    { // validate values are properly aligned
        const size_t asNum = reinterpret_cast<size_t>(this->raw.values);
        sy_assert(asNum % actualAlignment == 0, "The stack values must be aligned to the required frame alignment");
    }

    if(this->extendCurrentFrameForNextFrameAlignment(actualAlignment) == false) {
        return FrameGuard();
    }

    const size_t newBaseOffset = this->raw.nextBaseOffset + frameLength + OLD_FRAME_INFO_RESERVED_SLOTS;
    if(newBaseOffset > this->raw.slots) {
        return FrameGuard();
    }

    // store old frame data within memory of new frame
    if(this->raw.nextBaseOffset != 0) {
        sy_assert(this->raw.nextBaseOffset >= OLD_FRAME_INFO_RESERVED_SLOTS, 
            "Trying to set out of bounds memory before the stack memory allocations");

        const size_t nextFrameOffsetBefore = this->raw.nextBaseOffset - OLD_FRAME_INFO_RESERVED_SLOTS;
        stack_value_t* const beforeFrameStart1 = &this->raw.values[nextFrameOffsetBefore];
        stack_type_t* const beforeFrameStart2 = &this->raw.types[nextFrameOffsetBefore];

        // stupid const cast
        const Bytecode*& oldInstructionPtr =
            const_cast<const Bytecode*&>(reinterpret_cast<Bytecode*&>(beforeFrameStart1[OLD_INSTRUCTION_POINTER]));
        size_t& oldFrameLength =
            reinterpret_cast<size_t&>(beforeFrameStart1[OLD_FRAME_LENGTH]); // actually uint16_t
        void*& oldRetValDst =
            reinterpret_cast<void*&>(beforeFrameStart2[OLD_RETURN_VALUE_DST]);
        const sy::Function*& oldFunction =
            const_cast<const sy::Function*&>(reinterpret_cast<sy::Function*&>(beforeFrameStart2[OLD_RETURN_VALUE_DST]));

        oldInstructionPtr = this->raw.instructionPointer;
        oldFrameLength = static_cast<size_t>(this->raw.currentFrame.frameLength);
        oldRetValDst = this->raw.currentFrame.retValueDst;
        oldFunction = this->raw.currentFrame.function;
    }

    const Frame newFrame = {
        this->raw.nextBaseOffset,
        frameLength,
        retValDst,
        #if _DEBUG
        false // flag
        #endif
    };
    this->raw.currentFrame = newFrame;
    this->raw.nextBaseOffset += frameLength + OLD_FRAME_INFO_RESERVED_SLOTS;
    return FrameGuard(this);
}

void Stack::setFrameFunction(const sy::Function *function)
{
    this->raw.currentFrame.function = function;

    sy::Allocator alloc;
    sy_assert(this->raw.callstackLen <= this->raw.callstackCapacity, "Corrupted memory");
    if(this->raw.callstackLen == this->raw.callstackCapacity) {
        // 32 is a reasonable starting amount for a call stack, as this will be long lived (potentially whole program)
        const size_t newCapacity = this->raw.callstackCapacity == 0 ? 32 : this->raw.callstackCapacity * 2;
        const auto result = alloc.allocArray<const sy::Function*>(newCapacity);
        const sy::Function** mem = result.get();
        if(this->raw.callstackFunctions != nullptr) {
            for(size_t i = 0; i < this->raw.callstackLen; i++) {
                mem[i] = this->raw.callstackFunctions[i];
            }
            alloc.freeArray(this->raw.callstackFunctions, this->raw.callstackCapacity);
        }
        this->raw.callstackCapacity = newCapacity;
    }

    this->raw.callstackFunctions[this->raw.callstackLen] = function;
    this->raw.callstackLen += 1;
}

FrameGuard Stack::pushFunctionFrame(const sy::Function *function, void* retValDst)
{
    sy_assert(function->tag == sy::Function::Type::Script, "Can only push frames for script functions");
    const sy::InterpreterFunctionScriptInfo* scriptInfo =
        reinterpret_cast<const sy::InterpreterFunctionScriptInfo*>(function->fptr);
    const uint16_t frameLength = scriptInfo->stackSpaceRequired;
    FrameGuard guard = this->pushFrame(frameLength, function->alignment, retValDst);
    this->setFrameFunction(function);
    return std::move(guard);
}

sy::CallStack Stack::callStack() const
{
    return sy::CallStack(this->raw.callstackFunctions, this->raw.callstackLen);
}

void Stack::popFrame()
{
    sy_assert(this->raw.nextBaseOffset != 0, "No more frames to pop");
    sy_assert(this->currentFrameValidated(), "Cannot pop non-validated frame");

    const size_t offset = this->raw.currentFrame.frameLength + OLD_FRAME_INFO_RESERVED_SLOTS;
    
    sy_assert(offset <= this->raw.nextBaseOffset, "Stack was corrupted");

    this->raw.nextBaseOffset -= offset;
    if(this->raw.nextBaseOffset == 0) {
        return;
    }

    const size_t currentFrameBaseBefore = this->raw.currentFrame.basePointerOffset - OLD_FRAME_INFO_RESERVED_SLOTS;
    const stack_value_t* const beforeCurrentFrameStart1 = &this->raw.values[currentFrameBaseBefore];
    const stack_type_t* const beforeCurrentFrameStart2 = &this->raw.types[currentFrameBaseBefore];

    const size_t oldFrameBaseBefore = this->raw.nextBaseOffset - OLD_FRAME_INFO_RESERVED_SLOTS;
    stack_value_t* const beforeOldFrameStart1 = &this->raw.values[oldFrameBaseBefore];
    stack_type_t* const beforeOldFrameStart2 = &this->raw.types[oldFrameBaseBefore];

    beforeOldFrameStart1[OLD_INSTRUCTION_POINTER] = beforeCurrentFrameStart1[OLD_INSTRUCTION_POINTER];
    beforeOldFrameStart1[OLD_FRAME_LENGTH] = beforeCurrentFrameStart1[OLD_FRAME_LENGTH];
    beforeOldFrameStart2[OLD_RETURN_VALUE_DST] = beforeCurrentFrameStart2[OLD_RETURN_VALUE_DST];
    beforeOldFrameStart2[OLD_FUNCTION] = beforeCurrentFrameStart2[OLD_FUNCTION];

    const Bytecode* const oldInstructionPtr = reinterpret_cast<Bytecode*>(beforeOldFrameStart1[OLD_INSTRUCTION_POINTER]);
    const uint16_t oldFrameLength = static_cast<uint16_t>(reinterpret_cast<size_t>(beforeOldFrameStart1[OLD_FRAME_LENGTH]));
    void* const oldRetValDst = reinterpret_cast<void*>(beforeOldFrameStart2[OLD_RETURN_VALUE_DST]);
    const sy::Function* const oldFunction = reinterpret_cast<sy::Function*>(beforeOldFrameStart2[OLD_FUNCTION]);

    sy_assert(this->raw.nextBaseOffset > static_cast<size_t>(oldFrameLength - OLD_FRAME_INFO_RESERVED_SLOTS), "Integer underflow");

    if(oldFunction != nullptr) {
        const size_t newCallstackLen = this->raw.callstackLen - 1;
        this->raw.callstackFunctions[newCallstackLen] = nullptr;
        this->raw.callstackLen = newCallstackLen;
    }

    const Frame oldFrame = {
        this->raw.nextBaseOffset - oldFrameLength - OLD_FRAME_INFO_RESERVED_SLOTS,
        oldFrameLength,
        oldRetValDst,
        oldFunction,
        #if _DEBUG
        true // flag
        #endif
    };

    this->raw.instructionPointer = oldInstructionPtr;
    this->raw.currentFrame = oldFrame;
}

bool Stack::currentFrameValidated() const
{
    #if _DEBUG
    return this->raw.currentFrame.validated;
    #else
    return true;
    #endif
}

bool Stack::extendCurrentFrameForNextFrameAlignment(uint16_t alignment)
{
    sy_assert(alignment % 2 == 0, "Expected frame alignment to be a multiple of 2");
    sy_assert(alignment >= 16, "Alignment should be greater than or equal to 16");

    const uint16_t normalizedAlignment = alignment / sizeof(void*);
    const uint16_t remainder = this->raw.nextBaseOffset % normalizedAlignment;
    if(remainder == 0) { // next base offset is already correctly aligned
        return true;
    }

    const uint16_t offset = remainder - normalizedAlignment;
    const size_t newBaseOffset = this->raw.nextBaseOffset + static_cast<size_t>(offset);
    if(newBaseOffset >= this->raw.slots) { // would stack overflow
        return false;
    }

    this->raw.currentFrame.frameLength += offset;
    this->raw.nextBaseOffset = newBaseOffset;
    
    sy_assert(this->raw.nextBaseOffset % alignment == 0, "Adjusted base offset must satisfy alignment requirements");

    return true;
}

FrameGuard::~FrameGuard()
{
    if(this->stack == nullptr) return;
    
    this->stack->popFrame();
    this->stack = nullptr;
}

FrameGuard::FrameGuard(FrameGuard&& other) : stack(other.stack) {
    other.stack = nullptr;
}

FrameGuard &FrameGuard::operator=(FrameGuard && other) {
    this->stack = other.stack;
    other.stack = nullptr;
    return *this;
}

bool FrameGuard::checkOverflow() const
{
    if(this->stack == nullptr) {
        return true;
    }

    #if _DEBUG
    Stack* mutableStack = const_cast<Stack*>(this->stack);
    mutableStack->raw.currentFrame.validated = true;
    #endif
    return false;
}

#ifdef SYNC_LIB_TEST

#include "../doctest.h"

#endif // SYNC_LIB_TEST
