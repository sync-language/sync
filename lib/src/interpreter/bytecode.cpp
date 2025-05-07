#include "bytecode.hpp"
#include "../types/type_info.hpp"
#include "../util/unreachable.hpp"
#include "../util/assert.hpp"

OpCode Bytecode::getOpcode() const
{
    // This is safe.
    return static_cast<OpCode>(this->value & OPCODE_BITMASK);
}

void Bytecode::assertOpCodeMatch(OpCode actual, OpCode expected)
{
    sy_assert(actual == expected, "Cannot convert this bytecode to an invalid operand");
}

const sy::Type *scalarTypeFromTag(ScalarTag tag)
{
    switch(tag) {
        case ScalarTag::Bool: return sy::Type::TYPE_BOOL;
        case ScalarTag::I8: return sy::Type::TYPE_I8;
        case ScalarTag::I16: return sy::Type::TYPE_I16;
        case ScalarTag::I32: return sy::Type::TYPE_I32;
        case ScalarTag::I64: return sy::Type::TYPE_I64;
        case ScalarTag::U8: return sy::Type::TYPE_U8;
        case ScalarTag::U16: return sy::Type::TYPE_U16;
        case ScalarTag::U32: return sy::Type::TYPE_U32;
        case ScalarTag::U64: return sy::Type::TYPE_U64;
        case ScalarTag::USize: return sy::Type::TYPE_USIZE;
        case ScalarTag::F32: return sy::Type::TYPE_F32;
        case ScalarTag::F64: return sy::Type::TYPE_F64;
    }
    unreachable();
    return nullptr;
}

size_t operands::CallImmediateNoReturn::bytecodeUsed(uint16_t argCount)
{
    /// Initial bytecode + immediate function
    size_t used = 1 + 1;
    if((argCount % 4) == 0) {
        used += (argCount / 4);
    } else {
        used += (argCount / 4) + 1;
    }
    return used;
}
