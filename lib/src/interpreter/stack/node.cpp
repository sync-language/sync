#include "node.hpp"
#include "../../core/core_internal.h"
#include "../../mem/allocator.hpp"
#include "../../mem/os_mem.hpp"
#include "../../threading/alloc_cache_align.hpp"
#include "../../types/type.hpp"
#include "../../util/pow_of_2.hpp"
#include "../bytecode.hpp"
#include "stack.hpp"

#if defined(_MSC_VER) || defined(_WIN32)
#include <new>
#elif __GNUC__
#include <sys/mman.h>
#endif
#include <cstring>

using namespace sy;

static_assert(sizeof(StackValueSlot) == 16);
static_assert(alignof(StackValueSlot) == 16);
static_assert(sizeof(StackTypeSlot) == 16);
static_assert(alignof(StackTypeSlot) == 16);

static_assert(sizeof(StackTypeSlot) >= sizeof(Type));
static_assert(alignof(StackTypeSlot) >= alignof(Type));

sy::StackTypeSlot::StackTypeSlot(Type type, bool owned) noexcept
    : indirection_(type.indirection), mutableBits_(type.mutableBits) {
    static_assert(offsetof(StackTypeSlot, baseMask_) == offsetof(Type, base));
    static_assert(offsetof(StackTypeSlot, indirection_) == offsetof(Type, indirection));
    static_assert(offsetof(StackTypeSlot, mutableBits_) == offsetof(Type, mutableBits));

    this->set(type, owned);
}

Option<Type> sy::StackTypeSlot::get() const noexcept {
    const uintptr_t maskedAwayFlag = this->baseMask_ & (~TYPE_OWNED_FLAG);
    const Type t{.base = reinterpret_cast<const sy::TypeMetadata*>(maskedAwayFlag),
                 .indirection = this->indirection_,
                 .mutableBits = this->mutableBits_};
    if (t.base == nullptr) {
        return {};
    }
    return t;
}

bool sy::StackTypeSlot::isOwned() const noexcept {
    const uintptr_t maskedAwayType = this->baseMask_ & TYPE_OWNED_FLAG;
    return maskedAwayType == TYPE_OWNED_FLAG;
}

void sy::StackTypeSlot::set(Type type, bool owned) noexcept {
    const uintptr_t typeMask = reinterpret_cast<uintptr_t>(type.base);
    sy_assert((typeMask & TYPE_OWNED_FLAG) == 0,
              "Type metadata pointer must be at least 2 byte aligned to reserve the low bit");
    this->baseMask_ = typeMask | static_cast<uintptr_t>(owned ? TYPE_OWNED_FLAG : 0);
    this->indirection_ = type.indirection;
    this->mutableBits_ = type.mutableBits;
}

Result<Node, AllocErr> sy::Node::init(uint32_t minSlotSize, Allocator allocator) noexcept {
    Node self;
    self.alloc = allocator;

    auto allocateWithAllocator = [&self, &allocator](uint32_t slots,
                                                     size_t align) -> Result<void, AllocErr> {
        auto valuesRes = allocator.allocAlignedArray<StackValueSlot>(slots, align);
        if (valuesRes.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        auto valuesAllocated = valuesRes.takeValue();

        auto typesRes = allocator.allocAlignedArray<StackTypeSlot>(slots, align);
        if (typesRes.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        auto typesAllocated = typesRes.takeValue();

        self.values = valuesAllocated.take();
        self.types = typesAllocated.take();
        self.slots = slots;
        return {};
    };

    if (minSlotSize <= MIN_SLOTS) {
        auto res = allocateWithAllocator(MIN_SLOTS, ALLOC_CACHE_ALIGN);
        if (res.hasErr()) {
            return Error(res.err());
        }
    } else {
#if SYNC_NO_PAGES
        auto res = allocateWithAllocator(minSlotSize, ALLOC_CACHE_ALIGN);
        if (res.hasErr()) {
            return Error(res.err());
        }
#else
        const size_t pageSize = page_size();

        size_t valuesBytesToAllocate = static_cast<size_t>(minSlotSize) * sizeof(StackValueSlot);
        {
            const size_t remainder = valuesBytesToAllocate % pageSize;
            if (remainder != 0) {
                valuesBytesToAllocate += pageSize - remainder;
            }
        }
        size_t typesBytesToAllocate = static_cast<size_t>(minSlotSize) * sizeof(StackTypeSlot);
        {
            const size_t remainder = typesBytesToAllocate % pageSize;
            if (remainder != 0) {
                typesBytesToAllocate += pageSize - remainder;
            }
        }

        void* valuesMem = page_malloc(valuesBytesToAllocate);
        if (valuesMem == nullptr) {
            return Error(AllocErr::OutOfMemory);
        }

        void* typesMem = page_malloc(typesBytesToAllocate);
        if (typesMem == nullptr) {
            page_free(valuesMem, valuesBytesToAllocate);
            return Error(AllocErr::OutOfMemory);
        }

        self.values = static_cast<StackValueSlot*>(valuesMem);
        self.types = static_cast<StackTypeSlot*>(typesMem);
        self.slots = valuesBytesToAllocate / sizeof(StackValueSlot); // fine for types too
#endif
    }

    return self;
}

sy::Node::~Node() noexcept {
    sy_assert(this->frameDepth == 0, "Cannot destroy a Node that still has live frames");

    if (this->slots == 0)
        return;

    auto freeWithAllocator = [this]() {
        this->alloc.freeAlignedArray(this->values, this->slots, ALLOC_CACHE_ALIGN);
        this->alloc.freeAlignedArray(this->types, this->slots, ALLOC_CACHE_ALIGN);
    };

    if (slots == MIN_SLOTS) {
        freeWithAllocator();
    } else {

#if SYNC_NO_PAGES
        freeWithAllocator();
#else
        page_free(this->values, this->slots * sizeof(StackValueSlot));
        page_free(this->types, this->slots * sizeof(StackTypeSlot));
#endif
    }

    this->values = nullptr;
    this->types = nullptr;
    this->slots = 0;
    this->nextBaseOffset = 0;
}

sy::Node::Node(Node&& other) noexcept
    : alloc(other.alloc), values(other.values), types(other.types), slots(other.slots),
      nextBaseOffset(other.nextBaseOffset), currentFrame(std::move(other.currentFrame)),
      frameDepth(other.frameDepth) {
    other.values = nullptr;
    other.types = nullptr;
    other.slots = 0;
    other.nextBaseOffset = 0;
    other.currentFrame = std::nullopt;
    other.frameDepth = 0;
}

bool sy::Node::hasFrames() const {
    sy_assert(this->currentFrame.hasValue() == (this->frameDepth != 0), "Invalid state");
    return this->frameDepth != 0;
}

/// @param initialNextBaseOffset Offset including the slots required to store frame info.
static uint32_t nextBaseOffsetForAlignment(uint32_t initialNextBaseOffset,
                                           uint16_t alignment) noexcept {
    sy_assert(isPowOf2(alignment), "`alignment` must be a power of 2");

    if (alignment <= alignof(StackValueSlot)) {
        return initialNextBaseOffset;
    }

    const uint32_t normalizedAlign = alignment / alignof(StackValueSlot);
    const uint32_t remainder = initialNextBaseOffset % normalizedAlign; // TODO can probably bit-and
    if (remainder == 0) {
        return initialNextBaseOffset;
    }
    return initialNextBaseOffset + (normalizedAlign - remainder);
}

bool sy::Node::hasEnoughSpaceForFrame(uint16_t frameLength, uint16_t alignment) const noexcept {
    const uint32_t nextBaseOffsetForAlign =
        nextBaseOffsetForAlignment(this->nextBaseOffset, alignment);
    if ((nextBaseOffsetForAlign + static_cast<uint32_t>(frameLength) +
         Frame::OLD_FRAME_INFO_RESERVED_SLOTS) > this->slots) {
        return false;
    }
    return true;
}

static void writeFrameAndInstructionPointer(StackValueSlot* valueSlot, StackTypeSlot* typesSlot,
                                            Frame frame, const Bytecode* instructionPointer) {
    static_assert(sizeof(StackValueSlot) >= (sizeof(uintptr_t) * 2));
    static_assert(alignof(StackValueSlot) >= alignof(uintptr_t));
    static_assert(sizeof(StackTypeSlot) >=
                  (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t)));

    uintptr_t* pointersStore = reinterpret_cast<uintptr_t*>(valueSlot);
    pointersStore[0] = reinterpret_cast<uintptr_t>(frame.retValueDst);
    pointersStore[1] = reinterpret_cast<uintptr_t>(instructionPointer);

    uint32_t* basePointerOffsetStore = reinterpret_cast<uint32_t*>(typesSlot);
    uint16_t* frameLengthStore = reinterpret_cast<uint16_t*>(&basePointerOffsetStore[1]);
    uint16_t* functionIndexStore = &frameLengthStore[1];

    *basePointerOffsetStore = frame.basePointerOffset;
    *frameLengthStore = frame.frameLength;
    *functionIndexStore = frame.functionIndex;
}

static FrameInstructionPair readFrameAndInstructionPointer(const StackValueSlot* valueSlot,
                                                           const StackTypeSlot* typesSlot) {
    static_assert(sizeof(StackValueSlot) >= (sizeof(uintptr_t) * 2));
    static_assert(alignof(StackValueSlot) >= alignof(uintptr_t));
    static_assert(sizeof(StackTypeSlot) >=
                  (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t)));

    const uintptr_t* pointersStore = reinterpret_cast<const uintptr_t*>(valueSlot);
    void* retValueDst = const_cast<void*>(reinterpret_cast<const void*>(pointersStore[0]));
    const Bytecode* instructionPointer = reinterpret_cast<const Bytecode*>(pointersStore[1]);

    const uint32_t* basePointerOffsetStore = reinterpret_cast<const uint32_t*>(typesSlot);
    const uint16_t* frameLengthStore =
        reinterpret_cast<const uint16_t*>(&basePointerOffsetStore[1]);
    const uint16_t* functionIndexStore = &frameLengthStore[1];

    Frame frame;
    frame.retValueDst = retValueDst;
    frame.basePointerOffset = *basePointerOffsetStore;
    frame.frameLength = *frameLengthStore;
    frame.functionIndex = *functionIndexStore;

    FrameInstructionPair pair;
    pair.frame = frame;
    pair.instructionPointer = instructionPointer;
    return pair;
}

