#include "node.hpp"
#include "../../util/assert.hpp"
#include "stack.hpp"
#include "../../threading/alloc_cache_align.hpp"
#include "../../mem/os_mem.hpp"
#include "../../mem/allocator.hpp"
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
    if ((this->nextBaseOffset + frameLength + Frame::OLD_FRAME_INFO_RESERVED_SLOTS) > this->slots) {
        return false;
    }
    return true;
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

bool Node::pushFrame(uint32_t frameLength, uint16_t byteAlign, void *retValDst, std::optional<Frame *> previousFrame, const Bytecode *instructionPointer)
{
    sy_assert(previousFrame.has_value() == (instructionPointer != nullptr), 
        "If there is a previous frame, a valid instruction pointer is expected and vice versa");

    if(this->currentFrame.has_value()) {
        sy_assert(previousFrame.has_value(), "Expected there to be a previous frame if this node is in use");
        Frame& prevFrame = *previousFrame.value();
        Frame& currFrame = this->currentFrame.value();
        const char* message = "Expected previous frame to be current frame";
        sy_assert(prevFrame.basePointerOffset   == currFrame.basePointerOffset, message);
        sy_assert(prevFrame.frameLength         == currFrame.frameLength, message);
        sy_assert(prevFrame.functionIndex       == currFrame.functionIndex, message);
        sy_assert(prevFrame.retValueDst         == currFrame.retValueDst, message);

        return this->pushFrameNoReallocate(frameLength, byteAlign, retValDst, instructionPointer);
    } else {
        this->pushFrameAllowReallocate(frameLength, byteAlign, retValDst, previousFrame, instructionPointer);
        return true;
    }
}

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

TEST_CASE_FIXTURE(Node, "example") {

}

#endif

