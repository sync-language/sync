
#include "../mem/allocator.hpp"
#include "stack.hpp"
#include "../util/assert.hpp"
#include "../mem/os_mem.hpp"
#include "../types/function/function.hpp"
#include "../program/program_internal.hpp"
#include "../types/type_info.hpp"
#include <utility>
#include <iostream>
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

    alignas(STORAGE_ALIGNMENT) static thread_local uintptr_t defaultTypeStorage[DEFAULT_STACK_SLOT_SIZE];

    /// Creating this as a compile time known value avoids any problem of constructor ordering before main(),
    /// in the event anyone using this library requires script stuff to be done before main() as well.
    /// Furthermore, we avoid calling the destructor unnecessarily on program termination.
    static thread_local Stack::Raw threadLocalRawStack = {
        nullptr,                // instruction pointer
        0,                      // next base offset
        {0},                    // current frame
        defaultValueStorage,
        defaultTypeStorage,
        DEFAULT_STACK_SLOT_SIZE, // has no allocation
        nullptr,                // no callstack functions
        0,                      // no callstack length
        0                       // no callstack capacity
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
    uintptr_t* types = reinterpret_cast<uintptr_t*>(page_malloc(bytes));
    
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

FrameGuard Stack::pushFrame(size_t frameLength, uint16_t alignment, void *retValDst)
{
    sy_assert(alignment % 2 == 0, "Expected frame alignment to be a multiple of 2");
    sy_assert(frameLength > 0, "Frame length of 0 is useless");
    sy_assert(frameLength <= MAX_FRAME_LEN, "Frame length cannot exceed maximum");

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

        this->storeCurrentFrameInfoInNext();
    }

    const Frame newFrame = {
        this->raw.nextBaseOffset + OLD_FRAME_INFO_RESERVED_SLOTS,
        frameLength,
        retValDst,
        nullptr,
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
    sy_assert(function->tag == sy::Function::CallType::Script, "Can only push frames for script functions");
    const sy::InterpreterFunctionScriptInfo* scriptInfo =
        reinterpret_cast<const sy::InterpreterFunctionScriptInfo*>(function->fptr);
    const size_t frameLength = scriptInfo->stackSpaceRequired;
    FrameGuard guard = this->pushFrame(frameLength, function->alignment, retValDst);
    this->setFrameFunction(function);
    return guard;
}

sy::CallStack Stack::callStack() const
{
    return sy::CallStack(this->raw.callstackFunctions, this->raw.callstackLen);
}

const Bytecode *Stack::getInstructionPointer()
{
    sy_assert(this->raw.instructionPointer != nullptr, "Cannot get invalid instruction pointer");
    return this->raw.instructionPointer;
}

void Stack::setInstructionPointer(const Bytecode *bytecode)
{
    sy_assert(bytecode != nullptr, "Cannot set invalid instruction pointer");
    this->raw.instructionPointer = bytecode;
}

void* Stack::valueMemoryAt(uint16_t offset)
{
    sy_assert(static_cast<size_t>(offset) < this->raw.currentFrame.frameLength, "Cannot access past the frame length");
    sy_assert(this->currentFrameValidated(), "Cannot operate on invalid stack frame");
    const size_t actualOffset = this->raw.currentFrame.basePointerOffset + static_cast<size_t>(offset);
    return &this->raw.values[actualOffset];
}

const sy::Type* Stack::typeAt(uint16_t offset) const
{
    sy_assert(static_cast<size_t>(offset) < this->raw.currentFrame.frameLength, "Cannot access past the frame length");
    sy_assert(this->currentFrameValidated(), "Cannot operate on invalid stack frame");
    const size_t actualOffset = this->raw.currentFrame.basePointerOffset + static_cast<size_t>(offset);
    const uintptr_t typeMemAsInt = this->raw.types[actualOffset];
    const uintptr_t maskedAwayFlag = typeMemAsInt & (~TYPE_NOT_OWNED_FLAG);
    return reinterpret_cast<const sy::Type*>(maskedAwayFlag);
}

void Stack::setTypeAt(const sy::Type *type, uint16_t offset)
{
    sy_assert(type != nullptr, "Cannot set null type");
    sy_assert(this->currentFrameValidated(), "Cannot operate on invalid stack frame");
    this->setOptionalTypeAt(type, offset, false);
}

void Stack::setNonOwningTypeAt(const sy::Type *type, uint16_t offset)
{    
    sy_assert(type != nullptr, "Cannot set null type");
    sy_assert(this->currentFrameValidated(), "Cannot operate on invalid stack frame");
    this->setOptionalTypeAt(type, offset, true);
}

void Stack::setNullTypeAt(uint16_t offset)
{
    sy_assert(this->currentFrameValidated(), "Cannot operate on invalid stack frame");
    this->setOptionalTypeAt(nullptr, offset, false);
}

bool Stack::isOwnedTypeAt(uint16_t offset) const
{
    sy_assert(static_cast<size_t>(offset) < this->raw.currentFrame.frameLength, "Cannot access past the frame length");
    sy_assert(this->currentFrameValidated(), "Cannot operate on invalid stack frame");
    const size_t actualOffset = this->raw.currentFrame.basePointerOffset + static_cast<size_t>(offset);
    const uintptr_t typeMemAsInt = this->raw.types[actualOffset];
    const uintptr_t maskedAwayType = typeMemAsInt & TYPE_NOT_OWNED_FLAG;
    return maskedAwayType != TYPE_NOT_OWNED_FLAG;
}

void *Stack::returnDst()
{
    return this->raw.currentFrame.retValueDst;
}

bool Stack::pushScriptFunctionArg(const void *argMem, const sy::Type *type, uint16_t offset)
{
    const size_t actualOffset = this->raw.nextBaseOffset + OLD_FRAME_INFO_RESERVED_SLOTS + static_cast<size_t>(offset);
    const size_t slotsOccupied = type->sizeType / 8;
    { // validate no stack overflow     
        const size_t requiredStackSize = actualOffset + slotsOccupied;
        if(requiredStackSize > this->raw.slots) {
            return false;
        }
    }

    std::memcpy(&this->raw.values[actualOffset], argMem, type->sizeType);
    this->raw.types[actualOffset] = reinterpret_cast<uintptr_t>(type);
    if(slotsOccupied > 1) {
        for(size_t i = 1; i < slotsOccupied; i++) {
            this->raw.types[actualOffset + i] = reinterpret_cast<uintptr_t>(nullptr);
        }
    }
    return true;
}

void Stack::popFrame()
{
    sy_assert(this->raw.nextBaseOffset != 0, "No more frames to pop");
    sy_assert(this->currentFrameValidated(), "Cannot pop non-validated frame");
    
    sy_assert(this->raw.currentFrame.frameLength <= this->raw.nextBaseOffset, "Stack was corrupted");

    const size_t oldNextBaseOffset = 
        this->raw.nextBaseOffset - (this->raw.currentFrame.frameLength + OLD_FRAME_INFO_RESERVED_SLOTS);

    if(oldNextBaseOffset == 0) {
        this->raw.nextBaseOffset = 0;
        return;
    }

    const Frame oldFrame = this->previousFrame();
    const Bytecode* const oldInstructionPtr = this->previousInstructionPointer();

    sy_assert(this->raw.nextBaseOffset > (oldFrame.frameLength - OLD_FRAME_INFO_RESERVED_SLOTS), "Integer underflow");

    if(oldFrame.function != nullptr) {
        const size_t newCallstackLen = this->raw.callstackLen - 1;
        this->raw.callstackFunctions[newCallstackLen] = nullptr;
        this->raw.callstackLen = newCallstackLen;
    }

    this->raw.instructionPointer = oldInstructionPtr;
    this->raw.currentFrame = oldFrame;
    this->raw.nextBaseOffset = oldNextBaseOffset;
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

    const uint16_t offset = normalizedAlignment - remainder;
    const size_t newBaseOffset = this->raw.nextBaseOffset + static_cast<size_t>(offset);
    if(newBaseOffset >= this->raw.slots) { // would stack overflow
        return false;
    }

    this->raw.currentFrame.frameLength += offset;
    this->raw.nextBaseOffset = newBaseOffset;
    
    sy_assert((this->raw.nextBaseOffset * sizeof(void*)) % alignment == 0, "Adjusted base offset must satisfy alignment requirements");

    return true;
}

static bool ptrIsAligned(const void* p, size_t alignment) {
    // Works for null pointers
    return (((uintptr_t)p) % alignment) == 0;
}

void Stack::setOptionalTypeAt(const sy::Type *type, uint16_t offset, bool isRef)
{
    static_assert(alignof(sy::Type) > 1, "Lowest bit must be reserved");
    sy_assert(static_cast<size_t>(offset) < this->raw.currentFrame.frameLength, "Cannot access past the frame length");
    sy_assert(this->currentFrameValidated(), "Cannot operate on invalid stack frame");

    const size_t actualOffset = this->raw.currentFrame.basePointerOffset + static_cast<size_t>(offset);

    if(type == nullptr) {
        this->raw.types[actualOffset] = reinterpret_cast<uintptr_t>(nullptr);
    } 
    
    sy_assert(ptrIsAligned(reinterpret_cast<const void*>(type), alignof(sy::Type)), "Type pointer misaligned");
    const size_t slotsOccupied = type->sizeType / 8;
    const size_t requiredFrameLength = static_cast<size_t>(offset) + slotsOccupied;
    sy_assert(requiredFrameLength < this->raw.currentFrame.frameLength, "Cannot set type past the frame length");

    const uintptr_t typePtrAsInt = reinterpret_cast<uintptr_t>(type);
    const uintptr_t refTag = isRef ? TYPE_NOT_OWNED_FLAG : 0; // would be optimized

    this->raw.types[actualOffset] = typePtrAsInt | refTag;
    if(slotsOccupied > 1) {
        for(size_t i = 1; i < slotsOccupied; i++) {
            this->raw.types[actualOffset + i] = reinterpret_cast<uintptr_t>(nullptr);
        }
    }
}

void Stack::storeCurrentFrameInfoInNext()
{
    stack_value_t* const beforeFrameStart1 = &this->raw.values[this->raw.nextBaseOffset];
    uintptr_t* const beforeFrameStart2 = &this->raw.types[this->raw.nextBaseOffset];

    // stupid const cast
    const Bytecode*& oldInstructionPtr =
        const_cast<const Bytecode*&>(reinterpret_cast<Bytecode*&>(beforeFrameStart1[OLD_INSTRUCTION_POINTER]));
    size_t& oldFrameLength =
        reinterpret_cast<size_t&>(beforeFrameStart1[OLD_FRAME_LENGTH]);
    void*& oldRetValDst =
        reinterpret_cast<void*&>(beforeFrameStart2[OLD_RETURN_VALUE_DST]);
    const sy::Function*& oldFunction =
        const_cast<const sy::Function*&>(reinterpret_cast<sy::Function*&>(beforeFrameStart2[OLD_RETURN_VALUE_DST]));

    oldInstructionPtr = this->raw.instructionPointer;
    oldFrameLength = static_cast<size_t>(this->raw.currentFrame.frameLength);
    oldRetValDst = this->raw.currentFrame.retValueDst;
    oldFunction = this->raw.currentFrame.function;
}

Stack::Frame Stack::previousFrame() const
{
    sy_assert(this->raw.nextBaseOffset != 0, "Cannot get non-existant previous frame");
    sy_assert(this->currentFrameValidated(), "Cannot pop non-validated frame");

    const Frame currentFrame = this->raw.currentFrame;
    const size_t oldInfoStartOffset = currentFrame.basePointerOffset - OLD_FRAME_INFO_RESERVED_SLOTS;

    const stack_value_t* const beforeCurrentFrameStart1 = &this->raw.values[oldInfoStartOffset];
    const uintptr_t* const beforeCurrentFrameStart2 = &this->raw.types[oldInfoStartOffset];

    const size_t oldFrameLength = reinterpret_cast<size_t>(beforeCurrentFrameStart1[OLD_FRAME_LENGTH]);
    void* const oldRetValDst = reinterpret_cast<void*>(beforeCurrentFrameStart2[OLD_RETURN_VALUE_DST]);
    const sy::Function* const oldFunction = reinterpret_cast<sy::Function*>(beforeCurrentFrameStart2[OLD_FUNCTION]);

    const Frame oldFrame = {
        currentFrame.basePointerOffset - oldFrameLength - OLD_FRAME_INFO_RESERVED_SLOTS,
        oldFrameLength,
        oldRetValDst,
        oldFunction,
        #if _DEBUG
        true // flag
        #endif
    };
    return oldFrame;
}

const Bytecode *Stack::previousInstructionPointer() const
{
    sy_assert(this->raw.nextBaseOffset != 0, "Cannot get non-existant previous frame");
    sy_assert(this->currentFrameValidated(), "Cannot pop non-validated frame");

    const size_t oldInfoStartOffset = this->raw.currentFrame.basePointerOffset - OLD_FRAME_INFO_RESERVED_SLOTS;

    const stack_value_t* const beforeCurrentFrameStart1 = &this->raw.values[oldInfoStartOffset];

    const Bytecode* const oldInstructionPtr = reinterpret_cast<Bytecode*>(beforeCurrentFrameStart1[OLD_INSTRUCTION_POINTER]);
    return oldInstructionPtr;
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

TEST_CASE("push frame on thread local stack") {
    Stack& tls = Stack::getThisThreadDefaultStack();
    FrameGuard guard = tls.pushFrame(1, 16, nullptr);
    CHECK_FALSE(guard.checkOverflow());
}

TEST_CASE("push frame on active stack") {
    Stack& active = Stack::getActiveStack();
    FrameGuard guard = active.pushFrame(1, 16, nullptr);
    CHECK_FALSE(guard.checkOverflow());
}

TEST_CASE("construct stack") {
    Stack stack = Stack(3);
    FrameGuard guard = stack.pushFrame(1, 16, nullptr);
    CHECK_FALSE(guard.checkOverflow());
}

TEST_CASE("overflow stack with first frame") {
    Stack stack = Stack(3); // only has 1 actual slot
    FrameGuard guard = stack.pushFrame(2, 16, nullptr);
    CHECK(guard.checkOverflow());
}

TEST_CASE("2 frames") {
    Stack& active = Stack::getActiveStack();
    FrameGuard guard1 = active.pushFrame(4, 16, nullptr);
    CHECK_FALSE(guard1.checkOverflow());
    {
        FrameGuard guard2 = active.pushFrame(9, 16, nullptr);
        CHECK_FALSE(guard2.checkOverflow());
    }
}

TEST_CASE("many nested frames") {    
    Stack& active = Stack::getActiveStack();
    FrameGuard guard1 = active.pushFrame(1, 16, nullptr);
    CHECK_FALSE(guard1.checkOverflow());
    {
        FrameGuard guard2 = active.pushFrame(1, 16, nullptr);
        CHECK_FALSE(guard2.checkOverflow());
        {
            FrameGuard guard3 = active.pushFrame(1, 16, nullptr);
            CHECK_FALSE(guard3.checkOverflow());
            {
                FrameGuard guard4 = active.pushFrame(1, 16, nullptr);
                CHECK_FALSE(guard4.checkOverflow());
                {
                    FrameGuard guard5 = active.pushFrame(1, 16, nullptr);
                    CHECK_FALSE(guard5.checkOverflow());
                }
            }
        }
    }
}

TEST_CASE("frames of various length") {
    Stack& active = Stack::getActiveStack();
    FrameGuard guard1 = active.pushFrame(100, 16, nullptr);
    CHECK_FALSE(guard1.checkOverflow());
    {
        FrameGuard guard2 = active.pushFrame(400, 16, nullptr);
        CHECK_FALSE(guard2.checkOverflow());
        {
            FrameGuard guard3 = active.pushFrame(3, 16, nullptr);
            CHECK_FALSE(guard3.checkOverflow());
            {
                FrameGuard guard4 = active.pushFrame(80, 16, nullptr);
                CHECK_FALSE(guard4.checkOverflow());
                {
                    FrameGuard guard5 = active.pushFrame(1000, 16, nullptr);
                    CHECK_FALSE(guard5.checkOverflow());
                }
            }
        }
    }
}

TEST_CASE("max frame") {
    Stack& active = Stack::getActiveStack();
    FrameGuard guard = active.pushFrame(UINT16_MAX, 16, nullptr);
    CHECK_FALSE(guard.checkOverflow());
}

#endif // SYNC_LIB_TEST