Result<void, Node::PushFailure>
sy::Node::pushFrame(uint16_t frameLength, uint16_t requiredByteAlign, void* retValDst,
                    Option<FrameInstructionPair> previous) noexcept {
    // TODO what happens when a native function calls the interpreter that already has a frame?
    // TODO then `previous` would be empty??
    if (!hasEnoughSpaceForFrame(frameLength, requiredByteAlign)) {
        if (this->hasFrames()) {
            return Error(Node::PushFailure::TooBigAndHasFrames);
        }
        return Error(Node::PushFailure::CanReconstructBigger);
    }

    // `nextBaseOffsetForAlignment` works in slot indices, which only produces a correctly
    // byte-aligned address when the allocation's base is itself at least `requiredByteAlign`
    // aligned. Allocations are only guaranteed to `ALLOC_CACHE_ALIGN` (or page alignment), so a
    // higher request cannot be satisfied and the Stack must allocate a suitably aligned Node.
    sy_assert((reinterpret_cast<uintptr_t>(this->values) % requiredByteAlign) == 0 &&
                  (reinterpret_cast<uintptr_t>(this->types) % requiredByteAlign) == 0,
              "Node allocation is not aligned enough for the requested frame alignment");

    const uint32_t thisActualBaseOffset =
        nextBaseOffsetForAlignment(this->nextBaseOffset, requiredByteAlign);

    const uint32_t newNextBaseOffset = nextBaseOffsetForAlignment(
        thisActualBaseOffset + frameLength + Frame::OLD_FRAME_INFO_RESERVED_SLOTS,
        requiredByteAlign);

    const uint32_t previousFrameWriteOffset =
        thisActualBaseOffset - Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
    if (previous.hasValue()) {
        { // store old frame
            const FrameInstructionPair previousFrameIP = previous.take();
            sy_assert(previousFrameIP.instructionPointer != nullptr,
                      "Must have a previous instruction pointer");
#ifndef NDEBUG
            if (this->hasFrames()) {
                const Frame thisCurrentFrame = this->currentFrame.value();
                sy_assert(previousFrameIP.frame.basePointerOffset ==
                              thisCurrentFrame.basePointerOffset,
                          "`previous` should have been this Node's previous frame");
                sy_assert(previousFrameIP.frame.frameLength == thisCurrentFrame.frameLength,
                          "`previous` should have been this Node's previous frame");
                sy_assert(previousFrameIP.frame.functionIndex == thisCurrentFrame.functionIndex,
                          "`previous` should have been this Node's previous frame");
                sy_assert(previousFrameIP.frame.retValueDst == thisCurrentFrame.retValueDst,
                          "`previous` should have been this Node's previous frame");
            }
#endif

            writeFrameAndInstructionPointer(
                &this->values[previousFrameWriteOffset], &this->types[previousFrameWriteOffset],
                previousFrameIP.frame, previousFrameIP.instructionPointer);
        }
    } else {
        writeFrameAndInstructionPointer(&this->values[previousFrameWriteOffset],
                                        &this->types[previousFrameWriteOffset], Frame{}, nullptr);
    }

    std::memset(&this->types[thisActualBaseOffset], 0,
                static_cast<size_t>(frameLength) * sizeof(StackTypeSlot));

    { // new frame
        Frame frame;
        frame.retValueDst = retValDst;
        frame.frameLength = frameLength;
        frame.functionIndex = 0; // TODO should this be set here?
        frame.basePointerOffset = thisActualBaseOffset;
        this->currentFrame = Option<Frame>(frame);
        this->frameDepth += 1;
    }

    this->nextBaseOffset = newNextBaseOffset;

    return {};
}

Option<FrameInstructionPair> sy::Node::popFrame() noexcept {
    sy_assert(this->hasFrames(), "No frames to pop");

    const Frame currFrame = this->currentFrame.value();
    const size_t oldInfoStartOffset =
        currFrame.basePointerOffset - Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
    // it's safe to get the previous even if there wasn't. Just check if stuff is null.
    const FrameInstructionPair oldPair = readFrameAndInstructionPointer(
        &this->values[oldInfoStartOffset], &this->types[oldInfoStartOffset]);
    this->frameDepth -= 1;
    this->nextBaseOffset = currFrame.basePointerOffset;
    if (this->frameDepth == 0) {
        this->currentFrame = {};
        if (oldPair.instructionPointer !=
            nullptr) { // TODO pushFrame from native code must figure out
            return oldPair;
        }
        return {};
    }

    this->currentFrame = Option<Frame>(oldPair.frame);
    return oldPair;
}

Result<uint16_t, Node::PushFailure>
sy::Node::pushScriptFunctionArg(void* argMem, Type type, uint16_t offset, uint16_t frameLength,
                                uint16_t frameByteAlign) noexcept {
    if (!hasEnoughSpaceForFrame(frameLength, frameByteAlign)) {
        if (this->hasFrames()) {
            return Error(Node::PushFailure::TooBigAndHasFrames);
        }
        return Error(Node::PushFailure::CanReconstructBigger);
    }

    sy_assert((reinterpret_cast<uintptr_t>(this->values) % frameByteAlign) == 0 &&
                  (reinterpret_cast<uintptr_t>(this->types) % frameByteAlign) == 0,
              "Node allocation is not aligned enough for this frame alignment");
    // TODO hmmm... what happens if it's way more alignment than ALLOC_CACHE_ALIGN...

    // PRESUMABLY, `frameByteAlign` is consistent across calls to this function to push all the
    // arguments a function needs. AS SUCH, the multiple updates to `this->nextBaseOffset` will
    // write the same value multiple times. This means any previous calls to this won't need to
    // have the actual arguments shifted to a new location.
    this->nextBaseOffset = nextBaseOffsetForAlignment(this->nextBaseOffset, frameByteAlign);

    const uint32_t actualOffset = [this, offset, type]() -> uint32_t {
        const uint32_t initialOffset = this->nextBaseOffset + static_cast<uint32_t>(offset);
        if (type.isReference()) {
            return initialOffset;
        } else {
            if (type.base->typeAlign <= alignof(StackValueSlot)) {
                return initialOffset;
            }
            const uint32_t normalizedTypeAlign = type.base->typeAlign / alignof(StackValueSlot);

            const uint32_t remainder = initialOffset % normalizedTypeAlign;
            if (remainder == 0) {
                return initialOffset;
            }

            return initialOffset + (normalizedTypeAlign - remainder);
        }
    }();

    {
        StackValueSlot* valueMem = &this->values[actualOffset];
        const size_t size = type.isReference() ? sizeof(void*) : type.base->typeSize;
        memcpy(valueMem, argMem, size);
    }

    { // do the type and clear
        StackTypeSlot* typeMem = &this->types[actualOffset];
        if (!type.isReference()) {
            const uint32_t slotsOccupied =
                static_cast<uint32_t>((type.base->typeSize - 1) / sizeof(StackValueSlot)) + 1;
            sy_assert((static_cast<uint32_t>(offset) + slotsOccupied) <= frameLength,
                      "Cannot set type information past the frame length");
            for (size_t i = 1; i < slotsOccupied; i++) {
                typeMem[i] = StackTypeSlot(); // empty
            }
        }

        *typeMem = StackTypeSlot(type, true);
    }

    const uint16_t newOffset = [this, actualOffset, type]() -> uint16_t {
        uint16_t initialOffset = static_cast<uint16_t>((actualOffset - this->nextBaseOffset) + 1);
        if (type.isReference()) {
            return initialOffset;
        }
        return initialOffset +
               static_cast<uint16_t>((type.base->typeSize - 1) / sizeof(StackValueSlot));
    }();

    return newOffset;
}

