#pragma once
#ifndef SY_INTERPRETER_STACK_FRAME_HPP_
#define SY_INTERPRETER_STACK_FRAME_HPP_

#include "../../core.h"
#include <optional>
#include <tuple>

struct Bytecode;

class Frame {
public:
    // Sync only compiles on targets with full support for 64 bit integers (not necessarily 64 bit architectures
    // due to the existence of wasm32). As a result, frame metadata is stored as 4, 64 bit integer (2 + 2).

    uint32_t basePointerOffset = 0;
    uint32_t frameLength = 0;
    uint16_t functionIndex = 0;
    void*    retValueDst = nullptr;

    Frame() = default;

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

#endif // SY_INTERPRETER_STACK_FRAME_HPP_
