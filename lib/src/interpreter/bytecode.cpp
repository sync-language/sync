#include "bytecode.hpp"
#include "../core/core_internal.h"
#include "../types/type.hpp"

using namespace sy;

static_assert(sizeof(Bytecode) < sizeof(Type), "Catch if Type changes size");
static_assert(sizeof(Bytecode) * BYTECODE_NEEDED_FOR_TYPE <= sizeof(Type),
              "Type must occupy no more than 2 bytecode");

OpCode sy::Bytecode::getOpcode() const {
    // This is safe.
    return static_cast<OpCode>(this->value & OPCODE_BITMASK);
}

void sy::Bytecode::assertOpCodeMatch(OpCode actual, OpCode expected) {
    sy_assert(actual == expected, "Cannot convert this bytecode to an invalid operand");
    (void)actual;
    (void)expected;
}

sy::Type sy::scalarTypeFromTag(ScalarTag tag) {
    switch (tag) {
    case ScalarTag::Bool:
        return sy::internal::TYPE_BOOL;
    case ScalarTag::I8:
        return sy::internal::TYPE_I8;
    case ScalarTag::I16:
        return sy::internal::TYPE_I16;
    case ScalarTag::I32:
        return sy::internal::TYPE_I32;
    case ScalarTag::I64:
        return sy::internal::TYPE_I64;
    case ScalarTag::U8:
        return sy::internal::TYPE_U8;
    case ScalarTag::U16:
        return sy::internal::TYPE_U16;
    case ScalarTag::U32:
        return sy::internal::TYPE_U32;
    case ScalarTag::U64:
        return sy::internal::TYPE_U64;
    case ScalarTag::USize:
        return sy::internal::TYPE_USIZE;
    case ScalarTag::F32:
        return sy::internal::TYPE_F32;
    case ScalarTag::F64:
        return sy::internal::TYPE_F64;
    }
    sync_unreachable();
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
    sy::Type scalarType = scalarTypeFromTag(scalarTag);
    if (scalarType.base->typeSize <= 4) {
        // Fits into initial bytecode
        return 1;
    } else {
        // LoadImmediateScalar operands + the memory required for the immediate value
        return 1 + (1 + scalarType.base->typeSize / alignof(Bytecode));
    }
}

Type sy::operators::SetType::getNonScalarType(const SetType* self) noexcept {
    sy_assert(self->isScalar == false,
              "Cannot get non-scalar type if this instruction is for a scalar");
    const SetType* offsetMem = &self[1];
    return *reinterpret_cast<const Type*>(offsetMem);
}