StackTypeSlot sy::Node::typeAt(uint16_t offset) const {
    this->ensureOffsetWithinFrameBounds(offset);
    const auto frame = this->currentFrame.value();
    const uint32_t actualOffset = frame.basePointerOffset + static_cast<uint32_t>(offset);
    return this->types[actualOffset];
}

void sy::Node::setTypeAt(StackTypeSlot type, uint16_t offset) {
    this->ensureOffsetWithinFrameBounds(offset);
    const auto frame = this->currentFrame.value();
    const uint32_t actualOffset = frame.basePointerOffset + static_cast<uint32_t>(offset);

    if (auto typeRes = type.get(); typeRes.hasValue()) {
        Type t = typeRes.value();
        if (!t.isReference()) {
            const uint32_t slotsOccupied =
                static_cast<uint32_t>((t.base->typeSize - 1) / sizeof(StackTypeSlot)) + 1;
            sy_assert((static_cast<uint32_t>(offset) + slotsOccupied) <= frame.frameLength,
                      "Cannot set type information past the frame length");
            for (size_t i = 1; i < slotsOccupied; i++) {
                this->types[actualOffset + i] = StackTypeSlot(); // empty
            }
        }
    }

    this->types[actualOffset] = type;
}

void sy::Node::setFrameFunction(uint16_t functionIndex) {
    sy_assert(this->hasFrames(), "Expected node to have a frame");
    this->currentFrame.value().functionIndex = functionIndex;
}

void sy::Node::ensureOffsetWithinFrameBounds(uint16_t offset) const {
    sy_assert(this->currentFrame.hasValue(), "No frame");
    sy_assert(offset < this->currentFrame.value().frameLength,
              "Index out of bounds for stack frame");
    (void)offset;
}

// bool Node::pushFrame(uint32_t frameLength, uint16_t byteAlign, void *retValDst,
// std::optional<Frame *> previousFrame, const Bytecode *instructionPointer)
// {
//     sy_assert(previousFrame.has_value() == (instructionPointer != nullptr),
//         "If there is a previous frame, a valid instruction pointer is expected and vice versa");

//     if(this->currentFrame.has_value()) {
//         sy_assert(previousFrame.has_value(), "Expected there to be a previous frame if this node
//         is in use"); Frame& prevFrame = *previousFrame.value(); Frame& currFrame =
//         this->currentFrame.value(); const char* message = "Expected previous frame to be current
//         frame"; sy_assert(prevFrame.basePointerOffset   == currFrame.basePointerOffset, message);
//         sy_assert(prevFrame.frameLength         == currFrame.frameLength, message);
//         sy_assert(prevFrame.functionIndex       == currFrame.functionIndex, message);
//         sy_assert(prevFrame.retValueDst         == currFrame.retValueDst, message);

//         return this->pushFrameNoReallocate(frameLength, byteAlign, retValDst,
//         instructionPointer);
//     } else {
//         this->pushFrameAllowReallocate(frameLength, byteAlign, retValDst, previousFrame,
//         instructionPointer); return true;
//     }
// }

// bool sy::Node::pushFrameNoReallocate(const uint16_t frameLength, const uint16_t byteAlign,
//                                      void* const retValDst,
//                                      const Bytecode* const instructionPointer) {
//     sy_assert(this->currentFrame.has_value(), "Expected this node to be in use");
//     sy_assert(this->frameDepth > 0, "Expected frame depth");
//     sy_assert(this->nextBaseOffset >= Frame::OLD_FRAME_INFO_RESERVED_SLOTS,
//               "next base offset invalid value");
//     sy_assert(instructionPointer != nullptr,
//               "Cannot have null instruction pointer when previous frames exist");

//     { // update next base offset and current frame for length
//         const uint32_t newNextBaseOffset =
//             requiredBaseOffsetForByteAlignment(this->nextBaseOffset, byteAlign);
//         if (newNextBaseOffset >= this->slots) {
//             return false;
//         }

//         // even if the frame cannot be pushed onto this node, increasing the previous frame's
//         length
//         // and updated the next base offset is safe. increase frame length by the difference
//         const uint32_t newFrameLength =
//             static_cast<uint32_t>(this->currentFrame.value().frameLength) +
//             (newNextBaseOffset - this->nextBaseOffset);
//         sy_assert(newFrameLength < Stack::MAX_FRAME_LEN,
//                   "Align extended frame length exceeds maximum");
//         this->currentFrame.value().frameLength = static_cast<uint16_t>(newFrameLength);
//         this->nextBaseOffset = newNextBaseOffset;
//     }

//     if (!this->hasEnoughSpaceForFrame(frameLength, byteAlign)) {
//         return false;
//     }

//     // now we know that the frame can fit

//     { // store old frame
//         const uint32_t actualOffset =
//             this->nextBaseOffset - Frame::OLD_FRAME_INFO_RESERVED_SLOTS; // no int overflow
//         uint64_t* valuesMem = &this->values[actualOffset];
//         uintptr_t* typesMem = reinterpret_cast<uintptr_t*>(&this->types[actualOffset]);
//         this->currentFrame.value().storeInMemory(valuesMem, typesMem, instructionPointer);
//     }
//     { // new frame
//         Frame f{};
//         f.basePointerOffset = this->nextBaseOffset;
//         f.frameLength = frameLength;
//         f.functionIndex = 0; // TODO function index
//         f.retValueDst = retValDst;
//         this->currentFrame = f;
//     }
//     { // update base offset for after the new frame
//         this->nextBaseOffset += frameLength + Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
//     }
//     { // set new frame depth
//         this->frameDepth += 1;
//     }

//     return true;
// }

// void sy::Node::pushFrameAllowReallocate(const uint16_t frameLength, const uint16_t byteAlign,
//                                         void* const retValDst, std::optional<Frame>
//                                         previousFrame, const Bytecode* const instructionPointer)
//                                         {
//     sy_assert(this->currentFrame.has_value() == false, "Expected this node to not be in use");
//     sy_assert(this->frameDepth == 0, "Expected no frame depth");
//     sy_assert(this->nextBaseOffset >= Frame::OLD_FRAME_INFO_RESERVED_SLOTS,
//               "next base offset invalid value");
//     sy_assert(
//         previousFrame.has_value() == (instructionPointer != nullptr),
//         "If there is a previous frame, a valid instruction pointer is expected and vice versa");

//     { // potential reallocation
//         std::optional<uint32_t> optReallocSize = this->shouldReallocate(frameLength, byteAlign);
//         if (optReallocSize.has_value()) {
//             this->reallocate(optReallocSize.value());
//         }
//     }
//     { // update next base offset
//         const uint32_t newNextBaseOffset =
//             requiredBaseOffsetForByteAlignment(this->nextBaseOffset, byteAlign);
//         // `previousFrame` is from a different node so we do not have to increase it's frame
//         length this->nextBaseOffset = newNextBaseOffset;
//     }
//     { // store old frame
//         const uint32_t actualOffset =
//             this->nextBaseOffset - Frame::OLD_FRAME_INFO_RESERVED_SLOTS; // no int overflow
//         uint64_t* valuesMem = &this->values[actualOffset];
//         uintptr_t* typesMem = reinterpret_cast<uintptr_t*>(&this->types[actualOffset]);
//         if (previousFrame.has_value()) {
//             previousFrame.value().storeInMemory(valuesMem, typesMem, instructionPointer);
//         } else {
//             Frame::storeNullFrameInMemory(valuesMem, typesMem);
//         }
//     }
//     { // new frame
//         Frame f{};
//         f.basePointerOffset = this->nextBaseOffset;
//         f.frameLength = frameLength;
//         f.functionIndex = 0; // TODO function index
//         f.retValueDst = retValDst;
//         this->currentFrame = f;
//     }
//     { // update base offset for after the new frame
//         this->nextBaseOffset += frameLength + Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
//     }
//     { // set new frame depth
//         this->frameDepth = 1;
//     }
// }

// std::optional<uint16_t> sy::Node::pushScriptFunctionArg(const void* argMem, const sy::Type* type,
//                                                         uint16_t offset, const uint16_t
//                                                         frameLength, const uint16_t
//                                                         frameByteAlign) {
//     sy_assert(argMem != nullptr, "Expected valid argument memory");
//     sy_assert(type != nullptr, "Expected valid type memory");
//     const uint16_t normalizedAlign =
//         frameByteAlign < (2 * alignof(uint64_t)) ? (2 * alignof(uint64_t)) : frameByteAlign;
//     sy_assert(type->alignType <= normalizedAlign, "Type alignment exceeds frame alignment");
//     (void)normalizedAlign;

