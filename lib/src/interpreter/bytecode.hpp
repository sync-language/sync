#pragma once
#ifndef SY_INTERPRETER_BYTECODE_HPP_
#define SY_INTERPRETER_BYTECODE_HPP_

#include "../core.h"

enum class OpCode : uint8_t {
    Nop = 0,
    LoadZero,
    LoadImmediate,
    Return,
    Call,
    Jump,
    JumpIfFalse,
    Destruct,
    Sync,
    Unsync,
    Move,
    Clone,
    Dereference,
    SetReference,
    MakeReference,
    GetMember,
    SetMember,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Add,
};

constexpr size_t OPCODE_USED_BITS = 8;
constexpr size_t OPCODE_BITMASK = 0b11111111;

struct Bytecode {
    uint64_t value;

    OpCode getOpcode() const;
};


#endif // SY_INTERPRETER_BYTECODE_HPP_