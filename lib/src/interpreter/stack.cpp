
#include "../mem/allocator.hpp"
#include "stack.hpp"
#include "../util/assert.hpp"
#include "../mem/os_mem.hpp"
#include "../types/function/function.hpp"
#include "../program/program_internal.hpp"
#include "../types/type_info.hpp"
#include "../threading/alloc_cache_align.hpp"
#include <utility>
#include <iostream>
#include <cstring>
#if _MSC_VER
#include <new>
#elif __GNUC__
#include <sys/mman.h>
#endif

static_assert(sizeof(size_t) == sizeof(void*));
static_assert(alignof(size_t) == alignof(void*));
//static_assert(reinterpret_cast<size_t>(nullptr) == 0);

namespace detail {
    /// For now, this value will never change, however it'll be supported anyways for when coroutines become a thing.
    static thread_local Stack activeStack{};
}

std::optional<uint32_t> Stack::Frame::frameExtendAmountForAlignment(
    const uint32_t totalSlots, const uint32_t nextBaseOffset, const uint16_t alignment)
{
    sy_assert(alignment % 2 == 0, "Expected frame alignment to be a multiple of 2");
    sy_assert(alignment >= 16, "Alignment should be greater than or equal to 16");

    const uint16_t normalizedAlignment = alignment / sizeof(void*);
    const uint16_t remainder = nextBaseOffset % normalizedAlignment;
    if(remainder == 0) {
        return std::optional<uint32_t>(0); // Does not need to extend at all because is aligned
    }

    const uint32_t offset = normalizedAlignment - remainder;
    const uint32_t newBaseOffset = nextBaseOffset + static_cast<uint32_t>(normalizedAlignment - remainder);
    if(newBaseOffset >= totalSlots) {
        return std::optional<uint32_t>(); // empty cause would stack overflow
    }

    sy_assert((newBaseOffset * sizeof(void*)) % alignment == 0, 
        "Adjusted base offset must satisfy alignment requirements");

    return std::optional<uint32_t>(offset);
}

std::optional<std::tuple<Stack::Frame, const Bytecode*>> Stack::Frame::readFromMemory(
    const uint64_t* valuesMem, const uint64_t* typesMem)
{
    const Bytecode* oldInstructionPointer = reinterpret_cast<const Bytecode*>(valuesMem[OLD_INSTRUCTION_POINTER]);
    if(oldInstructionPointer == nullptr) {
        return std::nullopt;
    }

    const uint64_t frameLengthAndFunctionIndex = valuesMem[OLD_FRAME_LENGTH_AND_FUNCTION_INDEX];
    void* oldRetDst = const_cast<void*>(reinterpret_cast<const void*>(&typesMem[OLD_RETURN_VALUE_DST]));
    const uint32_t oldBasePointerOffset = 
        *reinterpret_cast<const uint32_t*>(&valuesMem[OLD_BASE_POINTER_OFFSET]);

    const Frame oldFrame = {
        oldBasePointerOffset,
        static_cast<uint32_t>(frameLengthAndFunctionIndex & 0xFFFFFFFF),    // frame length
        static_cast<uint16_t>(frameLengthAndFunctionIndex >> 32),           // function index
        oldRetDst
    };
    return std::make_tuple(oldFrame, oldInstructionPointer);
}

void Stack::Frame::storeInMemory(uint64_t* valuesMem, uint64_t* typesMem, const Bytecode* instructionPointer) const
{
    uint64_t* frameLengthAndFunctionIndex = &valuesMem[OLD_FRAME_LENGTH_AND_FUNCTION_INDEX];
    *frameLengthAndFunctionIndex = 
        static_cast<uint64_t>(this->frameLength) |
        (static_cast<uint64_t>(this->functionIndex) << 32);

    void** oldRetDst = reinterpret_cast<void**>(&typesMem[OLD_RETURN_VALUE_DST]);
    *oldRetDst = const_cast<void*>(this->retValueDst);

    uint32_t* oldBasePointerOffset = reinterpret_cast<uint32_t*>(&valuesMem[OLD_BASE_POINTER_OFFSET]);
    *oldBasePointerOffset = this->basePointerOffset;
}

