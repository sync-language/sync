#include "node.hpp"
#include "../../util/assert.hpp"
#include "stack.hpp"
#include "../../threading/alloc_cache_align.hpp"
#include "../../mem/os_mem.hpp"
#include "../../mem/allocator.hpp"
#include "../bytecode.hpp"
#if _MSC_VER
#include <new>
#elif __GNUC__
#include <sys/mman.h>
#endif

/// By default, values use 1KB.
/// On targets with 64 bit pointers, the types minimum allocation is 1KB. On targets with 32 bit pointers,
/// such as wasm32, the types minimum allocation is 512B.
constexpr size_t MIN_SLOTS = 128;

/// Values are aligned to either their smaller-than-page allocation size, or are page aligned.
/// Alignments greater than page alignment makes no sense.
constexpr size_t MIN_VALUES_ALIGNMENT = 128 * alignof(uint64_t);

#pragma region Memory_Allocation

struct Allocation {
    uint64_t*   values;
    uintptr_t*  types;
    uint32_t    slots;
};

static Allocation allocateStack(const uint32_t minSlotSize)
{
    // TODO custom allocator for stack nodes : IAllocator

    Allocation aloc{};

    if(minSlotSize <= MIN_SLOTS) {
        sy::Allocator allocator;
        aloc.values = allocator.allocAlignedArray<uint64_t>(MIN_SLOTS, MIN_VALUES_ALIGNMENT).get();
        aloc.types = allocator.allocAlignedArray<uintptr_t>(MIN_SLOTS, ALLOC_CACHE_ALIGN).get();
        aloc.slots = MIN_SLOTS;
    } else {
        const size_t pageSize = page_size();
        size_t valuesBytesToAllocate = static_cast<size_t>(minSlotSize) * sizeof(uint64_t);
        {
            const size_t remainder = valuesBytesToAllocate % pageSize;
            if(remainder != 0) {
                valuesBytesToAllocate += pageSize - remainder;
            }
        }
        const size_t typesBytesToAllocate = [valuesBytesToAllocate]() -> size_t {
            if constexpr(sizeof(uintptr_t) == sizeof(int64_t)) {
                return valuesBytesToAllocate;
            } else if constexpr(sizeof(uintptr_t) < sizeof(int64_t)) { 
                // 32 bit pointers (wasm32). We never target anything smaller
                // https://webassembly.org/features/
                // Gosh darn safari...
                return valuesBytesToAllocate / (sizeof(uint64_t) / sizeof(uintptr_t));
            } else { // pointer bigger than 64 bits
                // CHERI ? Maybe not worth considering
                return valuesBytesToAllocate * (sizeof(uintptr_t) / sizeof(uint64_t));
            }
        }();

        void* valuesMem = page_malloc(valuesBytesToAllocate);
        void* typesMem = page_malloc(typesBytesToAllocate);
        
        // https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        #if _MSC_VER
        sy_assert(valuesMem != nullptr, "Failed to allocate pages");
        sy_assert(typesMem != nullptr, "Failed to allocate pages");
        #elif __GNUC__
        sy_assert(valuesMem != MAP_FAILED, "Failed to allocate pages");
        sy_assert(typesMem != MAP_FAILED, "Failed to allocate pages");
        #endif

        aloc.values = reinterpret_cast<uint64_t*>(valuesMem);
        aloc.types = reinterpret_cast<uintptr_t*>(typesMem);
        aloc.slots = static_cast<uint32_t>(valuesBytesToAllocate / sizeof(uint64_t));
    }

    return aloc;
}

static void freeStack(Allocation& allocation)
{
    if(allocation.slots == MIN_SLOTS) {
        sy::Allocator allocator;
        allocator.freeAlignedArray(allocation.values, MIN_SLOTS, MIN_VALUES_ALIGNMENT);
        allocator.freeAlignedArray(allocation.types, MIN_SLOTS, ALLOC_CACHE_ALIGN);
    }
    else {
        const size_t valuesBytesAllocated = static_cast<size_t>(allocation.slots) * sizeof(uint64_t);
        const size_t typesBytesAllocated = [valuesBytesAllocated]() -> size_t {
            if constexpr(sizeof(uintptr_t) == sizeof(int64_t)) {
                return valuesBytesAllocated;
            } else if constexpr(sizeof(uintptr_t) < sizeof(int64_t)) { 
                // 32 bit pointers (wasm32). We never target anything smaller
                // https://webassembly.org/features/
                // Gosh darn safari...
                return valuesBytesAllocated / (sizeof(uint64_t) / sizeof(uintptr_t));
            } else { // pointer bigger than 64 bits
                // CHERI ? Maybe not worth considering
                return valuesBytesAllocated * (sizeof(uintptr_t) / sizeof(uint64_t));
            }
        }();
        page_free(allocation.values, valuesBytesAllocated);
        page_free(allocation.types, typesBytesAllocated);
    }
}

