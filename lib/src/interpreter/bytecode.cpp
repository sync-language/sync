#include "bytecode.hpp"

OpCode Bytecode::getOpcode() const
{
    // This is safe.
    return static_cast<OpCode>(this->value & OPCODE_BITMASK);
}
