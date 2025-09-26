#include "frame.hpp"
#include "../../util/assert.hpp"

std::optional<uint32_t> Frame::frameExtendAmountForAlignment(const uint32_t totalSlots, const uint32_t nextBaseOffset,
                                                             const uint16_t alignment) {
    sy_assert(alignment % 2 == 0, "Expected frame alignment to be a multiple of 2");
    sy_assert(alignment >= 16, "Alignment should be greater than or equal to 16");

    const uint16_t normalizedAlignment = alignment / sizeof(void*);
    const uint16_t remainder = nextBaseOffset % normalizedAlignment;
    if (remainder == 0) {
        return std::optional<uint32_t>(0); // Does not need to extend at all because is aligned
    }

    const uint32_t offset = normalizedAlignment - remainder;
    const uint32_t newBaseOffset = nextBaseOffset + static_cast<uint32_t>(normalizedAlignment - remainder);
    if (newBaseOffset >= totalSlots) {
        return std::optional<uint32_t>(); // empty cause would stack overflow
    }

    sy_assert((newBaseOffset * sizeof(void*)) % alignment == 0,
              "Adjusted base offset must satisfy alignment requirements");

    return std::optional<uint32_t>(offset);
}

std::optional<std::tuple<Frame, const Bytecode*>> Frame::readFromMemory(const uint64_t* valuesMem,
                                                                        const uintptr_t* typesMem) {
    const Bytecode* oldInstructionPointer = reinterpret_cast<const Bytecode*>(valuesMem[OLD_INSTRUCTION_POINTER]);
    if (oldInstructionPointer == nullptr) {
        return std::nullopt;
    }

    const uint64_t frameLengthAndFunctionIndex = valuesMem[OLD_FRAME_LENGTH_AND_FUNCTION_INDEX];
    void* oldRetDst = const_cast<void*>(reinterpret_cast<const void*>(typesMem[OLD_RETURN_VALUE_DST]));
    const uint32_t oldBasePointerOffset = *reinterpret_cast<const uint32_t*>(&typesMem[OLD_BASE_POINTER_OFFSET]);

    const Frame oldFrame = {oldBasePointerOffset,
                            static_cast<uint32_t>(frameLengthAndFunctionIndex & 0xFFFFFFFF), // frame length
                            static_cast<uint16_t>(frameLengthAndFunctionIndex >> 32),        // function index
                            oldRetDst};
    return std::make_tuple(oldFrame, oldInstructionPointer);
}

void Frame::storeInMemory(uint64_t* valuesMem, uintptr_t* typesMem, const Bytecode* instructionPointer) const {
    valuesMem[OLD_INSTRUCTION_POINTER] = reinterpret_cast<uintptr_t>(instructionPointer);
    uint64_t* frameLengthAndFunctionIndex = &valuesMem[OLD_FRAME_LENGTH_AND_FUNCTION_INDEX];
    *frameLengthAndFunctionIndex =
        static_cast<uint64_t>(this->frameLength) | (static_cast<uint64_t>(this->functionIndex) << 32);

    void** oldRetDst = reinterpret_cast<void**>(&typesMem[OLD_RETURN_VALUE_DST]);
    *oldRetDst = const_cast<void*>(this->retValueDst);

    uint32_t* oldBasePointerOffset = reinterpret_cast<uint32_t*>(&typesMem[OLD_BASE_POINTER_OFFSET]);
    *oldBasePointerOffset = this->basePointerOffset;
}

void Frame::storeNullFrameInMemory(uint64_t* valueMem, uintptr_t* typesMem) {
    valueMem[OLD_INSTRUCTION_POINTER] = 0;
    valueMem[OLD_FRAME_LENGTH_AND_FUNCTION_INDEX] = 0;
    typesMem[OLD_RETURN_VALUE_DST] = 0;
    typesMem[OLD_BASE_POINTER_OFFSET] = 0;
}

const Bytecode* Frame::readOldInstructionPointer(const size_t* valuesMem) {
    return reinterpret_cast<const Bytecode*>(valuesMem[OLD_INSTRUCTION_POINTER]);
}

void Frame::storeOldInstructionPointer(size_t* valuesMem, const Bytecode* instructionPointer) {
    auto oldinstructionPointer = reinterpret_cast<const Bytecode**>(&valuesMem[OLD_INSTRUCTION_POINTER]);
    *oldinstructionPointer = instructionPointer;
}