//     if (this->isInUse()) { // update next base offset and current frame for length
//         const uint32_t newNextBaseOffset =
//             requiredBaseOffsetForByteAlignment(this->nextBaseOffset, frameByteAlign);
//         if (newNextBaseOffset >= this->slots) {
//             return std::nullopt;
//         }

//         // even if the frame cannot be pushed onto this node, increasing the previous frame's
//         length
//         // and updated the next base offset is safe. increase frame length by the difference
//         const uint32_t newFrameLength =
//             static_cast<uint32_t>(this->currentFrame.value().frameLength) +
//             (newNextBaseOffset - this->nextBaseOffset);
//         sy_assert(newFrameLength < Stack::MAX_FRAME_LEN,
//                   "Align extended frame length exceeds maximum");
//         this->currentFrame.value().frameLength = static_cast<uint16_t>(newFrameLength);
//         this->nextBaseOffset = newNextBaseOffset;

//         if (!this->hasEnoughSpaceForFrame(frameLength, frameByteAlign)) {
//             return std::nullopt;
//         }
//     } else {
//         { // potential reallocation
//             std::optional<uint32_t> optReallocSize =
//                 this->shouldReallocate(frameLength, frameByteAlign);
//             if (optReallocSize.has_value()) {
//                 this->reallocate(optReallocSize.value());
//             }
//         }
//         { // update next base offset
//             const uint32_t newNextBaseOffset =
//                 requiredBaseOffsetForByteAlignment(this->nextBaseOffset, frameByteAlign);
//             // `previousFrame` is from a different node so we do not have to increase it's frame
//             // length
//             this->nextBaseOffset = newNextBaseOffset;
//         }
//     }

//     const uint32_t actualOffset = [this, offset, type]() -> uint32_t {
//         const uint32_t normalizedTypeAlign = ((type->alignType - 1) / 8) + 1;
//         const uint32_t initialOffset = this->nextBaseOffset + static_cast<uint32_t>(offset);
//         const uint32_t remainder = initialOffset % normalizedTypeAlign;
//         if (remainder == 0) {
//             return initialOffset;
//         }

//         return initialOffset + (this->nextBaseOffset - remainder);
//     }();

//     { // ensure fits within frame
//         const uint32_t extraTypeSlots = static_cast<uint32_t>((type->sizeType - 1) / 8);
//         if (((actualOffset + extraTypeSlots)) - this->nextBaseOffset >= frameLength) {
//             return std::nullopt;
//         }
//     }

//     // guaranteed that all arguments will fit even with their alignment requirements

//     uint64_t* valueMem = &this->values[actualOffset];
//     TypeOfValue* typesMem = &this->types[actualOffset];
//     memcpy(valueMem, argMem, type->sizeType);
//     typesMem->set(type, true);

//     const uint16_t newOffset = [this, actualOffset, type]() -> uint16_t {
//         // add one because the argument must occupy at least one slot
//         uint16_t initialOffset = static_cast<uint16_t>((actualOffset - this->nextBaseOffset) +
//         1);
//         // if size is 16, occupies 2 slots, so this math makes it only do 1 iteration
//         for (size_t i = 0; i < ((type->sizeType - 1) / 8); i++) {
//             initialOffset += 1;
//         }
//         return initialOffset;
//     }();

//     return std::optional(newOffset);
// }

// std::optional<uint32_t> sy::Node::shouldReallocate(uint16_t frameLength, uint16_t alignment)
// const {
//     // `alignment` must always be a power of 2
//     // `frameLength` must be a multiple of alignment
//     // Despite that, since Frame::OLD_FRAME_INFO_RESERVED_SLOTS are needed,
//     // this may result in flawed alignment.
//     // As a result, we double the input frame length so that even with the padding, we can still
//     // always get correct alignment

//     sy_assert(this->currentFrame.has_value() == false, "Expected this node to not be in use");
//     sy_assert(frameLength <= Stack::MAX_FRAME_LEN, "Frame length cannot exceed the maximum");

//     const size_t pageSize = page_size();
//     sy_assert(alignment > 0, "Alignment must be non zero");
//     sy_assert(alignment <= pageSize, "Alignment greater than page size does not make sense");
//     (void)pageSize;
// #ifndef NDEBUG
//     uint32_t normalizeAlign =
//         alignment / sizeof(uint64_t); // not size_t because the stack values occupy 64 bits
//     if (normalizeAlign == 0)
//         normalizeAlign = 1;
//     sy_assert((frameLength % normalizeAlign) == 0, "Frame length must be a multiple of
//     alignment");
// #endif
//     sy_assert((alignment & (alignment - 1)) == 0, "Alignment must be a power of 2");

//     const uint32_t minRequiredSlots = (frameLength * 2) + Frame::OLD_FRAME_INFO_RESERVED_SLOTS;
//     uint32_t reallocationSlots = alignment;
//     while (reallocationSlots < minRequiredSlots) {
//         reallocationSlots <<= 1;
//     }

//     if (this->slots < reallocationSlots) {
//         return std::optional<uint32_t>(reallocationSlots);
//     }

//     if (reinterpret_cast<size_t>(this->values) % alignment != 0) {
//         return std::optional<uint32_t>(
//             reallocationSlots); // the allocation itself is not aligned enough
//     }

//     return std::nullopt;
// }

// #if SYNC_LIB_WITH_TESTS

// #include "../../doctest.h"

// TEST_CASE("requiredBaseOffsetForByteAlignment") {
//     { // 1 byte align from default next base offset
//         uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS,
//         1); CHECK_EQ(res, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
//     }
//     { // 8 byte align from default next base offset
//         uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS,
//         8); CHECK_EQ(res, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
//     }
//     { // 16 byte align (two slots) from default next base offset
//         uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS,
//         16); CHECK_EQ(res, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
//     }
//     { // 64 byte align (two slots) from default next base offset
//         uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS,
//         64); CHECK_EQ(res, 8);
//     }
//     { // 1 byte align from non-default but already aligned
//         uint32_t res = requiredBaseOffsetForByteAlignment(32, 1);
//         CHECK_EQ(res, 32);
//     }
//     { // 8 byte align from non-default but already aligned
//         uint32_t res = requiredBaseOffsetForByteAlignment(32, 8);
//         CHECK_EQ(res, 32);
//     }
//     { // 16 byte align (two slots) from non-default but already aligned
//         uint32_t res = requiredBaseOffsetForByteAlignment(32, 16);
//         CHECK_EQ(res, 32);
//     }
//     { // 64 byte align (two slots) from non-default but already aligned
//         uint32_t res = requiredBaseOffsetForByteAlignment(32, 64);
//         CHECK_EQ(res, 32);
//     }
//     { // 16 byte align (two slots) from non-default and not aligned
//         uint32_t res = requiredBaseOffsetForByteAlignment(33, 16);
//         CHECK_EQ(res, 34);
//     }
//     { // 64 byte align (two slots) from non-default and not aligned
//         uint32_t res = requiredBaseOffsetForByteAlignment(33, 64);
//         CHECK_EQ(res, 40);
//     }
// }

// TEST_CASE("Construct destruct mininum") {
//     auto node = Node(1);
//     CHECK_EQ(node.slots, MIN_SLOTS);
// }

// TEST_CASE("Construct destruct exactly minimum") {
//     auto node = Node(MIN_SLOTS);
//     CHECK_EQ(node.slots, MIN_SLOTS);
// }

// TEST_CASE("Construct destruct one above minimum") {
//     auto node = Node(MIN_SLOTS + 1);
//     CHECK_GT(node.slots, MIN_SLOTS);
// }

// TEST_CASE("Node reallocate bigger") {
//     auto node = Node(1);
//     node.reallocate(Node::MIN_SLOTS + 1);
//     CHECK_GT(node.slots, Node::MIN_SLOTS);
// }

// TEST_CASE("Node reallocate smaller") {
//     auto node = Node(Node::MIN_SLOTS + 1);
//     node.reallocate(1);
//     CHECK_EQ(node.slots, Node::MIN_SLOTS);
// }

// TEST_CASE("Node should reallocate true with simple alignment") {
//     auto node = Node(4);
//     auto result = node.shouldReallocate(static_cast<uint16_t>(node.slots) + 1, alignof(size_t));
//     CHECK(result.has_value());
//     CHECK_GT(result.value(), node.slots);
// }

// TEST_CASE("Node should reallocate false with simple alignment") {
//     auto node = Node(4);
//     auto result = node.shouldReallocate(8, alignof(size_t));
//     CHECK_FALSE(result.has_value());
// }