void Stack::Frame::storeNullFrameInMemory(uint64_t *valueMem, uint64_t *typesMem)
{
    valueMem[OLD_INSTRUCTION_POINTER] = 0;
    valueMem[OLD_FRAME_LENGTH_AND_FUNCTION_INDEX] = 0;
    typesMem[OLD_RETURN_VALUE_DST] = 0;
    typesMem[OLD_BASE_POINTER_OFFSET] = 0;
}

const Bytecode *Stack::Frame::readOldInstructionPointer(const size_t *valuesMem)
{
    return reinterpret_cast<const Bytecode*>(valuesMem[OLD_INSTRUCTION_POINTER]);
}

void Stack::Frame::storeOldInstructionPointer(size_t *valuesMem, const Bytecode *instructionPointer)
{
    auto oldinstructionPointer = reinterpret_cast<const Bytecode**>(&valuesMem[OLD_INSTRUCTION_POINTER]);
    *oldinstructionPointer = instructionPointer;
}

Stack::~Stack() noexcept
{
    if(this->nodes == nullptr && this->callstackFunctions == nullptr) return;

    sy::Allocator alloc{};

    for(decltype(this->nodesLen) i = 0; i < nodesLen; i++) {
        nodes[i].~Node();
    }

    alloc.freeAlignedArray(this->nodes, this->nodesCapacity, ALLOC_CACHE_ALIGN);
    alloc.freeAlignedArray(this->callstackFunctions, this->callstackCapacity, ALLOC_CACHE_ALIGN);

    memset(this, 0, sizeof(Stack));
}

Stack &Stack::getThisThreadDefaultStack()
{
    return detail::activeStack;
}

Stack &Stack::getActiveStack()
{
    // TODO stack switching for co-routines?
    return detail::activeStack;
}

static constexpr size_t minNodeCapacityForCacheAlign() {
    size_t bytes = sizeof(Stack::Node);
    while((bytes % ALLOC_CACHE_ALIGN) != 0) {
        bytes += sizeof(Stack::Node);
    }
    return bytes / sizeof(Stack::Node);
}

static constexpr size_t minCallstackFunctionCapacityForCacheAlign() {
    size_t bytes = sizeof(const sy::Function*);
    while((bytes % ALLOC_CACHE_ALIGN) != 0) {
        bytes += sizeof(const sy::Function*);
    }
    return bytes / sizeof(const sy::Function*);
}

FrameGuard Stack::pushFrame(uint32_t frameLength, uint16_t alignment, void *retValDst)
{
    sy_assert(frameLength > 0, "Frame length of 0 is useless");
    sy_assert(frameLength <= MAX_FRAME_LEN, "Frame length too big");
    // TODO validate alignment power of 2
    sy_assert((page_size() % alignment) == 0, "Alignment greater than system page size makes no sense");

    { // perform initial allocations if necessary
        sy::Allocator alloc{};

        if(this->nodes == nullptr) {
            constexpr size_t capacity = minNodeCapacityForCacheAlign();
            this->nodes = alloc.allocAlignedArray<Node>(capacity, ALLOC_CACHE_ALIGN).get();
            this->nodesCapacity = capacity;

            Node* _ = new (&this->nodes[0]) Node(frameLength * 4); // TODO evaluate this default
            (void)_;
            this->nodesLen = 1;
        }

        if(this->callstackFunctions == nullptr) {
            constexpr size_t capacity = minCallstackFunctionCapacityForCacheAlign();
            this->callstackFunctions = alloc.allocAlignedArray<const sy::Function*>(capacity, ALLOC_CACHE_ALIGN).get();
            this->callstackCapacity = capacity;
        }
    }

    const uint16_t actualAlignment = alignment < 16 ? 16 : alignment;

    const std::optional<Frame*> optCurrentFrame = getCurrentFrame();
    std::optional<Frame> optFrame = this->nodes[currentNode].pushFrame(
        frameLength, actualAlignment, retValDst, optCurrentFrame, this->instructionPointer);
    if(!optFrame.has_value()) {
        this->addOneNode(frameLength);
        this->currentNode += 1;
        optFrame = this->nodes[currentNode].pushFrame(
            frameLength, actualAlignment, retValDst, optCurrentFrame, this->instructionPointer);
        sy_assert(optFrame.has_value(), "Adding new node should not have failed");
    }

    this->currentFrame = optFrame.value();
    return FrameGuard(this);
}

