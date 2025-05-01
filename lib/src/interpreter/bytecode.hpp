#pragma once
#ifndef SY_INTERPRETER_BYTECODE_HPP_
#define SY_INTERPRETER_BYTECODE_HPP_

#include "../core.h"

namespace sy {
    struct Type;
}

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

/// Zero initializing gets a Nop.
struct Bytecode {
    uint64_t value;

    OpCode getOpcode() const;
};

enum class ScalarTag : uint8_t {
    Bool,
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    USize,
    F32,
    F64,
};

constexpr size_t SCALAR_TAG_USED_BITS = 6;

const sy::Type* scalarTypeFromTag(ScalarTag tag);

#endif // SY_INTERPRETER_BYTECODE_HPP_