#pragma endregion

/// @brief Calculates what the next base offset needs to be to satisfy the byte alignment of a new
/// stack frame. The difference between the return value and `Node::nextBaseOffset` member variable can be used
/// to calculate how much the current frame needs to increase it's length by.
/// @param currentNextBaseOffset Typically is `this->nextBaseOffset`, 
/// however is passed as an argument for testing
/// @param byteAlign Byte alignment of the next base offset
/// @return The aligned next base offset, which is always greater than or equal to 
/// `Stack::Frame::OLD_FRAME_INFO_RESERVED_SLOTS`
static uint32_t requiredBaseOffsetForByteAlignment(uint32_t currentNextBaseOffset, uint16_t byteAlign)
{
    // The 2 slots BEFORE the base offset (for both the values and types, totaling 4 used slots)
    // must be used for storing the previous frame data.
    // For consistency purposes, even if no frame is supplied, the reserve slots are still used.
    sy_assert(currentNextBaseOffset >= Frame::OLD_FRAME_INFO_RESERVED_SLOTS, 
        "Next base offset should always be greater than or equal to the default");

    const uint32_t normalizedAlign = (byteAlign / sizeof(uint64_t)) < Frame::OLD_FRAME_INFO_RESERVED_SLOTS 
        ? Frame::OLD_FRAME_INFO_RESERVED_SLOTS : (byteAlign / sizeof(uint64_t));

    const uint32_t remainder = currentNextBaseOffset % normalizedAlign;
    if(remainder == 0) {
        return currentNextBaseOffset;
    }

    return currentNextBaseOffset + (normalizedAlign - remainder);
}

Node::Node(const uint32_t minSlotSize)
{
    Allocation allocation = allocateStack(minSlotSize);
    
    this->values = allocation.values;
    this->types = allocation.types;
    this->slots = allocation.slots;
}

Node::~Node() noexcept
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

Node::Node(Node &&other) noexcept
    : values(other.values), types(other.types), slots(other.slots), nextBaseOffset(other.nextBaseOffset)
{
    other.values = nullptr;
    other.types = nullptr;
    other.slots = 0;
    other.nextBaseOffset = 0;
}

bool Node::hasEnoughSpaceForFrame(const uint32_t frameLength, const uint16_t alignment) const
{
    if ((this->nextBaseOffset + frameLength) > this->slots) {
        return false;
    }
    return true;
}

bool Node::isInUse() const
{
    sy_assert(this->currentFrame.has_value() == (this->frameDepth != 0), "Invalid state");
    return this->frameDepth != 0;
}