FrameGuard Stack::pushFunctionFrame(const sy::Function *function, void* retValDst)
{
    sy_assert(function->tag == sy::Function::CallType::Script, "Can only push frames for script functions");
    const sy::InterpreterFunctionScriptInfo* scriptInfo =
        reinterpret_cast<const sy::InterpreterFunctionScriptInfo*>(function->fptr);
    const uint32_t frameLength = scriptInfo->stackSpaceRequired;
    FrameGuard guard = this->pushFrame(frameLength, function->alignment, retValDst);

    { // potentially reallocate
        sy_assert(this->callstackFunctions != nullptr, "Initial allocations should have happened");

        if(this->callstackLen == callstackCapacity) {
            sy::Allocator alloc{};

            const uint16_t newCapacity = callstackCapacity * 2;
            auto newFunctions = alloc.allocAlignedArray<const sy::Function*>(newCapacity, ALLOC_CACHE_ALIGN).get();
            
            for(decltype(this->callstackLen) i = 0; i < this->callstackLen; i++) {
                newFunctions[i] = this->callstackFunctions[i];
            }

            alloc.freeAlignedArray(this->callstackFunctions, this->callstackCapacity, ALLOC_CACHE_ALIGN);
            this->callstackFunctions = newFunctions;
            this->callstackCapacity = newCapacity;
        }

        this->callstackFunctions[this->callstackLen] = function;
        this->callstackLen += 1;
    }

    return guard;
}

sy::CallStack Stack::callStack() const
{
    return sy::CallStack(this->callstackFunctions, this->callstackLen);
}

const Bytecode *Stack::getInstructionPointer()
{
    sy_assert(this->instructionPointer != nullptr, "Cannot get invalid instruction pointer");
    return this->instructionPointer;
}

void Stack::setInstructionPointer(const Bytecode *bytecode)
{
    sy_assert(bytecode != nullptr, "Cannot set invalid instruction pointer");
    this->instructionPointer = bytecode;
}

void* Stack::valueMemoryAt(uint16_t offset)
{
    sy_assert(offset < this->currentFrame.frameLength, "Cannot access past the frame length");
    const uint32_t actualOffset = this->currentFrame.basePointerOffset + static_cast<uint32_t>(offset);
    Node& node = this->nodes[currentNode];
    return &node.values[actualOffset];
}

const sy::Type* Stack::typeAt(uint16_t offset) const
{
    sy_assert(offset < this->currentFrame.frameLength, "Cannot access past the frame length");
    const uint32_t actualOffset = this->currentFrame.basePointerOffset + static_cast<uint32_t>(offset);
    Node& node = this->nodes[currentNode];
    const uintptr_t typeMemAsInt = node.types[actualOffset];
    const uintptr_t maskedAwayFlag = typeMemAsInt & (~TYPE_NOT_OWNED_FLAG);
    return reinterpret_cast<const sy::Type*>(maskedAwayFlag);
}

void Stack::setTypeAt(const sy::Type *type, uint16_t offset)
{
    sy_assert(type != nullptr, "Cannot set null type");
    this->setOptionalTypeAt(type, offset, false);
}

void Stack::setNonOwningTypeAt(const sy::Type *type, uint16_t offset)
{    
    sy_assert(type != nullptr, "Cannot set null type");
    this->setOptionalTypeAt(type, offset, true);
}

void Stack::setNullTypeAt(uint16_t offset)
{
    this->setOptionalTypeAt(nullptr, offset, false);
}

bool Stack::isOwnedTypeAt(uint16_t offset) const
{
    sy_assert(offset < this->currentFrame.frameLength, "Cannot access past the frame length");
    const uint32_t actualOffset = this->currentFrame.basePointerOffset + static_cast<uint32_t>(offset);
    Node& node = this->nodes[currentNode];
    const uintptr_t typeMemAsInt = node.types[actualOffset];
    const uintptr_t maskedAwayType = typeMemAsInt & TYPE_NOT_OWNED_FLAG;
    return maskedAwayType != TYPE_NOT_OWNED_FLAG;
}

void *Stack::returnDst()
{
    return this->currentFrame.retValueDst;
}

