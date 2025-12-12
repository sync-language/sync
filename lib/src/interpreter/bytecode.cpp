#include "bytecode.hpp"
#include "../types/type_info.hpp"
#include "../util/assert.hpp"
#include "../util/unreachable.hpp"

using namespace sy;

OpCode sy::Bytecode::getOpcode() const {
    // This is safe.
    return static_cast<OpCode>(this->value & OPCODE_BITMASK);
}

void sy::Bytecode::assertOpCodeMatch(OpCode actual, OpCode expected) {
    sy_assert(actual == expected, "Cannot convert this bytecode to an invalid operand");
}

const sy::Type* sy::scalarTypeFromTag(ScalarTag tag) {
    switch (tag) {
    case ScalarTag::Bool:
        return sy::Type::TYPE_BOOL;
    case ScalarTag::I8:
        return sy::Type::TYPE_I8;
    case ScalarTag::I16:
        return sy::Type::TYPE_I16;
    case ScalarTag::I32:
        return sy::Type::TYPE_I32;
    case ScalarTag::I64:
        return sy::Type::TYPE_I64;
    case ScalarTag::U8:
        return sy::Type::TYPE_U8;
    case ScalarTag::U16:
        return sy::Type::TYPE_U16;
    case ScalarTag::U32:
        return sy::Type::TYPE_U32;
    case ScalarTag::U64:
        return sy::Type::TYPE_U64;
    case ScalarTag::USize:
        return sy::Type::TYPE_USIZE;
    case ScalarTag::F32:
        return sy::Type::TYPE_F32;
    case ScalarTag::F64:
        return sy::Type::TYPE_F64;
    }
    unreachable();
}

size_t sy::operators::CallImmediateNoReturn::bytecodeUsed(uint16_t argCount) {
    /// Initial bytecode + immediate function
    size_t used = 1 + 1;
    if ((argCount % 4) == 0) {
        used += (argCount / 4);
    } else {
        used += (argCount / 4) + 1;
    }
    return used;
}

size_t sy::operators::CallSrcNoReturn::bytecodeUsed(uint16_t argCount) {
    /// Initial bytecode
    size_t used = 1;
    if ((argCount % 4) == 0) {
        used += (argCount / 4);
    } else {
        used += (argCount / 4) + 1;
    }
    return used;
}

size_t sy::operators::CallImmediateWithReturn::bytecodeUsed(uint16_t argCount) {
    /// Initial bytecode + immediate function
    size_t used = 1 + 1;
    if ((argCount % 4) == 0) {
        used += (argCount / 4);
    } else {
        used += (argCount / 4) + 1;
    }
    return used;
}

size_t sy::operators::CallSrcWithReturn::bytecodeUsed(uint16_t argCount) {
    /// Initial bytecode
    size_t used = 1;
    if ((argCount % 4) == 0) {
        used += (argCount / 4);
    } else {
        used += (argCount / 4) + 1;
    }
    return used;
}

size_t sy::operators::LoadImmediateScalar::bytecodeUsed(ScalarTag scalarTag) {
    const sy::Type* scalarType = scalarTypeFromTag(scalarTag);
    if (scalarType->sizeType <= 4) {
        // Fits into initial bytecode
        return 1;
    } else {
        // LoadImmediateScalar operands + the memory required for the immediate value
        return 1 + (1 + scalarType->sizeType / alignof(Bytecode));
    }
}