// TEST_CASE("Node should reallocate true with alignment same as frame length") {
//     auto node = Node(4);
//     const uint16_t lenAlign = 1024;
//     auto result = node.shouldReallocate(lenAlign, lenAlign);
//     CHECK(result.has_value());
//     CHECK_GT(result.value(), node.slots);
// }

// TEST_CASE("Node should reallocate false with alignment same as frame length") {
//     auto node = Node(4096);
//     const uint16_t lenAlign = 1024;
//     auto result = node.shouldReallocate(lenAlign, lenAlign);
//     CHECK_FALSE(result.has_value());
// }

// TEST_CASE("requiredBaseOffsetForByteAlignment") {
//     { // 1 byte align from default next base offset
//         uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS,
//         1); CHECK_EQ(res, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
//     }
//     { // 8 byte align from default next base offset
//         uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS,
//         8); CHECK_EQ(res, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
//     }
//     { // 16 byte align (two slots) from default next base offset
//         uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS,
//         16); CHECK_EQ(res, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
//     }
//     { // 64 byte align (two slots) from default next base offset
//         uint32_t res = requiredBaseOffsetForByteAlignment(Frame::OLD_FRAME_INFO_RESERVED_SLOTS,
//         64); CHECK_EQ(res, 8);
//     }
//     { // 1 byte align from non-default but already aligned
//         uint32_t res = requiredBaseOffsetForByteAlignment(32, 1);
//         CHECK_EQ(res, 32);
//     }
//     { // 8 byte align from non-default but already aligned
//         uint32_t res = requiredBaseOffsetForByteAlignment(32, 8);
//         CHECK_EQ(res, 32);
//     }
//     { // 16 byte align (two slots) from non-default but already aligned
//         uint32_t res = requiredBaseOffsetForByteAlignment(32, 16);
//         CHECK_EQ(res, 32);
//     }
//     { // 64 byte align (two slots) from non-default but already aligned
//         uint32_t res = requiredBaseOffsetForByteAlignment(32, 64);
//         CHECK_EQ(res, 32);
//     }
//     { // 16 byte align (two slots) from non-default and not aligned
//         uint32_t res = requiredBaseOffsetForByteAlignment(33, 16);
//         CHECK_EQ(res, 34);
//     }
//     { // 64 byte align (two slots) from non-default and not aligned
//         uint32_t res = requiredBaseOffsetForByteAlignment(33, 64);
//         CHECK_EQ(res, 40);
//     }
// }

// TEST_CASE("simple construct") {
//     auto node = Node(1);
//     CHECK_FALSE(node.isInUse());
//     CHECK_FALSE(node.currentFrame.has_value());
//     CHECK_EQ(node.frameDepth, 0);
//     CHECK_EQ(node.nextBaseOffset, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
//     CHECK_EQ(node.slots, MIN_SLOTS);
// }

// TEST_CASE("hasEnoughSpaceForFrame minimum") {
//     auto node = Node(1);
//     // 1 slot, 1 byte align
//     CHECK(node.hasEnoughSpaceForFrame(1, 1));
//     // 2 slot, 1 byte align
//     CHECK(node.hasEnoughSpaceForFrame(2, 1));
//     // 1 slot, pointer byte align
//     CHECK(node.hasEnoughSpaceForFrame(1, alignof(void*)));
//     // 1 slot, slot align
//     CHECK(node.hasEnoughSpaceForFrame(1, alignof(uint64_t)));
//     // 2 slot, pointer byte align
//     CHECK(node.hasEnoughSpaceForFrame(2, alignof(void*)));
//     // 2 slot, slot align
//     CHECK(node.hasEnoughSpaceForFrame(2, alignof(uint64_t)));
//     // 2 slot, 2 slot align
//     CHECK(node.hasEnoughSpaceForFrame(2, 2 * alignof(uint64_t)));
//     // node is at min slots so too big, 1 slot align
//     CHECK_FALSE(node.hasEnoughSpaceForFrame(MIN_SLOTS, 1));
//     // half min slots, 1 slot align
//     CHECK(node.hasEnoughSpaceForFrame(MIN_SLOTS / 2, 1));
//     // half min slots, half min slot align (uses 2 preceeding slots as reserve, so fits)
//     CHECK(node.hasEnoughSpaceForFrame(MIN_SLOTS / 2, (MIN_SLOTS * alignof(uint64_t)) / 2));

//     { // use up all slots
//         const auto len = (MIN_SLOTS / 4) * 3;
//         const auto align = len / 3;
//         CHECK(node.hasEnoughSpaceForFrame(len, align));
//     }

//     // all slots excluding reserve
//     CHECK(node.hasEnoughSpaceForFrame(
//         static_cast<uint16_t>(node.slots - Frame::OLD_FRAME_INFO_RESERVED_SLOTS), 1));
//     // not all slots but not enough for reserve
//     CHECK_FALSE(node.hasEnoughSpaceForFrame(static_cast<uint16_t>(node.slots - 1), 1));
// }

// TEST_CASE("pushFrameAllowReallocate, no previous frame, non-special align") {
//     { // doesn't reallocate at all
//         auto node = Node(1);
//         CHECK_FALSE(node.shouldReallocate(1, 1).has_value());
//         node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
//         CHECK(node.isInUse());
//         CHECK(node.currentFrame.has_value());
//         CHECK_EQ(node.currentFrame.value().basePointerOffset, 2);
//         CHECK_EQ(node.currentFrame.value().frameLength, 1);
//         CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//         CHECK_EQ(node.frameDepth, 1);
//         CHECK_EQ(node.nextBaseOffset, 5); // 2 start + 1 frame length + 2 reserve slots
//     }
//     { // does reallocate
//         auto node = Node(1);
//         CHECK(node.shouldReallocate(1024, 1).has_value());
//         node.pushFrameAllowReallocate(1024, 1, nullptr, std::nullopt, nullptr);
//         CHECK(node.isInUse());
//         CHECK(node.currentFrame.has_value());
//         CHECK_EQ(node.currentFrame.value().basePointerOffset, 2);
//         CHECK_EQ(node.currentFrame.value().frameLength, 1024);
//         CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//         CHECK_EQ(node.frameDepth, 1);
//         CHECK_EQ(node.nextBaseOffset, 1028); // 2 start + 1024 frame length + 2 reserve slots
//     }
// }

// TEST_CASE("pushFrameAllowReallocate, no previous frame, special align") {
//     { // doesn't reallocate at all
//         auto node = Node(1);
//         CHECK_FALSE(node.shouldReallocate(4, 32).has_value());
//         node.pushFrameAllowReallocate(4, 32, nullptr, std::nullopt, nullptr);
//         CHECK(node.isInUse());
//         CHECK(node.currentFrame.has_value());
//         CHECK_EQ(node.currentFrame.value().basePointerOffset, 4);
//         CHECK_EQ(node.currentFrame.value().frameLength, 4);
//         CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//         CHECK_EQ(node.frameDepth, 1);
//         CHECK_EQ(node.nextBaseOffset,
//                  10); // 2 start + 2 bump for alignment + 4 frame length + 2 reserve slots
//     }
//     { // does reallocate
//         auto node = Node(1);
//         CHECK(node.shouldReallocate(1024, 32).has_value());
//         node.pushFrameAllowReallocate(1024, 32, nullptr, std::nullopt, nullptr);
//         CHECK(node.isInUse());
//         CHECK(node.currentFrame.has_value());
//         CHECK_EQ(node.currentFrame.value().basePointerOffset, 4);
//         CHECK_EQ(node.currentFrame.value().frameLength, 1024);
//         CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//         CHECK_EQ(node.frameDepth, 1);
//         CHECK_EQ(node.nextBaseOffset,
//                  1030); // 2 start + 2 bump for alignment + 1024 frame length + 2 reserve slots
//     }
// }