bool Stack::pushScriptFunctionArg(
    const void* argMem,
    const sy::Type* type,
    uint16_t offset,
    const uint32_t frameLength,
    const uint16_t frameAlign
) {
    bool onThisFrame = true;
    Node& currentNodeUsed = this->nodes[this->currentNode];
    std::optional<Frame*> currentFrame = this->getCurrentFrame();

    std::optional<uint32_t> optExtend = Stack::Frame::frameExtendAmountForAlignment(
        currentNodeUsed.slots,
        currentNodeUsed.nextBaseOffset,
        frameAlign
    );
    if(!optExtend.has_value()) {
        onThisFrame = false;
    } else { // extend frame
        uint32_t actualNextBaseOffset = currentNodeUsed.nextBaseOffset;
        if(currentFrame.has_value()) {
            actualNextBaseOffset += Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
            const uint32_t currentFrameLength = currentFrame.value()->frameLength;
            const uint32_t newFrameLen = currentFrameLength + optExtend.value();
            sy_assert(newFrameLen < MAX_FRAME_LEN, "Frame extension exceeds maximum frame length");
            currentFrame.value()->frameLength = static_cast<uint16_t>(newFrameLen);
        }
        currentNodeUsed.nextBaseOffset = actualNextBaseOffset + optExtend.value();
    }

    if(!currentNodeUsed.hasEnoughSpaceForFrame(frameLength, frameAlign)) {
        onThisFrame = false;
    }



    


    // const size_t actualOffset = this->raw.nextBaseOffset + OLD_FRAME_INFO_RESERVED_SLOTS + static_cast<size_t>(offset);
    // const size_t slotsOccupied = type->sizeType / 8;
    // { // validate no stack overflow     
    //     const size_t requiredStackSize = actualOffset + slotsOccupied;
    //     if(requiredStackSize > this->raw.slots) {
    //         return false;
    //     }
    // }

    // std::memcpy(&this->raw.values[actualOffset], argMem, type->sizeType);
    // this->raw.types[actualOffset] = reinterpret_cast<uintptr_t>(type);
    // if(slotsOccupied > 1) {
    //     for(size_t i = 1; i < slotsOccupied; i++) {
    //         this->raw.types[actualOffset + i] = reinterpret_cast<uintptr_t>(nullptr);
    //     }
    // }
    // return true;
    (void)argMem;
    (void)type;
    (void)offset;
    return true;
}

std::optional<Stack::Frame *> Stack::getCurrentFrame()
{
    if(this->currentNode != 0 || this->nodes[0].nextBaseOffset != 0) {
        return std::optional<Frame*>(&this->currentFrame);
    } else {
        return std::nullopt;
    }
}

#pragma region Node

Stack::Node::Node(const uint32_t minSlotSize)
    : nextBaseOffset(0)
{
    Allocation allocation = allocateStack(minSlotSize);
    
    this->values = allocation.values;
    this->types = allocation.types;
    this->slots = allocation.slots;
}

Stack::Node::~Node() noexcept
{
    if(this->slots == 0) return;

    Allocation allocation{
        this->values,
        this->types,
        this->slots
    };

    freeStack(allocation);

    this->values = nullptr;
    this->types = nullptr;
    this->slots = 0;
    this->nextBaseOffset = 0;
}

Stack::Node::Node(Node &&other) noexcept
    : values(other.values), types(other.types), slots(other.slots), nextBaseOffset(other.nextBaseOffset)
{
    other.values = nullptr;
    other.types = nullptr;
    other.slots = 0;
    other.nextBaseOffset = 0;
}

bool Stack::Node::hasEnoughSpaceForFrame(const uint32_t frameLength, const uint16_t alignment) const
{
    if ((this->nextBaseOffset + frameLength + Frame::OLD_FRAME_INFO_RESERVED_SLOTS) > this->slots) {
        return false;
    }
    return true;
}

void Stack::Node::reallocate(const uint32_t minSlotSize)
{
    sy_assert(!this->currentFrame.has_value(), "Cannot reallocate stack node while it's being used");

    if(this->slots > 0) {
        Allocation allocation{
            this->values,
            this->types,
            this->slots
        };

        freeStack(allocation);
    }
    
    Allocation allocation = allocateStack(minSlotSize);
    
    this->values = allocation.values;
    this->types = allocation.types;
    this->slots = allocation.slots;
}

