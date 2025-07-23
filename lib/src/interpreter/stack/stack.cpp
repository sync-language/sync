
#include "../../mem/allocator.hpp"
#include "stack.hpp"
#include "../../util/assert.hpp"
#include "../../mem/os_mem.hpp"
#include "../../types/function/function.hpp"
#include "../../program/program_internal.hpp"
#include "../../types/type_info.hpp"
#include "../../threading/alloc_cache_align.hpp"
#include "node.hpp"
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
    size_t bytes = sizeof(Node);
    while((bytes % ALLOC_CACHE_ALIGN) != 0) {
        bytes += sizeof(Node);
    }
    return bytes / sizeof(Node);
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

    
    const bool success = this->nodes[currentNode].pushFrameNoReallocate(
        frameLength, actualAlignment, retValDst, this->instructionPointer);
    if(!success) {
        const std::optional<Frame> optCurrentFrame = std::nullopt;
        this->addOneNode(frameLength);
        this->currentNode += 1;
        this->nodes[currentNode].pushFrameAllowReallocate(
            frameLength, actualAlignment, retValDst, optCurrentFrame, this->instructionPointer);
    }

    this->currentFrame = this->nodes[currentNode].currentFrame.value();
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

const sy::Type *Stack::typeAt(uint16_t offset) const
{
    sy_assert(offset < this->currentFrame.frameLength, "Cannot access past the frame length");
    const uint32_t actualOffset = this->currentFrame.basePointerOffset + static_cast<uint32_t>(offset);
    Node& node = this->nodes[currentNode];
    return node.types[actualOffset].get();
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
    return node.types[actualOffset].isOwned();
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

    std::optional<uint32_t> optExtend = Frame::frameExtendAmountForAlignment(
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
    return onThisFrame;
}

std::optional<Frame*> Stack::getCurrentFrame()
{
    if(this->currentNode != 0 || this->nodes[0].nextBaseOffset != 0) {
        return std::optional<Frame*>(&this->currentFrame);
    } else {
        return std::nullopt;
    }
}

void Stack::popFrame()
{
    auto popResult = this->nodes[this->currentNode].popFrame();
    if(!popResult.has_value()) {
        sy_assert(this->currentNode == 0, "Node incorrectly reported having no previous frame");
        return;
    }

    Frame           oldFrame = std::get<0>(popResult.value());
    const Bytecode* oldInstructionPointer = std::get<1>(popResult.value());
    
    this->currentFrame = oldFrame;
    this->instructionPointer = oldInstructionPointer;
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
        node.types[actualOffset] = nullptr;
        return;
    } 
    
    sy_assert(ptrIsAligned(reinterpret_cast<const void*>(type), alignof(sy::Type)), "Type pointer misaligned");
    const size_t slotsOccupied = type->sizeType / 8;
    const size_t requiredFrameLength = static_cast<size_t>(offset) + slotsOccupied;
    sy_assert(requiredFrameLength < this->currentFrame.frameLength, "Cannot set type past the frame length");

    node.types[actualOffset].set(type, !isRef);
    if(slotsOccupied > 1) {
        for(size_t i = 1; i < slotsOccupied; i++) {
            node.types[actualOffset + i] = nullptr;
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

#include "../../doctest.h"

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