void Node::reallocate(const uint32_t minSlotSize)
{
    sy_assert(!this->currentFrame.has_value(), "Cannot reallocate stack node while it's being used");
    sy_assert(this->frameDepth == 0, "Cannot reallocate stack node while it's being used");

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

// bool Node::pushFrame(uint32_t frameLength, uint16_t byteAlign, void *retValDst, std::optional<Frame *> previousFrame, const Bytecode *instructionPointer)
// {
//     sy_assert(previousFrame.has_value() == (instructionPointer != nullptr), 
//         "If there is a previous frame, a valid instruction pointer is expected and vice versa");

//     if(this->currentFrame.has_value()) {
//         sy_assert(previousFrame.has_value(), "Expected there to be a previous frame if this node is in use");
//         Frame& prevFrame = *previousFrame.value();
//         Frame& currFrame = this->currentFrame.value();
//         const char* message = "Expected previous frame to be current frame";
//         sy_assert(prevFrame.basePointerOffset   == currFrame.basePointerOffset, message);
//         sy_assert(prevFrame.frameLength         == currFrame.frameLength, message);
//         sy_assert(prevFrame.functionIndex       == currFrame.functionIndex, message);
//         sy_assert(prevFrame.retValueDst         == currFrame.retValueDst, message);

//         return this->pushFrameNoReallocate(frameLength, byteAlign, retValDst, instructionPointer);
//     } else {
//         this->pushFrameAllowReallocate(frameLength, byteAlign, retValDst, previousFrame, instructionPointer);
//         return true;
//     }
// }

bool Node::pushFrameNoReallocate(const uint32_t frameLength, const uint16_t byteAlign, void *const retValDst, const Bytecode *const instructionPointer)
{
    sy_assert(this->currentFrame.has_value(), "Expected this node to be in use");
    sy_assert(this->frameDepth > 0, "Expected frame depth");
    sy_assert(this->nextBaseOffset >= Frame::OLD_FRAME_INFO_RESERVED_SLOTS, "next base offset invalid value");
    sy_assert(instructionPointer != nullptr, "Cannot have null instruction pointer when previous frames exist");

    { // update next base offset and current frame for length
        const uint32_t newNextBaseOffset = requiredBaseOffsetForByteAlignment(this->nextBaseOffset, byteAlign);
        if(newNextBaseOffset >= this->slots) {
            return false;
        }

        // even if the frame cannot be pushed onto this node, increasing the previous frame's length and
        // updated the next base offset is safe.
        // increase frame length by the difference
        this->currentFrame.value().frameLength += newNextBaseOffset - this->nextBaseOffset;
        this->nextBaseOffset = newNextBaseOffset;
    }

    if(!this->hasEnoughSpaceForFrame(frameLength, byteAlign)) {
        return false;
    }

    // now we know that the frame can fit

    { // store old frame
        const uint32_t actualOffset = this->nextBaseOffset - Frame::OLD_FRAME_INFO_RESERVED_SLOTS; // no int overflow
        uint64_t* valuesMem = &this->values[actualOffset];
        uintptr_t* typesMem = &this->types[actualOffset];
        this->currentFrame.value().storeInMemory(valuesMem, typesMem, instructionPointer);
    }
    { // new frame
        Frame f{
            this->nextBaseOffset,
            frameLength,
            0, // TODO function index
            retValDst
        };
        this->currentFrame = f;
    }
    { // update base offset for after the new frame
        this->nextBaseOffset += frameLength + Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
    }
    { // set new frame depth
        this->frameDepth += 1;
    }

    return true;
}

void Node::pushFrameAllowReallocate(
    const uint32_t frameLength,
    const uint16_t byteAlign,
    void *const retValDst,
    std::optional<Frame*> previousFrame,
    const Bytecode *const instructionPointer
) {
    sy_assert(this->currentFrame.has_value() == false, "Expected this node to not be in use");
    sy_assert(this->frameDepth == 0, "Expected no frame depth");
    sy_assert(this->nextBaseOffset >= Frame::OLD_FRAME_INFO_RESERVED_SLOTS, "next base offset invalid value");
    sy_assert(previousFrame.has_value() == (instructionPointer != nullptr), 
        "If there is a previous frame, a valid instruction pointer is expected and vice versa");

    { // potential reallocation
        std::optional<uint32_t> optReallocSize = this->shouldReallocate(frameLength, byteAlign);
        if(optReallocSize.has_value()) {
            this->reallocate(optReallocSize.value());
        }
    }
    { // update next base offset
        const uint32_t newNextBaseOffset = requiredBaseOffsetForByteAlignment(this->nextBaseOffset, byteAlign);
        // `previousFrame` is from a different node so we do not have to increase it's frame length
        this->nextBaseOffset = newNextBaseOffset;
    }
    { // store old frame
        const uint32_t actualOffset = this->nextBaseOffset - Frame::OLD_FRAME_INFO_RESERVED_SLOTS; // no int overflow
        uint64_t* valuesMem = &this->values[actualOffset];
        uintptr_t* typesMem = &this->types[actualOffset];
        if(previousFrame.has_value()) {
            previousFrame.value()->storeInMemory(valuesMem, typesMem, instructionPointer);
        } else {
            Frame::storeNullFrameInMemory(valuesMem, typesMem);
        }
    }
    { // new frame
        Frame f{
            this->nextBaseOffset,
            frameLength,
            0, // TODO function index
            retValDst
        };
        this->currentFrame = f;
    }
    { // update base offset for after the new frame
        this->nextBaseOffset += frameLength + Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
    }
    { // set new frame depth
        this->frameDepth = 1;
    }
}

std::optional<std::tuple<Frame, const Bytecode *, bool>> Node::popFrame(
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
    const uint64_t* const valuesMem = &this->values[oldInfoStartOffset];
    const uintptr_t* const typesMem  = &this->types[oldInfoStartOffset];

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

std::optional<uint32_t> Node::shouldReallocate(uint32_t frameLength, uint16_t alignment) const
{
    // `alignment` must always be a power of 2
    // `frameLength` must be a multiple of alignment
    // Despite that, since Frame::OLD_FRAME_INFO_RESERVED_SLOTS are needed,
    // this may result in flawed alignment.
    // As a result, we double the input frame length so that even with the padding, we can still
    // always get correct alignment

    sy_assert(this->currentFrame.has_value() == false, "Expected this node to not be in use");
    sy_assert(frameLength <= Stack::MAX_FRAME_LEN, "Frame length cannot exceed the maximum");

    const size_t pageSize = page_size();
    sy_assert(alignment > 0, "Alignment must be non zero");
    sy_assert(alignment <= pageSize, "Alignment greater than page size does not make sense");
    #if _DEBUG
    uint32_t normalizeAlign = alignment / sizeof(size_t);
    if(normalizeAlign == 0) normalizeAlign = 1;
    sy_assert((frameLength % normalizeAlign) == 0, "Frame length must be a multiple of alignment");
    #endif
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

#ifdef SYNC_LIB_TEST

#include "../../doctest.h"

TEST_CASE("requiredBaseOffsetForByteAlignment") {
    { // 1 byte align from default next base offset
        uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS, 1);
        CHECK_EQ(res, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
    }
    { // 8 byte align from default next base offset
        uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS, 8);
        CHECK_EQ(res, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
    }
    { // 16 byte align (two slots) from default next base offset
        uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS, 16);
        CHECK_EQ(res, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
    }
    { // 64 byte align (two slots) from default next base offset
        uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS, 64);
        CHECK_EQ(res, 8);
    }
    { // 1 byte align from non-default but already aligned
        uint32_t res = requiredBaseOffsetForByteAlignment(32, 1);
        CHECK_EQ(res, 32);
    }
    { // 8 byte align from non-default but already aligned
        uint32_t res = requiredBaseOffsetForByteAlignment(32, 8);
        CHECK_EQ(res, 32);
    }
    { // 16 byte align (two slots) from non-default but already aligned
        uint32_t res = requiredBaseOffsetForByteAlignment(32, 16);
        CHECK_EQ(res, 32);
    }
    { // 64 byte align (two slots) from non-default but already aligned
        uint32_t res = requiredBaseOffsetForByteAlignment(32, 64);
        CHECK_EQ(res, 32);
    }
    { // 16 byte align (two slots) from non-default and not aligned
        uint32_t res = requiredBaseOffsetForByteAlignment(33, 16);
        CHECK_EQ(res, 34);
    }
    { // 64 byte align (two slots) from non-default and not aligned
        uint32_t res = requiredBaseOffsetForByteAlignment(33, 64);
        CHECK_EQ(res, 40);
    }
}

TEST_CASE("Construct destruct mininum") {
    auto node = Node(1);
    CHECK_EQ(node.slots, MIN_SLOTS);
}

TEST_CASE("Construct destruct exactly minimum") {
    auto node = Node(MIN_SLOTS);
    CHECK_EQ(node.slots, MIN_SLOTS);
}

TEST_CASE("Construct destruct one above minimum") {
    auto node = Node(MIN_SLOTS + 1);
    CHECK_GT(node.slots, MIN_SLOTS);
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

TEST_CASE("requiredBaseOffsetForByteAlignment") {
    { // 1 byte align from default next base offset
        uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS, 1);
        CHECK_EQ(res, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
    }
    { // 8 byte align from default next base offset
        uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS, 8);
        CHECK_EQ(res, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
    }
    { // 16 byte align (two slots) from default next base offset
        uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS, 16);
        CHECK_EQ(res, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
    }
    { // 64 byte align (two slots) from default next base offset
        uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS, 64);
        CHECK_EQ(res, 8);
    }
    { // 1 byte align from non-default but already aligned
        uint32_t res = requiredBaseOffsetForByteAlignment(32, 1);
        CHECK_EQ(res, 32);
    }
    { // 8 byte align from non-default but already aligned
        uint32_t res = requiredBaseOffsetForByteAlignment(32, 8);
        CHECK_EQ(res, 32);
    }
    { // 16 byte align (two slots) from non-default but already aligned
        uint32_t res = requiredBaseOffsetForByteAlignment(32, 16);
        CHECK_EQ(res, 32);
    }
    { // 64 byte align (two slots) from non-default but already aligned
        uint32_t res = requiredBaseOffsetForByteAlignment(32, 64);
        CHECK_EQ(res, 32);
    }
    { // 16 byte align (two slots) from non-default and not aligned
        uint32_t res = requiredBaseOffsetForByteAlignment(33, 16);
        CHECK_EQ(res, 34);
    }
    { // 64 byte align (two slots) from non-default and not aligned
        uint32_t res = requiredBaseOffsetForByteAlignment(33, 64);
        CHECK_EQ(res, 40);
    }
}

TEST_CASE("simple construct") {
    auto node = Node(1);
    CHECK_FALSE(node.isInUse());
    CHECK_FALSE(node.currentFrame.has_value());
    CHECK_EQ(node.frameDepth, 0);
    CHECK_EQ(node.nextBaseOffset, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
    CHECK_EQ(node.slots, MIN_SLOTS);
}

TEST_CASE("hasEnoughSpaceForFrame minimum") {
    auto node = Node(1);
    // 1 slot, 1 byte align
    CHECK(node.hasEnoughSpaceForFrame(1, 1));
    // 2 slot, 1 byte align
    CHECK(node.hasEnoughSpaceForFrame(2, 1));
    // 1 slot, pointer byte align
    CHECK(node.hasEnoughSpaceForFrame(1, alignof(void*)));
    // 1 slot, slot align
    CHECK(node.hasEnoughSpaceForFrame(1, alignof(uint64_t)));
    // 2 slot, pointer byte align
    CHECK(node.hasEnoughSpaceForFrame(2, alignof(void*)));
    // 2 slot, slot align
    CHECK(node.hasEnoughSpaceForFrame(2, alignof(uint64_t)));
    // 2 slot, 2 slot align
    CHECK(node.hasEnoughSpaceForFrame(2, 2 * alignof(uint64_t)));
    // node is at min slots so too big, 1 slot align
    CHECK_FALSE(node.hasEnoughSpaceForFrame(MIN_SLOTS, 1));
    // half min slots, 1 slot align
    CHECK(node.hasEnoughSpaceForFrame(MIN_SLOTS / 2, 1));
    // half min slots, half min slot align (uses 2 preceeding slots as reserve, so fits)
    CHECK(node.hasEnoughSpaceForFrame(MIN_SLOTS / 2, (MIN_SLOTS * alignof(uint64_t)) / 2));

    { // use up all slots
        const auto len = (MIN_SLOTS / 4) * 3;
        const auto align = len / 3;
        CHECK(node.hasEnoughSpaceForFrame(len, align));
    }

    // all slots excluding reserve
    CHECK(node.hasEnoughSpaceForFrame(node.slots - Frame::OLD_FRAME_INFO_RESERVED_SLOTS, 1));
    // not all slots but not enough for reserve
    CHECK_FALSE(node.hasEnoughSpaceForFrame(node.slots - 1, 1));
}

TEST_CASE("pushFrameAllowReallocate, no previous frame, non-special align") {
    { // doesn't reallocate at all
        auto node = Node(1);
        CHECK_FALSE(node.shouldReallocate(1, 1).has_value());
        node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
        CHECK(node.isInUse());
        CHECK(node.currentFrame.has_value());
        CHECK_EQ(node.currentFrame.value().basePointerOffset, 2);
        CHECK_EQ(node.currentFrame.value().frameLength, 1);
        CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
        CHECK_EQ(node.frameDepth, 1);
        CHECK_EQ(node.nextBaseOffset, 5); // 2 start + 1 frame length + 2 reserve slots
    }
    { // does reallocate        
        auto node = Node(1);
        CHECK(node.shouldReallocate(1024, 1).has_value());
        node.pushFrameAllowReallocate(1024, 1, nullptr, std::nullopt, nullptr);
        CHECK(node.isInUse());
        CHECK(node.currentFrame.has_value());
        CHECK_EQ(node.currentFrame.value().basePointerOffset, 2);
        CHECK_EQ(node.currentFrame.value().frameLength, 1024);
        CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
        CHECK_EQ(node.frameDepth, 1);
        CHECK_EQ(node.nextBaseOffset, 1028); // 2 start + 1024 frame length + 2 reserve slots
    }
}

TEST_CASE("pushFrameAllowReallocate, no previous frame, special align") {
    { // doesn't reallocate at all
        auto node = Node(1);
        CHECK_FALSE(node.shouldReallocate(4, 32).has_value());
        node.pushFrameAllowReallocate(4, 32, nullptr, std::nullopt, nullptr);
        CHECK(node.isInUse());
        CHECK(node.currentFrame.has_value());
        CHECK_EQ(node.currentFrame.value().basePointerOffset, 4);
        CHECK_EQ(node.currentFrame.value().frameLength, 4);
        CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
        CHECK_EQ(node.frameDepth, 1);
        CHECK_EQ(node.nextBaseOffset, 10); // 2 start + 2 bump for alignment + 4 frame length + 2 reserve slots
    }
    { // does reallocate        
        auto node = Node(1);
        CHECK(node.shouldReallocate(1024, 32).has_value());
        node.pushFrameAllowReallocate(1024, 32, nullptr, std::nullopt, nullptr);
        CHECK(node.isInUse());
        CHECK(node.currentFrame.has_value());
        CHECK_EQ(node.currentFrame.value().basePointerOffset, 4);
        CHECK_EQ(node.currentFrame.value().frameLength, 1024);
        CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
        CHECK_EQ(node.frameDepth, 1);
        CHECK_EQ(node.nextBaseOffset, 1030); // 2 start + 2 bump for alignment + 1024 frame length + 2 reserve slots
    }
}

TEST_CASE("pushFrameNoReallocate, successful, 2 total frames, non-special align") {
    Bytecode unusedBytecode;
    {
        auto node = Node(1);
        node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
        CHECK(node.pushFrameNoReallocate(1, 1, nullptr, &unusedBytecode));
        CHECK(node.isInUse());
        CHECK(node.currentFrame.has_value());
        CHECK_EQ(node.currentFrame.value().basePointerOffset, 6);
        CHECK_EQ(node.currentFrame.value().frameLength, 1);
        CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
        CHECK_EQ(node.frameDepth, 2);
        // 2 start + 1 old frame length + 1 extended for alignment + 2 reserve slots + 1 new frame length + 2 reserve
        CHECK_EQ(node.nextBaseOffset, 9); 
    }
    {
        auto node = Node(1);
        node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
        CHECK(node.pushFrameNoReallocate(4, 1, nullptr, &unusedBytecode));
        CHECK(node.isInUse());
        CHECK(node.currentFrame.has_value());
        CHECK_EQ(node.currentFrame.value().basePointerOffset, 6);
        CHECK_EQ(node.currentFrame.value().frameLength, 4);
        CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
        CHECK_EQ(node.frameDepth, 2);
        // 2 start + 1 old frame length + 1 extended for alignment + 2 reserve slots + 4 new frame length + 2 reserve
        CHECK_EQ(node.nextBaseOffset, 12); 
    }
    { // exactly enough
        auto node = Node(1);
        node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
        CHECK(node.pushFrameNoReallocate(Node::MIN_SLOTS - 6, 1, nullptr, &unusedBytecode));
        CHECK(node.isInUse());
        CHECK(node.currentFrame.has_value());
        CHECK_EQ(node.currentFrame.value().basePointerOffset, 6);
        CHECK_EQ(node.currentFrame.value().frameLength, Node::MIN_SLOTS - 6);
        CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
        CHECK_EQ(node.frameDepth, 2);
        // 2 start + 1 old frame length + 1 extended for alignment + 2 reserve slots + 4 new frame length + 2 reserve
        CHECK_GE(node.nextBaseOffset, node.slots); 
    }
}

TEST_CASE("pushFrameNoReallocate, successful, 2 total frames, special align") {
    Bytecode unusedBytecode;
    {
        auto node = Node(1);
        node.pushFrameAllowReallocate(8, 64, nullptr, std::nullopt, nullptr);
        CHECK(node.pushFrameNoReallocate(8, 64, nullptr, &unusedBytecode));
        CHECK(node.isInUse());
        CHECK(node.currentFrame.has_value());
        // 2 start + 6 extend old alignment + 8 old frame length + 2 reserve + 6 extended for new alignment
        CHECK_EQ(node.currentFrame.value().basePointerOffset, 24);
        CHECK_EQ(node.currentFrame.value().frameLength, 8);
        CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
        CHECK_EQ(node.frameDepth, 2);
        // 2 start + 6 extend old alignment + 8 old frame length + 2 reserve + 6 extended for new alignment
        // + 8 new length + 2 reserve
        CHECK_EQ(node.nextBaseOffset, 34); 
    }
    {
        auto node = Node(1);
        node.pushFrameAllowReallocate(8, 64, nullptr, std::nullopt, nullptr);
        CHECK(node.pushFrameNoReallocate(32, 64, nullptr, &unusedBytecode));
        CHECK(node.isInUse());
        CHECK(node.currentFrame.has_value());
        // 2 start + 6 extend old alignment + 8 old frame length + 2 reserve + 6 extended for new alignment
        CHECK_EQ(node.currentFrame.value().basePointerOffset, 24);
        CHECK_EQ(node.currentFrame.value().frameLength, 32);
        CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
        CHECK_EQ(node.frameDepth, 2);
        // 2 start + 6 extend old alignment + 8 old frame length + 2 reserve + 6 extended for new alignment
        // + 32 new length + 2 reserve
        CHECK_EQ(node.nextBaseOffset, 58); 
    }
}

TEST_CASE("pushFrameNoReallocate, not successful, non-special align") {
    Bytecode unusedBytecode;
    {
        auto node = Node(1);
        node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
        CHECK_FALSE(node.pushFrameNoReallocate(node.slots, 1, nullptr, &unusedBytecode));

        // retains all old data
        CHECK(node.isInUse());
        CHECK(node.currentFrame.has_value());
        CHECK_EQ(node.currentFrame.value().basePointerOffset, 2);
        CHECK_EQ(node.currentFrame.value().frameLength, 2);
        CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
        CHECK_EQ(node.frameDepth, 1);
        // despite mostly not doing anything, the frame is still extended (maybe this is stupid)
        CHECK_EQ(node.nextBaseOffset, 6); // 2 start + 1 frame length + 1 extended for alignment + 2 reserve slots
    }
    {
        auto node = Node(1);
        node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
        CHECK_FALSE(node.pushFrameNoReallocate(node.slots, 1, nullptr, &unusedBytecode));

        // retains all old data
        // despite mostly not doing anything, the frame is still extended (maybe this is stupid)
        CHECK(node.isInUse());
        CHECK(node.currentFrame.has_value());
        CHECK_EQ(node.currentFrame.value().basePointerOffset, 2);
        CHECK_EQ(node.currentFrame.value().frameLength, 2);
        CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
        CHECK_EQ(node.frameDepth, 1);
        CHECK_EQ(node.nextBaseOffset, 6); // 2 start + 1 frame length + 1 extended for alignment + 2 reserve slots
    }
    { // not quite enough
        auto node = Node(1);
        node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
        CHECK_FALSE(node.pushFrameNoReallocate(Node::MIN_SLOTS - 5, 1, nullptr, &unusedBytecode));

        CHECK(node.isInUse());
        CHECK(node.currentFrame.has_value());
        CHECK_EQ(node.currentFrame.value().basePointerOffset, 2);
        CHECK_EQ(node.currentFrame.value().frameLength, 2);
        CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
        CHECK_EQ(node.frameDepth, 1);
        CHECK_EQ(node.nextBaseOffset, 6); // 2 start + 1 frame length + 1 extended for alignment + 2 reserve slots
    }
}

TEST_CASE("pushFrameNoReallocate, not successful, special align") {
    Bytecode unusedBytecode;
    { // cannot fit frame because of length
        auto node = Node(1);
        node.pushFrameAllowReallocate(8, 64, nullptr, std::nullopt, nullptr);
        CHECK_FALSE(node.pushFrameNoReallocate(node.slots, 64, nullptr, &unusedBytecode));

        // retains all old data
        CHECK(node.isInUse());
        CHECK(node.currentFrame.has_value());
        CHECK_EQ(node.currentFrame.value().basePointerOffset, 8);
        CHECK_EQ(node.currentFrame.value().frameLength, 14); // extended even though failed
        CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
        CHECK_EQ(node.frameDepth, 1);
        // 2 start + 6 extend old alignment + 8 old frame length
        // + 2 reserve + 6 extended for new alignment (even though failed)
        CHECK_EQ(node.nextBaseOffset, 24);
    }
    { // cannot fit frame because of length and align (need more than 2 frames in order for alignment alone to prevent)
        auto node = Node(1);
        node.pushFrameAllowReallocate(8, 64, nullptr, std::nullopt, nullptr);
        CHECK_FALSE(node.pushFrameNoReallocate(
            Node::MIN_SLOTS,
            (Node::MIN_SLOTS) * alignof(uint64_t), 
            nullptr, 
            &unusedBytecode
        ));

        // retains all old data
        // despite mostly not doing anything, the frame is still extended (maybe this is stupid)
        CHECK(node.isInUse());
        CHECK(node.currentFrame.has_value());
        CHECK_EQ(node.currentFrame.value().basePointerOffset, 8);
        CHECK_EQ(node.currentFrame.value().frameLength, 8);
        CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
        CHECK_EQ(node.frameDepth, 1);
        CHECK_EQ(node.nextBaseOffset, 18); // 2 start + 1 frame length + 1 extended for alignment + 2 reserve slots
    }
}

TEST_CASE("push 3 frames, successful, non-special align") {
    Bytecode unusedBytecode;
    auto node = Node(1);
    node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
    CHECK(node.pushFrameNoReallocate(1, 1, nullptr, &unusedBytecode));
    CHECK(node.pushFrameNoReallocate(1, 1, nullptr, &unusedBytecode));
    
    CHECK(node.isInUse());
    CHECK(node.currentFrame.has_value());
    CHECK_EQ(node.currentFrame.value().basePointerOffset, 10);
    CHECK_EQ(node.currentFrame.value().frameLength, 1);
    CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
    CHECK_EQ(node.frameDepth, 3);
    CHECK_EQ(node.nextBaseOffset, 13); 
}

TEST_CASE("push 3 frames, successful, special align") {
    Bytecode unusedBytecode;
    auto node = Node(1);
    node.pushFrameAllowReallocate(8, 64, nullptr, std::nullopt, nullptr);
    CHECK(node.pushFrameNoReallocate(8, 64, nullptr, &unusedBytecode));
    CHECK(node.pushFrameNoReallocate(8, 64, nullptr, &unusedBytecode));

    CHECK(node.isInUse());
    CHECK(node.currentFrame.has_value());
    CHECK_EQ(node.currentFrame.value().basePointerOffset, 40);
    CHECK_EQ(node.currentFrame.value().frameLength, 8);
    CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
    CHECK_EQ(node.frameDepth, 3);
    CHECK_EQ(node.nextBaseOffset, 50); 
}

TEST_CASE("push 3 frames, not successful, non-special align") {
    Bytecode unusedBytecode;
    auto node = Node(1);
    node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
    CHECK(node.pushFrameNoReallocate(1, 1, nullptr, &unusedBytecode));
    CHECK_FALSE(node.pushFrameNoReallocate(Node::MIN_SLOTS, 1, nullptr, &unusedBytecode));

    CHECK(node.isInUse());
    CHECK(node.currentFrame.has_value());
    CHECK_EQ(node.currentFrame.value().basePointerOffset, 6);
    CHECK_EQ(node.currentFrame.value().frameLength, 2);
    CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
    CHECK_EQ(node.frameDepth, 2);
    CHECK_EQ(node.nextBaseOffset, 10); 
}

TEST_CASE("push 3 frames, not successful, special align") {
    Bytecode unusedBytecode;
    auto node = Node(1);
    node.pushFrameAllowReallocate(8, 64, nullptr, std::nullopt, nullptr);
    CHECK(node.pushFrameNoReallocate(8, 64, nullptr, &unusedBytecode));
    CHECK_FALSE(node.pushFrameNoReallocate(Node::MIN_SLOTS, 64, nullptr, &unusedBytecode));

    CHECK(node.isInUse());
    CHECK(node.currentFrame.has_value());
    CHECK_EQ(node.currentFrame.value().basePointerOffset, 24);
    CHECK_EQ(node.currentFrame.value().frameLength, 14);
    CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
    CHECK_EQ(node.frameDepth, 2);
    CHECK_EQ(node.nextBaseOffset, 40); 
}

#endif