std::optional<Stack::Frame> Stack::Node::pushFrame(
    uint32_t frameLength,
    uint16_t alignment,
    void *retValDst,
    std::optional<Frame *> currentFrame,
    const Bytecode *instructionPointer)
{
    sy_assert(alignment % 2 == 0, "Expected frame alignment to be a multiple of 2");
    sy_assert(frameLength > 0, "Frame length of 0 is useless");
    sy_assert(frameLength <= MAX_FRAME_LEN, "Frame length cannot exceed maximum");

    const uint16_t actualAlignment = alignment < 16 ? 16 : alignment;

    { // validate values are properly aligned
        const size_t asNum = reinterpret_cast<size_t>(this->values);
        sy_assert(asNum % actualAlignment == 0, "The stack values must be aligned to the required frame alignment");
    }

    { // check if fits
        if ((this->nextBaseOffset + frameLength + Frame::OLD_FRAME_INFO_RESERVED_SLOTS) > this->slots) {
            return std::optional<Frame>();
        }
    }

    { // extend frame
        uint32_t actualNextBaseOffset = this->nextBaseOffset;
        if(currentFrame.has_value()) actualNextBaseOffset += Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
        // TODO integer overflow checks

        std::optional<uint32_t> optExtendAmount = Frame::frameExtendAmountForAlignment(
            this->slots, actualNextBaseOffset, alignment);
        if (!optExtendAmount.has_value()) {
            return std::optional<Frame>();
        }

        if(currentFrame.has_value()) {
            this->nextBaseOffset = actualNextBaseOffset + optExtendAmount.value();
            const uint32_t currentFrameLength = currentFrame.value()->frameLength;
            const uint32_t newFrameLen = currentFrameLength + optExtendAmount.value();
            sy_assert(newFrameLen < MAX_FRAME_LEN, "Frame extension exceeds maximum frame length");
            currentFrame.value()->frameLength = static_cast<uint16_t>(newFrameLen);
        }
    }

    size_t* const valuesBefore  = &this->values[this->nextBaseOffset];
    size_t* const typesBefore   = &this->types[this->nextBaseOffset];
    if(currentFrame.has_value()) {
        currentFrame.value()->storeInMemory(valuesBefore, typesBefore, instructionPointer);
    } else {
        Frame::storeNullFrameInMemory(valuesBefore, typesBefore);
    }

    const Frame newFrame = {
        static_cast<uint32_t>(this->nextBaseOffset + Frame::OLD_FRAME_INFO_RESERVED_SLOTS),
        static_cast<uint16_t>(frameLength - 1),
        UINT16_MAX - 1,
        retValDst
    };

    this->nextBaseOffset += frameLength + Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
    return std::optional<Frame>(newFrame);
}

Stack::Frame Stack::Node::pushFrameAllowReallocate(
    const uint32_t frameLength,
    const uint16_t alignment,
    void *const retValDst,
    std::optional<Frame*> previousFrame,
    const Bytecode *const instructionPointer
) {
    sy_assert(this->currentFrame.has_value() == false, "Expected this node to not be in use");

    bool mustReallocate = false;
    do { // check if need to reallocate
        const size_t pageSize = page_size();
        sy_assert(alignment <= pageSize, "Alignment greater than page size does not make sense");
        const size_t asNum = reinterpret_cast<size_t>(this->values);

        // 
    } while(false);

    return Stack::Frame();
}

std::optional<std::tuple<Stack::Frame, const Bytecode *, bool>> Stack::Node::popFrame(
    const uint16_t currentFrameLenMinusOne)
{
    sy_assert(this->nextBaseOffset != 0, "No frames to pop");

    const uint32_t currentFrameLength = static_cast<uint32_t>(currentFrameLenMinusOne) + 1;
    sy_assert(currentFrameLength <= this->nextBaseOffset, "Stack was corrupted");

    const uint32_t oldNextBaseOffset 
        = this->nextBaseOffset - (currentFrameLength + Frame::OLD_FRAME_INFO_RESERVED_SLOTS);

    if(oldNextBaseOffset == 0) {
        this->nextBaseOffset = 0;
        return std::optional<std::tuple<Frame, const Bytecode*, bool>>();
    }
    
    const size_t oldInfoStartOffset = this->nextBaseOffset - Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
    const size_t* const valuesMem   = &this->values[oldInfoStartOffset];
    const size_t* const typesMem    = &this->types[oldInfoStartOffset];

    return std::nullopt;

    // Frame oldFrame = Frame::readFromMemory(valuesMem, typesMem);
    // const Bytecode* oldInstructionPointer = Frame::readOldInstructionPointer(valuesMem);

    // const bool backToEmptyNode = oldNextBaseOffset == Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
    // if(backToEmptyNode) {
    //     this->nextBaseOffset = 0;
    // } else {
    //     this->nextBaseOffset = oldNextBaseOffset;
    // }
    
    // auto tuple = std::make_tuple(oldFrame, oldInstructionPointer, backToEmptyNode);
    // return std::optional<std::tuple<Frame, const Bytecode*, bool>>(tuple);
}

