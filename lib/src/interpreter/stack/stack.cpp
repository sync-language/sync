
#include "../../mem/allocator.hpp"
#include "stack.hpp"
#include "../../util/assert.hpp"
#include "../../mem/os_mem.hpp"
#include "../../types/function/function.hpp"
#include "../../program/program_internal.hpp"
#include "../../types/type_info.hpp"
#include "../../threading/alloc_cache_align.hpp"
#include "../bytecode.hpp"
#include <utility>
#include <iostream>
#include <cstring>
#include <new>
#if __GNUC__
#include <sys/mman.h>
#endif

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

    Node& currNode = this->nodes[currentNode];
    if(currNode.isInUse() == false) {
        sy_assert(currentNode == 0, "If the current node isn't in use, it's the first node");

        currNode.pushFrameAllowReallocate(frameLength, alignment, retValDst, std::nullopt, this->instructionPointer);
    } else {  
        const bool success = currNode.pushFrameNoReallocate(
            frameLength, actualAlignment, retValDst, this->instructionPointer);
        if(!success) {
            sy_assert(currNode.isInUse(), "Expected node to be in use");
            const Frame currFrame = this->nodes[currentNode].currentFrame.value();
            // initialize the next node if necessary
            this->addOneNode(currNode.slots + frameLength); // TODO determine better way to do over-allocation       
            this->nodes[currentNode + 1].pushFrameAllowReallocate(
                frameLength, actualAlignment, retValDst, currFrame, this->instructionPointer);
            this->currentNode += 1;
        }
    }

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
    }

    this->callstackFunctions[this->callstackLen] = function;
    this->nodes[this->currentNode].setFrameFunction(this->callstackLen);
    this->callstackLen += 1;

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

Node::TypeOfValue Stack::typeAt(const uint16_t offset) const
{
    return this->nodes[this->currentNode].typeAt(offset);
}

void Stack::setTypeAt(const Node::TypeOfValue type, const uint16_t offset)
{
    this->nodes[this->currentNode].setTypeAt(type, offset);
}

void *Stack::returnDst()
{
    return this->nodes[currentNode].currentFrame.value().retValueDst;
}

uint16_t Stack::pushScriptFunctionArg(
    const void* argMem,
    const sy::Type* type,
    uint16_t offset,
    const uint32_t frameLength,
    const uint16_t frameAlign
) {
    std::optional<uint16_t> result = this->nodes[this->currentNode].pushScriptFunctionArg(argMem, type, offset, frameLength, frameAlign);
    if(result.has_value()) {
        return result.value();
    }

    // TODO determine better way to do over-allocation 
    this->addOneNode(this->nodes[this->currentNode].slots + frameLength);
    uint16_t actualResult = this->nodes[this->currentNode + 1].pushScriptFunctionArg(argMem, type, offset, frameLength, frameAlign).value();
    return actualResult;
}

std::optional<Frame> Stack::getCurrentFrame()
{
    return this->nodes[this->currentNode].currentFrame;
}

void Stack::popFrame()
{
    auto popResult = this->nodes[this->currentNode].popFrame();
    if(!popResult.has_value()) {
        sy_assert(this->currentNode == 0, "Node incorrectly reported having no previous frame");
        this->instructionPointer = nullptr;
        return;
    }

    Frame           oldFrame = std::get<0>(popResult.value());
    const Bytecode* oldInstructionPointer = std::get<1>(popResult.value());

    if(this->nodes[this->currentNode].isInUse() == false) {
        //this->nodes[this->currentNode - 1].currentFrame = oldFrame;
        (void)oldFrame;
        this->currentNode -= 1;
    }
    
    this->instructionPointer = oldInstructionPointer;
}

void Stack::addOneNode(const uint32_t requiredFrameLength)
{
    sy_assert(this->nodesCapacity != 0, "Initial allocation should have been done");

    if(this->nodesLen > (this->currentNode + 1)) {
        return; // already added
    }

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

#ifndef SYNC_LIB_NO_TESTS

#include "../../doctest.h"

TEST_CASE("push frame on thread local stack") {
    Stack& tls = Stack::getThisThreadDefaultStack();
    FrameGuard guard = tls.pushFrame(1, 1, nullptr);
    tls.setTypeAt(nullptr, 0);
}

TEST_CASE("push frame on active stack") {
    Stack& active = Stack::getActiveStack();
    FrameGuard guard = active.pushFrame(1, 1, nullptr);
}

TEST_CASE("construct stack") {
    Stack stack = Stack();
    FrameGuard guard = stack.pushFrame(1, 1, nullptr);
}

TEST_CASE("overflow stack with first frame") {
    Stack stack = Stack(); // only has 1 actual slot
    FrameGuard guard = stack.pushFrame(2, 1, nullptr);
}

TEST_CASE("2 frames") {
    Stack& active = Stack::getActiveStack();
    FrameGuard guard1 = active.pushFrame(4, 1, nullptr);
    Bytecode bytecode;
    active.setInstructionPointer(&bytecode);
    {
        FrameGuard guard2 = active.pushFrame(9, 1, nullptr);
    }
}

TEST_CASE("many nested frames") {    
    Stack& active = Stack::getActiveStack();
    FrameGuard guard1 = active.pushFrame(1, 1, nullptr);
    Bytecode bytecode;
    active.setInstructionPointer(&bytecode);
    {
        FrameGuard guard2 = active.pushFrame(1, 1, nullptr);
        {
            FrameGuard guard3 = active.pushFrame(1, 1, nullptr);
            {
                FrameGuard guard4 = active.pushFrame(1, 1, nullptr);
                {
                    FrameGuard guard5 = active.pushFrame(1, 1, nullptr);
                }
            }
        }
    }
}

TEST_CASE("frames of various length") {
    Stack& active = Stack::getActiveStack();
    FrameGuard guard1 = active.pushFrame(100, 1, nullptr);
    Bytecode bytecode;
    active.setInstructionPointer(&bytecode);
    {
        FrameGuard guard2 = active.pushFrame(400, 1, nullptr);
        {
            FrameGuard guard3 = active.pushFrame(3, 1, nullptr);
            {
                FrameGuard guard4 = active.pushFrame(80, 1, nullptr);
                {
                    FrameGuard guard5 = active.pushFrame(1000, 1, nullptr);
                }
            }
        }
    }
}

TEST_CASE("max frame") {
    Stack& active = Stack::getActiveStack();
    FrameGuard guard = active.pushFrame(UINT16_MAX + 1, 1, nullptr);
}

#endif // SYNC_LIB_NO_TESTS