// TEST_CASE("pushFrameNoReallocate, successful, 2 total frames, non-special align") {
//     Bytecode unusedBytecode;
//     {
//         auto node = Node(1);
//         node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
//         CHECK(node.pushFrameNoReallocate(1, 1, nullptr, &unusedBytecode));
//         CHECK(node.isInUse());
//         CHECK(node.currentFrame.has_value());
//         CHECK_EQ(node.currentFrame.value().basePointerOffset, 6);
//         CHECK_EQ(node.currentFrame.value().frameLength, 1);
//         CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//         CHECK_EQ(node.frameDepth, 2);
//         // 2 start + 1 old frame length + 1 extended for alignment + 2 reserve slots + 1 new
//         frame
//         // length + 2 reserve
//         CHECK_EQ(node.nextBaseOffset, 9);
//     }
//     {
//         auto node = Node(1);
//         node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
//         CHECK(node.pushFrameNoReallocate(4, 1, nullptr, &unusedBytecode));
//         CHECK(node.isInUse());
//         CHECK(node.currentFrame.has_value());
//         CHECK_EQ(node.currentFrame.value().basePointerOffset, 6);
//         CHECK_EQ(node.currentFrame.value().frameLength, 4);
//         CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//         CHECK_EQ(node.frameDepth, 2);
//         // 2 start + 1 old frame length + 1 extended for alignment + 2 reserve slots + 4 new
//         frame
//         // length + 2 reserve
//         CHECK_EQ(node.nextBaseOffset, 12);
//     }
//     { // exactly enough
//         auto node = Node(1);
//         node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
//         CHECK(node.pushFrameNoReallocate(Node::MIN_SLOTS - 6, 1, nullptr, &unusedBytecode));
//         CHECK(node.isInUse());
//         CHECK(node.currentFrame.has_value());
//         CHECK_EQ(node.currentFrame.value().basePointerOffset, 6);
//         CHECK_EQ(node.currentFrame.value().frameLength, Node::MIN_SLOTS - 6);
//         CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//         CHECK_EQ(node.frameDepth, 2);
//         // 2 start + 1 old frame length + 1 extended for alignment + 2 reserve slots + 4 new
//         frame
//         // length + 2 reserve
//         CHECK_GE(node.nextBaseOffset, node.slots);
//     }
// }

// TEST_CASE("pushFrameNoReallocate, successful, 2 total frames, special align") {
//     Bytecode unusedBytecode;
//     {
//         auto node = Node(1);
//         node.pushFrameAllowReallocate(8, 64, nullptr, std::nullopt, nullptr);
//         CHECK(node.pushFrameNoReallocate(8, 64, nullptr, &unusedBytecode));
//         CHECK(node.isInUse());
//         CHECK(node.currentFrame.has_value());
//         // 2 start + 6 extend old alignment + 8 old frame length + 2 reserve + 6 extended for new
//         // alignment
//         CHECK_EQ(node.currentFrame.value().basePointerOffset, 24);
//         CHECK_EQ(node.currentFrame.value().frameLength, 8);
//         CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//         CHECK_EQ(node.frameDepth, 2);
//         // 2 start + 6 extend old alignment + 8 old frame length + 2 reserve + 6 extended for new
//         // alignment
//         // + 8 new length + 2 reserve
//         CHECK_EQ(node.nextBaseOffset, 34);
//     }
//     {
//         auto node = Node(1);
//         node.pushFrameAllowReallocate(8, 64, nullptr, std::nullopt, nullptr);
//         CHECK(node.pushFrameNoReallocate(32, 64, nullptr, &unusedBytecode));
//         CHECK(node.isInUse());
//         CHECK(node.currentFrame.has_value());
//         // 2 start + 6 extend old alignment + 8 old frame length + 2 reserve + 6 extended for new
//         // alignment
//         CHECK_EQ(node.currentFrame.value().basePointerOffset, 24);
//         CHECK_EQ(node.currentFrame.value().frameLength, 32);
//         CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//         CHECK_EQ(node.frameDepth, 2);
//         // 2 start + 6 extend old alignment + 8 old frame length + 2 reserve + 6 extended for new
//         // alignment
//         // + 32 new length + 2 reserve
//         CHECK_EQ(node.nextBaseOffset, 58);
//     }
// }

// TEST_CASE("pushFrameNoReallocate, not successful, non-special align") {
//     Bytecode unusedBytecode;
//     {
//         auto node = Node(1);
//         node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
//         CHECK_FALSE(node.pushFrameNoReallocate(static_cast<uint16_t>(node.slots), 1, nullptr,
//                                                &unusedBytecode));

//         // retains all old data
//         CHECK(node.isInUse());
//         CHECK(node.currentFrame.has_value());
//         CHECK_EQ(node.currentFrame.value().basePointerOffset, 2);
//         CHECK_EQ(node.currentFrame.value().frameLength, 2);
//         CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//         CHECK_EQ(node.frameDepth, 1);
//         // despite mostly not doing anything, the frame is still extended (maybe this is stupid)
//         CHECK_EQ(node.nextBaseOffset,
//                  6); // 2 start + 1 frame length + 1 extended for alignment + 2 reserve slots
//     }
//     {
//         auto node = Node(1);
//         node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
//         CHECK_FALSE(node.pushFrameNoReallocate(static_cast<uint16_t>(node.slots), 1, nullptr,
//                                                &unusedBytecode));

//         // retains all old data
//         // despite mostly not doing anything, the frame is still extended (maybe this is stupid)
//         CHECK(node.isInUse());
//         CHECK(node.currentFrame.has_value());
//         CHECK_EQ(node.currentFrame.value().basePointerOffset, 2);
//         CHECK_EQ(node.currentFrame.value().frameLength, 2);
//         CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//         CHECK_EQ(node.frameDepth, 1);
//         CHECK_EQ(node.nextBaseOffset,
//                  6); // 2 start + 1 frame length + 1 extended for alignment + 2 reserve slots
//     }
//     { // not quite enough
//         auto node = Node(1);
//         node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
//         CHECK_FALSE(node.pushFrameNoReallocate(Node::MIN_SLOTS - 5, 1, nullptr,
//         &unusedBytecode));

//         CHECK(node.isInUse());
//         CHECK(node.currentFrame.has_value());
//         CHECK_EQ(node.currentFrame.value().basePointerOffset, 2);
//         CHECK_EQ(node.currentFrame.value().frameLength, 2);
//         CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//         CHECK_EQ(node.frameDepth, 1);
//         CHECK_EQ(node.nextBaseOffset,
//                  6); // 2 start + 1 frame length + 1 extended for alignment + 2 reserve slots
//     }
// }

// TEST_CASE("pushFrameNoReallocate, not successful, special align") {
//     Bytecode unusedBytecode;
//     { // cannot fit frame because of length
//         auto node = Node(1);
//         node.pushFrameAllowReallocate(8, 64, nullptr, std::nullopt, nullptr);
//         CHECK_FALSE(node.pushFrameNoReallocate(static_cast<uint16_t>(node.slots), 64, nullptr,
//                                                &unusedBytecode));

//         // retains all old data
//         CHECK(node.isInUse());
//         CHECK(node.currentFrame.has_value());
//         CHECK_EQ(node.currentFrame.value().basePointerOffset, 8);
//         CHECK_EQ(node.currentFrame.value().frameLength, 14); // extended even though failed
//         CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//         CHECK_EQ(node.frameDepth, 1);
//         // 2 start + 6 extend old alignment + 8 old frame length
//         // + 2 reserve + 6 extended for new alignment (even though failed)
//         CHECK_EQ(node.nextBaseOffset, 24);
//     }
//     { // cannot fit frame because of length and align (need more than 2 frames in order for
//       // alignment alone to prevent)
//         auto node = Node(1);
//         node.pushFrameAllowReallocate(8, 64, nullptr, std::nullopt, nullptr);
//         CHECK_FALSE(node.pushFrameNoReallocate(
//             Node::MIN_SLOTS, (Node::MIN_SLOTS) * alignof(uint64_t), nullptr, &unusedBytecode));

//         // retains all old data
//         // despite mostly not doing anything, the frame is still extended (maybe this is stupid)
//         CHECK(node.isInUse());
//         CHECK(node.currentFrame.has_value());
//         CHECK_EQ(node.currentFrame.value().basePointerOffset, 8);
//         CHECK_EQ(node.currentFrame.value().frameLength, 8);
//         CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//         CHECK_EQ(node.frameDepth, 1);
//         CHECK_EQ(node.nextBaseOffset,
//                  18); // 2 start + 1 frame length + 1 extended for alignment + 2 reserve slots
//     }
// }

// TEST_CASE("push 3 frames, successful, non-special align") {
//     Bytecode unusedBytecode;
//     auto node = Node(1);
//     node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
//     CHECK(node.pushFrameNoReallocate(1, 1, nullptr, &unusedBytecode));
//     CHECK(node.pushFrameNoReallocate(1, 1, nullptr, &unusedBytecode));

//     CHECK(node.isInUse());
//     CHECK(node.currentFrame.has_value());
//     CHECK_EQ(node.currentFrame.value().basePointerOffset, 10);
//     CHECK_EQ(node.currentFrame.value().frameLength, 1);
//     CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//     CHECK_EQ(node.frameDepth, 3);
//     CHECK_EQ(node.nextBaseOffset, 13);
// }

// TEST_CASE("push 3 frames, successful, special align") {
//     Bytecode unusedBytecode;
//     auto node = Node(1);
//     node.pushFrameAllowReallocate(8, 64, nullptr, std::nullopt, nullptr);
//     CHECK(node.pushFrameNoReallocate(8, 64, nullptr, &unusedBytecode));
//     CHECK(node.pushFrameNoReallocate(8, 64, nullptr, &unusedBytecode));