std::optional<uint32_t> Stack::Node::shouldReallocate(uint32_t frameLength, uint16_t alignment) const
{
    // `alignment` must always be a power of 2
    // `frameLength` must be a multiple of alignment
    // Despite that, since Frame::OLD_FRAME_INFO_RESERVED_SLOTS are needed,
    // this may result in flawed alignment.
    // As a result, we double the input frame length so that even with the padding, we can still
    // always get correct alignment

    sy_assert(this->currentFrame.has_value() == false, "Expected this node to not be in use");
    sy_assert(frameLength <= MAX_FRAME_LEN, "Frame length cannot exceed the maximum");

    const size_t pageSize = page_size();
    sy_assert(alignment > 0, "Alignment must be non zero");
    sy_assert(alignment <= pageSize, "Alignment greater than page size does not make sense");
    sy_assert((frameLength % (alignment / sizeof(size_t))) == 0, "Frame length must be a multiple of alignment");
    sy_assert((alignment & (alignment - 1)) == 0, "Alignment must be a power of 2");

    const uint32_t minRequiredSlots = (frameLength * 2) + Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
    uint32_t reallocationSlots = alignment;
    while(reallocationSlots < minRequiredSlots) {
        reallocationSlots <<= 1;
    }

    if(this->slots < reallocationSlots) {
        return std::optional<uint32_t>(reallocationSlots);
    }

    if(reinterpret_cast<size_t>(this->values) % alignment != 0) {
        return std::optional<uint32_t>(reallocationSlots); // the allocation itself is not aligned enough
    }

    return std::nullopt;
}

Stack::Node::Allocation Stack::Node::allocateStack(const uint32_t minSlotSize)
{
    // TODO custom allocator for stack nodes : IAllocator

    Allocation aloc{};

    if(minSlotSize <= MIN_SLOTS) {
        sy::Allocator allocator;
        aloc.values = allocator.allocAlignedArray<uint64_t>(MIN_SLOTS, MIN_BYTES_ALLOCATED).get();
        aloc.types  = allocator.allocAlignedArray<uint64_t>(MIN_SLOTS, MIN_BYTES_ALLOCATED).get();
        aloc.slots  = MIN_SLOTS;
    } else {
        const size_t pageSize = page_size();
        size_t bytesToAllocate = static_cast<size_t>(minSlotSize) * sizeof(uint64_t);
        {
            const size_t remainder = bytesToAllocate % pageSize;
            if(remainder != 0) {
                bytesToAllocate += pageSize - remainder;
            }
        }

        void* valuesMem = page_malloc(bytesToAllocate);
        void* typesMem = page_malloc(bytesToAllocate);
        
        // https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        #if _MSC_VER
        sy_assert(valuesMem != nullptr, "Failed to allocate pages");
        sy_assert(typesMem != nullptr, "Failed to allocate pages");
        #elif __GNUC__
        sy_assert(valuesMem != MAP_FAILED, "Failed to allocate pages");
        sy_assert(typesMem != MAP_FAILED, "Failed to allocate pages");
        #endif

        aloc.values = reinterpret_cast<uint64_t*>(valuesMem);
        aloc.types  = reinterpret_cast<uint64_t*>(typesMem);
        aloc.slots  = static_cast<uint32_t>(bytesToAllocate / sizeof(uint64_t));
    }

    return aloc;
}

void Stack::Node::freeStack(Allocation &allocation)
{
    if(allocation.slots == MIN_SLOTS) {
        sy::Allocator allocator;
        allocator.freeAlignedArray(allocation.values, MIN_SLOTS, MIN_BYTES_ALLOCATED);
        allocator.freeAlignedArray(allocation.types, MIN_SLOTS, MIN_BYTES_ALLOCATED);
    }
    else {
        const size_t bytesAllocated = static_cast<size_t>(allocation.slots) * sizeof(uint64_t);
        page_free(allocation.values, bytesAllocated);
        page_free(allocation.types, bytesAllocated);
    }
}