//     CHECK(node.isInUse());
//     CHECK(node.currentFrame.has_value());
//     CHECK_EQ(node.currentFrame.value().basePointerOffset, 40);
//     CHECK_EQ(node.currentFrame.value().frameLength, 8);
//     CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//     CHECK_EQ(node.frameDepth, 3);
//     CHECK_EQ(node.nextBaseOffset, 50);
// }

// TEST_CASE("push 3 frames, not successful, non-special align") {
//     Bytecode unusedBytecode;
//     auto node = Node(1);
//     node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
//     CHECK(node.pushFrameNoReallocate(1, 1, nullptr, &unusedBytecode));
//     CHECK_FALSE(node.pushFrameNoReallocate(Node::MIN_SLOTS, 1, nullptr, &unusedBytecode));

//     CHECK(node.isInUse());
//     CHECK(node.currentFrame.has_value());
//     CHECK_EQ(node.currentFrame.value().basePointerOffset, 6);
//     CHECK_EQ(node.currentFrame.value().frameLength, 2);
//     CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//     CHECK_EQ(node.frameDepth, 2);
//     CHECK_EQ(node.nextBaseOffset, 10);
// }

// TEST_CASE("push 3 frames, not successful, special align") {
//     Bytecode unusedBytecode;
//     auto node = Node(1);
//     node.pushFrameAllowReallocate(8, 64, nullptr, std::nullopt, nullptr);
//     CHECK(node.pushFrameNoReallocate(8, 64, nullptr, &unusedBytecode));
//     CHECK_FALSE(node.pushFrameNoReallocate(Node::MIN_SLOTS, 64, nullptr, &unusedBytecode));

//     CHECK(node.isInUse());
//     CHECK(node.currentFrame.has_value());
//     CHECK_EQ(node.currentFrame.value().basePointerOffset, 24);
//     CHECK_EQ(node.currentFrame.value().frameLength, 14);
//     CHECK_EQ(node.currentFrame.value().retValueDst, nullptr);
//     CHECK_EQ(node.frameDepth, 2);
//     CHECK_EQ(node.nextBaseOffset, 40);
// }

// TEST_CASE("pop become unused") {
//     auto node = Node(1);
//     node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
//     CHECK_FALSE(node.popFrame().has_value());
//     CHECK_FALSE(node.isInUse());
//     CHECK_FALSE(node.currentFrame.has_value());
//     CHECK_EQ(node.frameDepth, 0);
//     CHECK_EQ(node.nextBaseOffset, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
// }

// TEST_CASE("push 2 pop 1 success") {
//     auto node = Node(1);
//     node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
//     Bytecode bytecode;
//     CHECK(node.pushFrameNoReallocate(1, 1, nullptr, &bytecode));
//     auto result = node.popFrame();

//     CHECK(node.isInUse());
//     CHECK_EQ(node.frameDepth, 1);
//     CHECK(result.has_value());
//     const Frame oldFrame = std::get<0>(result.value());
//     const Bytecode* oldIp = std::get<1>(result.value());
//     CHECK_EQ(oldIp, &bytecode);
//     CHECK_EQ(oldFrame.basePointerOffset, node.currentFrame.value().basePointerOffset);
//     CHECK_EQ(oldFrame.frameLength, node.currentFrame.value().frameLength);
//     CHECK_EQ(oldFrame.functionIndex, node.currentFrame.value().functionIndex);
//     CHECK_EQ(oldFrame.retValueDst, node.currentFrame.value().retValueDst);
// }

// TEST_CASE("push 2 pop 2 no more frames") {
//     auto node = Node(1);
//     node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
//     Bytecode bytecode;
//     CHECK(node.pushFrameNoReallocate(1, 1, nullptr, &bytecode));
//     CHECK(node.popFrame().has_value());
//     CHECK_FALSE(node.popFrame().has_value());

//     CHECK_FALSE(node.isInUse());
//     CHECK_FALSE(node.currentFrame.has_value());
//     CHECK_EQ(node.frameDepth, 0);
//     CHECK_EQ(node.nextBaseOffset, Frame::OLD_FRAME_INFO_RESERVED_SLOTS);
// }

// TEST_CASE("push frame from previous node") {
//     auto node1 = Node(1);
//     auto node2 = Node(1);

//     Bytecode bytecode;
//     node1.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);
//     node2.pushFrameAllowReallocate(1, 1, nullptr, node1.currentFrame, &bytecode);

//     auto result = node2.popFrame();

//     CHECK_FALSE(node2.isInUse());
//     CHECK(node1.isInUse());
//     CHECK(result.has_value());
//     const Frame oldFrame = std::get<0>(result.value());
//     const Bytecode* oldIp = std::get<1>(result.value());
//     CHECK_EQ(oldIp, &bytecode);
//     CHECK_EQ(oldFrame.basePointerOffset, node1.currentFrame.value().basePointerOffset);
//     CHECK_EQ(oldFrame.frameLength, node1.currentFrame.value().frameLength);
//     CHECK_EQ(oldFrame.functionIndex, node1.currentFrame.value().functionIndex);
//     CHECK_EQ(oldFrame.retValueDst, node1.currentFrame.value().retValueDst);
// }

// struct TestArgClass2Slot {
//     uint64_t v[2];
// };

// struct TestArgClass3Slot {
//     uint64_t v[3];
// };

// struct TestArgClass4Slot {
//     uint64_t v[4];
// };

// struct alignas(16) TestAlignedArgClass2Slot {
//     uint64_t v[2];
// };

// struct alignas(32) TestAlignedArgClass4Slot {
//     uint64_t v[4];
// };

// template <typename T> static sy::Type makeSimpleType() {
//     sy::Type t;
//     t.sizeType = sizeof(T);
//     t.alignType = alignof(T);
//     return t;
// }

// TEST_CASE("push 1 script function arg, 1 slot, non-special align, no frames") {
//     auto node = Node(1);
//     const int64_t arg = 10;
//     CHECK(node.pushScriptFunctionArg(&arg, sy::Type::TYPE_I64, 0, 1, 1));
//     node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);

//     const auto argMemLocation =
//         node.currentFrame.value().basePointerOffset; // argument is at offset 0
//     const int64_t* argMem = reinterpret_cast<const int64_t*>(&node.values[argMemLocation]);
//     CHECK_EQ(*argMem, arg);
//     CHECK_EQ(node.types[argMemLocation], sy::Type::TYPE_I64);
// }

// TEST_CASE("push 1 script function arg, 2 slot, non-special align, no frames") {
//     auto node = Node(1);
//     const TestArgClass2Slot arg = {{1, 2}};
//     const sy::Type type = makeSimpleType<decltype(arg)>();
//     CHECK(node.pushScriptFunctionArg(&arg, &type, 0, 2, 1));
//     node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);

//     const auto argMemLocation =
//         node.currentFrame.value().basePointerOffset; // argument is at offset 0
//     const TestArgClass2Slot* argMem =
//         reinterpret_cast<const TestArgClass2Slot*>(&node.values[argMemLocation]);
//     CHECK_EQ(argMem->v[0], 1);
//     CHECK_EQ(argMem->v[1], 2);
//     CHECK_EQ(node.types[argMemLocation], &type);
// }

// TEST_CASE("push 1 script function arg, 3 slot, non-special align, no frames") {
//     auto node = Node(1);
//     const TestArgClass3Slot arg = {{1, 2, 3}};
//     const sy::Type type = makeSimpleType<decltype(arg)>();
//     CHECK(node.pushScriptFunctionArg(&arg, &type, 0, 3, 1));
//     node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);

//     const auto argMemLocation =
//         node.currentFrame.value().basePointerOffset; // argument is at offset 0
//     const TestArgClass3Slot* argMem =
//         reinterpret_cast<const TestArgClass3Slot*>(&node.values[argMemLocation]);
//     CHECK_EQ(argMem->v[0], 1);
//     CHECK_EQ(argMem->v[1], 2);
//     CHECK_EQ(argMem->v[2], 3);
//     CHECK_EQ(node.types[argMemLocation], &type);
// }

// TEST_CASE("push 1 script function arg, 4 slot, non-special align, no frames") {
//     auto node = Node(1);
//     const TestArgClass4Slot arg = {{1, 2, 3, 4}};
//     const sy::Type type = makeSimpleType<decltype(arg)>();
//     CHECK(node.pushScriptFunctionArg(&arg, &type, 0, 4, 1));
//     node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);

//     const auto argMemLocation =
//         node.currentFrame.value().basePointerOffset; // argument is at offset 0
//     const TestArgClass4Slot* argMem =
//         reinterpret_cast<const TestArgClass4Slot*>(&node.values[argMemLocation]);
//     CHECK_EQ(argMem->v[0], 1);
//     CHECK_EQ(argMem->v[1], 2);
//     CHECK_EQ(argMem->v[2], 3);
//     CHECK_EQ(argMem->v[3], 4);
//     CHECK_EQ(node.types[argMemLocation], &type);
// }