#pragma endregion

void Stack::popFrame()
{
    auto popResult = this->nodes[this->currentNode].popFrame(this->currentFrame.frameLength);
    if(!popResult.has_value()) {
        sy_assert(this->currentNode == 0, "Node incorrectly reported having no previous frame");
        return;
    }

    Frame           oldFrame = std::get<0>(popResult.value());
    const Bytecode* oldInstructionPointer = std::get<1>(popResult.value());
    bool            shouldDecrementCurrentNode = std::get<2>(popResult.value());
    
    this->currentFrame = oldFrame;
    this->instructionPointer = oldInstructionPointer;
    if(shouldDecrementCurrentNode) {
        sy_assert(this->currentNode > 0, "Cannot decrement node");
        this->currentNode -= 1;
    }
}

void Stack::addOneNode(const uint32_t requiredFrameLength)
{
    sy_assert(this->nodesCapacity != 0, "Initial allocation should have been done");

    if(this->nodesLen == this->nodesCapacity) {
        sy::Allocator alloc{};

        const size_t newCapacity = nodesCapacity * 2;
        auto newNodes = alloc.allocAlignedArray<Node>(newCapacity, ALLOC_CACHE_ALIGN).get();
        
        for(decltype(this->nodesLen) i = 0; i < this->nodesLen; i++) {
            Node* _ = new (&newNodes[i]) Node(std::move(this->nodes[i]));
            (void)_;
        }

        alloc.freeAlignedArray(this->nodes, this->nodesCapacity, ALLOC_CACHE_ALIGN);
        this->nodes = newNodes;
        this->nodesCapacity = newCapacity;
    }

    const uint32_t minSlots = static_cast<uint32_t>(
        static_cast<double>(this->nodes[this->nodesLen - 1].slots + requiredFrameLength) * 1.5);
    Node* placedNode = new (&this->nodes[this->nodesLen]) Node(minSlots);
    (void)placedNode;
    this->nodesLen += 1;
}

static bool ptrIsAligned(const void* p, size_t alignment) {
    // Works for null pointers
    return (((size_t)p) % alignment) == 0;
}

void Stack::setOptionalTypeAt(const sy::Type *type, uint16_t offset, bool isRef)
{
    static_assert(alignof(sy::Type) > 1, "Lowest bit must be reserved");    
    sy_assert(offset < this->currentFrame.frameLength, "Cannot access past the frame length");
    const uint32_t actualOffset = this->currentFrame.basePointerOffset + static_cast<uint32_t>(offset);
    Node& node = this->nodes[currentNode];

    if(type == nullptr) {
        node.types[actualOffset] = reinterpret_cast<size_t>(nullptr);
        return;
    } 
    
    sy_assert(ptrIsAligned(reinterpret_cast<const void*>(type), alignof(sy::Type)), "Type pointer misaligned");
    const size_t slotsOccupied = type->sizeType / 8;
    const size_t requiredFrameLength = static_cast<size_t>(offset) + slotsOccupied;
    sy_assert(requiredFrameLength < this->currentFrame.frameLength, "Cannot set type past the frame length");

    const size_t typePtrAsInt = reinterpret_cast<size_t>(type);
    const size_t refTag = isRef ? TYPE_NOT_OWNED_FLAG : 0; // would be optimized

    node.types[actualOffset] = typePtrAsInt | refTag;
    if(slotsOccupied > 1) {
        for(size_t i = 1; i < slotsOccupied; i++) {
            node.types[actualOffset + i] = reinterpret_cast<size_t>(nullptr);
        }
    }
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

#ifdef SYNC_LIB_TEST

#include "../doctest.h"

using Node = Stack::Node;

TEST_SUITE("Node") {
    TEST_CASE("Construct destruct mininum") {
        auto node = Node(1);
        CHECK_EQ(node.slots, Node::MIN_SLOTS);
    }

    TEST_CASE("Construct destruct exactly minimum") {
        auto node = Node(Node::MIN_SLOTS);
        CHECK_EQ(node.slots, Node::MIN_SLOTS);
    }

    TEST_CASE("Construct destruct one above minimum") {
        auto node = Node(Node::MIN_SLOTS + 1);
        CHECK_GT(node.slots, Node::MIN_SLOTS);
    }

    TEST_CASE("Node reallocate bigger") {
        auto node = Node(1);
        node.reallocate(Node::MIN_SLOTS + 1);
        CHECK_GT(node.slots, Node::MIN_SLOTS);
    }

    TEST_CASE("Node reallocate smaller") {
        auto node = Node(Node::MIN_SLOTS + 1);
        node.reallocate(1);
        CHECK_EQ(node.slots, Node::MIN_SLOTS);
    }

    TEST_CASE("Node should reallocate true with simple alignment") {
        auto node = Node(4);
        auto result = node.shouldReallocate(node.slots + 1, alignof(size_t));
        CHECK(result.has_value());
        CHECK_GT(result.value(), node.slots);
    }

    TEST_CASE("Node should reallocate false with simple alignment") {
        auto node = Node(4);
        auto result = node.shouldReallocate(8, alignof(size_t));
        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("Node should reallocate true with alignment same as frame length") {
        auto node = Node(4);
        const uint16_t lenAlign = 1024;
        auto result = node.shouldReallocate(lenAlign, lenAlign);
        CHECK(result.has_value());
        CHECK_GT(result.value(), node.slots);
    }

    TEST_CASE("Node should reallocate false with alignment same as frame length") {
        auto node = Node(4096);
        const uint16_t lenAlign = 1024;
        auto result = node.shouldReallocate(lenAlign, lenAlign);
        CHECK_FALSE(result.has_value());
    }
}

// TEST_CASE("push frame on thread local stack") {
//     Stack& tls = Stack::getThisThreadDefaultStack();
//     FrameGuard guard = tls.pushFrame(1, 16, nullptr);
//     tls.setNullTypeAt(0);
// }

// TEST_CASE("push frame on active stack") {
//     Stack& active = Stack::getActiveStack();
//     FrameGuard guard = active.pushFrame(1, 16, nullptr);
// }

// TEST_CASE("construct stack") {
//     Stack stack = Stack();
//     FrameGuard guard = stack.pushFrame(1, 16, nullptr);
// }

// TEST_CASE("overflow stack with first frame") {
//     Stack stack = Stack(); // only has 1 actual slot
//     FrameGuard guard = stack.pushFrame(2, 16, nullptr);
// }

// TEST_CASE("2 frames") {
//     Stack& active = Stack::getActiveStack();
//     FrameGuard guard1 = active.pushFrame(4, 16, nullptr);
//     {
//         FrameGuard guard2 = active.pushFrame(9, 16, nullptr);
//     }
// }

// TEST_CASE("many nested frames") {    
//     Stack& active = Stack::getActiveStack();
//     FrameGuard guard1 = active.pushFrame(1, 16, nullptr);
//     {
//         FrameGuard guard2 = active.pushFrame(1, 16, nullptr);
//         {
//             FrameGuard guard3 = active.pushFrame(1, 16, nullptr);
//             {
//                 FrameGuard guard4 = active.pushFrame(1, 16, nullptr);
//                 {
//                     FrameGuard guard5 = active.pushFrame(1, 16, nullptr);
//                 }
//             }
//         }
//     }
// }

// TEST_CASE("frames of various length") {
//     Stack& active = Stack::getActiveStack();
//     FrameGuard guard1 = active.pushFrame(100, 16, nullptr);
//     {
//         FrameGuard guard2 = active.pushFrame(400, 16, nullptr);
//         {
//             FrameGuard guard3 = active.pushFrame(3, 16, nullptr);        
//             // active.setNullTypeAt(0);
//             // active.setNullTypeAt(1);
//             {
//                 FrameGuard guard4 = active.pushFrame(80, 16, nullptr);
//                 {
//                     FrameGuard guard5 = active.pushFrame(1000, 16, nullptr);
//                     // active.setNullTypeAt(0);
//                     // active.setNullTypeAt(500);
//                     // active.setNullTypeAt(999);
//                 }
//             }
//             // active.setNullTypeAt(1);
//             // active.setNullTypeAt(2);
//         }
//     }
// }

// TEST_CASE("max frame") {
//     Stack& active = Stack::getActiveStack();
//     FrameGuard guard = active.pushFrame(UINT16_MAX + 1, 16, nullptr);
// }

#endif // SYNC_LIB_TEST