// TEST_CASE("push 1 script function arg, 2 slot, special align, no frames") {
//     auto node = Node(1);
//     const TestAlignedArgClass2Slot arg = {{1, 2}};
//     const sy::Type type = makeSimpleType<decltype(arg)>();
//     CHECK(node.pushScriptFunctionArg(&arg, &type, 0, 2, 16));
//     node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);

//     const auto argMemLocation =
//         node.currentFrame.value().basePointerOffset; // argument is at offset 0
//     const TestAlignedArgClass2Slot* argMem =
//         reinterpret_cast<const TestAlignedArgClass2Slot*>(&node.values[argMemLocation]);
//     CHECK_EQ(argMem->v[0], 1);
//     CHECK_EQ(argMem->v[1], 2);
//     CHECK_EQ(node.types[argMemLocation], &type);
// }

// TEST_CASE("push 1 script function arg, 4 slot, special align, no frames") {
//     auto node = Node(1);
//     const TestAlignedArgClass4Slot arg = {{1, 2, 3, 4}};
//     const sy::Type type = makeSimpleType<decltype(arg)>();
//     CHECK(node.pushScriptFunctionArg(&arg, &type, 0, 4, 32));
//     node.pushFrameAllowReallocate(1, 1, nullptr, std::nullopt, nullptr);

//     const auto argMemLocation =
//         node.currentFrame.value().basePointerOffset; // argument is at offset 0
//     const TestAlignedArgClass4Slot* argMem =
//         reinterpret_cast<const TestAlignedArgClass4Slot*>(&node.values[argMemLocation]);
//     CHECK_EQ(argMem->v[0], 1);
//     CHECK_EQ(argMem->v[1], 2);
//     CHECK_EQ(argMem->v[2], 3);
//     CHECK_EQ(argMem->v[3], 4);
//     CHECK_EQ(node.types[argMemLocation], &type);
// }

// TEST_CASE("push 2 script function args, 1 slot, non-special align, no frames") {
//     auto node = Node(1);
//     const int64_t arg1 = 10;
//     const int64_t arg2 = 11;
//     auto result = node.pushScriptFunctionArg(&arg1, sy::Type::TYPE_I64, 0, 2, 1);
//     CHECK(result.has_value());
//     CHECK_EQ(result.value(), 1);
//     result = node.pushScriptFunctionArg(&arg2, sy::Type::TYPE_I64, 1, 2, 1);
//     CHECK(result.has_value());
//     CHECK_EQ(result.value(), 2);
//     node.pushFrameAllowReallocate(2, 1, nullptr, std::nullopt, nullptr);

//     const auto arg1MemLocation =
//         node.currentFrame.value().basePointerOffset; // argument 1 is at offset 0
//     const auto arg2MemLocation =
//         node.currentFrame.value().basePointerOffset + 1; // argument 2 is at offset 1
//     const int64_t* arg1Mem = reinterpret_cast<const int64_t*>(&node.values[arg1MemLocation]);
//     const int64_t* arg2Mem = reinterpret_cast<const int64_t*>(&node.values[arg2MemLocation]);
//     CHECK_EQ(*arg1Mem, arg1);
//     CHECK_EQ(*arg2Mem, arg2);
//     CHECK_EQ(node.types[arg1MemLocation], sy::Type::TYPE_I64);
//     CHECK_EQ(node.types[arg2MemLocation], sy::Type::TYPE_I64);
// }

// TEST_CASE("push 2 script function args, 2 slot, non-special align, no frames") {
//     auto node = Node(1);
//     const int64_t arg1 = 10;
//     const TestArgClass2Slot arg2 = {{1, 2}};
//     const sy::Type type = makeSimpleType<decltype(arg2)>();
//     auto result = node.pushScriptFunctionArg(&arg1, sy::Type::TYPE_I64, 0, 3, 1);
//     CHECK(result.has_value());
//     CHECK_EQ(result.value(), 1);
//     result = node.pushScriptFunctionArg(&arg2, &type, 1, 3, 1);
//     CHECK(result.has_value());
//     CHECK_EQ(result.value(), 3);
//     node.pushFrameAllowReallocate(3, 1, nullptr, std::nullopt, nullptr);

//     const auto arg1MemLocation =
//         node.currentFrame.value().basePointerOffset; // argument 1 is at offset 0
//     const auto arg2MemLocation =
//         node.currentFrame.value().basePointerOffset + 1; // argument 2 is at offset 1
//     const int64_t* arg1Mem = reinterpret_cast<const int64_t*>(&node.values[arg1MemLocation]);
//     const TestArgClass2Slot* arg2Mem =
//         reinterpret_cast<const TestArgClass2Slot*>(&node.values[arg2MemLocation]);
//     CHECK_EQ(*arg1Mem, arg1);
//     CHECK_EQ(arg2Mem->v[0], 1);
//     CHECK_EQ(arg2Mem->v[1], 2);
//     CHECK_EQ(node.types[arg1MemLocation], sy::Type::TYPE_I64);
//     CHECK_EQ(node.types[arg2MemLocation], &type);
// }

// TEST_CASE("push 2 script function args, 4 slot, non-special align, no frames") {
//     auto node = Node(1);
//     const int64_t arg1 = 10;
//     const TestArgClass4Slot arg2 = {{1, 2, 3, 4}};
//     const sy::Type type = makeSimpleType<decltype(arg2)>();
//     auto result = node.pushScriptFunctionArg(&arg1, sy::Type::TYPE_I64, 0, 5, 1);
//     CHECK(result.has_value());
//     CHECK_EQ(result.value(), 1);
//     result = node.pushScriptFunctionArg(&arg2, &type, 1, 5, 1);
//     CHECK(result.has_value());
//     CHECK_EQ(result.value(), 5);
//     node.pushFrameAllowReallocate(5, 1, nullptr, std::nullopt, nullptr);

//     const auto arg1MemLocation =
//         node.currentFrame.value().basePointerOffset; // argument 1 is at offset 0
//     const auto arg2MemLocation =
//         node.currentFrame.value().basePointerOffset + 1; // argument 2 is at offset 1
//     const int64_t* arg1Mem = reinterpret_cast<const int64_t*>(&node.values[arg1MemLocation]);
//     const TestArgClass4Slot* arg2Mem =
//         reinterpret_cast<const TestArgClass4Slot*>(&node.values[arg2MemLocation]);
//     CHECK_EQ(*arg1Mem, arg1);
//     CHECK_EQ(arg2Mem->v[0], 1);
//     CHECK_EQ(arg2Mem->v[1], 2);
//     CHECK_EQ(arg2Mem->v[2], 3);
//     CHECK_EQ(arg2Mem->v[3], 4);
//     CHECK_EQ(node.types[arg1MemLocation], sy::Type::TYPE_I64);
//     CHECK_EQ(node.types[arg2MemLocation], &type);
// }

// TEST_CASE("push 2 script function args, 4 slot, special align, no frames") {
//     auto node = Node(1);
//     const int64_t arg1 = 10;
//     const TestAlignedArgClass4Slot arg2 = {{1, 2, 3, 4}};
//     const sy::Type type = makeSimpleType<decltype(arg2)>();
//     auto result = node.pushScriptFunctionArg(&arg1, sy::Type::TYPE_I64, 0, 8, 32);
//     CHECK(result.has_value());
//     CHECK_EQ(result.value(), 1);
//     result = node.pushScriptFunctionArg(&arg2, &type, 1, 8, 32);
//     CHECK(result.has_value());
//     CHECK_EQ(result.value(), 8);
//     node.pushFrameAllowReallocate(8, alignof(decltype(arg2)), nullptr, std::nullopt, nullptr);

//     const auto arg1MemLocation =
//         node.currentFrame.value().basePointerOffset; // argument 1 is at offset 0
//     const auto arg2MemLocation =
//         node.currentFrame.value().basePointerOffset + 4; // argument 2 is at offset 4
//     const int64_t* arg1Mem = reinterpret_cast<const int64_t*>(&node.values[arg1MemLocation]);
//     const TestAlignedArgClass4Slot* arg2Mem =
//         reinterpret_cast<const TestAlignedArgClass4Slot*>(&node.values[arg2MemLocation]);
//     CHECK_EQ(*arg1Mem, arg1);
//     CHECK_EQ(arg2Mem->v[0], 1);
//     CHECK_EQ(arg2Mem->v[1], 2);
//     CHECK_EQ(arg2Mem->v[2], 3);
//     CHECK_EQ(arg2Mem->v[3], 4);
//     CHECK_EQ(node.types[arg1MemLocation], sy::Type::TYPE_I64);
//     CHECK_EQ(node.types[arg2MemLocation], &type);
// }

// #endif
