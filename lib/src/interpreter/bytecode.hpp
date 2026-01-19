#pragma once
#ifndef SY_INTERPRETER_BYTECODE_HPP_
#define SY_INTERPRETER_BYTECODE_HPP_

#include "../core/core.h"
#include "stack/stack.hpp"

namespace sy {
class Type;

/// All OpCodes occupy 1 byte
enum class OpCode : uint8_t {
    /// Does nothing
    Noop = 0x00,
    /// Returns from a function without a return value. After this operation, the function stops executing, the stack
    /// is then unwinded, and the frame is popped. Uses `operators::Return`.
    Return,
    /// Returns from a function with a return value. After this operation, the function stops executing, the stack is
    /// then unwinded, and the frame is popped. Uses `operators::ReturnValue`.
    ReturnValue,
    /// Calls a function whose `const sy::Function*` instance is provided within the bytecode as the one after the
    /// initial bytecode. The function argument sources started at the bytecode after the immediate function bytecode,
    /// extending as necessary as an array of `uint16_t` values. The function returns no value.
    /// Is at least 2 wide, but often more depending on function arguments. Uses `operators::CallImmediateNoReturn`.
    CallImmediateNoReturn,
    /// Calls a function at `src`. The function argument sources start after the initial bytecode, extending as
    /// necessary as an array of `uint16_t` values. The function returns no value. Uses `operators::CallSrcNoReturn`.
    CallSrcNoReturn,
    /// Calls a function whose `const sy::Function*` instance is provided within the bytecode as the one after the
    /// initial bytecode. The function argument sources started at the bytecode after the immediate function bytecode,
    /// extending as necessary as an array of `uint16_t` values. The function returns a value.
    /// Is at least 2 wide, but often more depending on function arguments. Uses `operators::CallImmediateWithReturn`.
    CallImmediateWithReturn,
    /// Calls a function at `src`. The function argument sources start after the initial bytecode, extending as
    /// necessary as an array of `uint16_t` values. The function returns a value. Uses `operators::CallSrcWithReturn`.
    CallSrcWithReturn,
    /// May be 2 wide instruction if loading the default value for non scalar types.
    /// For scalar types, loads zero values. Uses `operators::LoadDefault`.
    LoadDefault,
    /// May be 2 wide instructions, if loading immediate value greater than 32 bits in size.
    /// Uses `operators::LoadImmediateScalar`.
    LoadImmediateScalar,
    /// Loads value 0xAA into all bytes of the memory an object will occupy. Does not take a type, and doesn't set a
    /// type. Just calls memset.
    /// TODO memset references or heap allocations?
    /// This is the same as [undefined](https://ziglang.org/documentation/master/#undefined) in zig.
    /// Primarily, this is useful for doing struct and array initialization. Uses `operators::MemsetUninitialized`.
    MemsetUninitialized,
    /// Forcibly sets the type at `dst` to a type. Overrides the type of whatever was present.
    /// May be 2 wide instruction, if the type is not a scalar type, thus `isScalar` flag is false.
    /// Uses `operators::SetType`.
    SetType,
    /// Forcibly sets the type at `dst` to be null, signaling that the memory has no type.
    /// Useful to set memory as shouldn't be unwinded, or shouldn't be operated on until later specified.
    /// Uses `operators::SetNullType`.
    SetNullType,
    /// Unconditionally jumps the instruction pointer by `amount` bytecodes. Can jump by a positive or negative value.
    /// Is within the range of `int32_t`. Uses `operators::Jump`.
    Jump,
    /// Conditionally jumps the instruction pointer by `amount` bytecodes, if `src == false`. Can jump by a positive or
    /// negative value. Is within the range of `int32_t`. Uses `operators::JumpIfFalse`.
    JumpIfFalse,
    /// Explicitly call the destructor of `src`, which also removes it's type info. Uses `operators::Destruct`.
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
    Subtract,
    Multiply,
    Divide,
};

constexpr size_t OPCODE_USED_BITS = 8;
constexpr size_t OPCODE_BITMASK = 0b11111111;

/// Zero initializing gets a Nop.
struct Bytecode {
    uint64_t value = 0;

    Bytecode() = default;

    template <typename OperandsT> Bytecode(const OperandsT& operands) {
        static_assert(sizeof(OperandsT) == sizeof(Bytecode));
        static_assert(alignof(OperandsT) == alignof(Bytecode));
        assertOpCodeMatch(static_cast<OpCode>(operands.reserveOpcode),
                          OperandsT::OPCODE); // make sure mistakes arent made
        *this = reinterpret_cast<const Bytecode&>(operands);
    }

    OpCode getOpcode() const;

    template <typename OperandsT> OperandsT toOperands() const {
        static_assert(sizeof(OperandsT) == sizeof(Bytecode));
        static_assert(alignof(OperandsT) == alignof(Bytecode));
        assertOpCodeMatch(this->getOpcode(), OperandsT::OPCODE);
        return *reinterpret_cast<const OperandsT*>(this);
    }

  private:
    static void assertOpCodeMatch(OpCode actual, OpCode expected);
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

/// Holds all operand types. All operands are expected to have a static constexpr member named `OPCODE` of type
/// `OpCode`, matching the opcode of the operation. This is used for validation.
/// They are also all expected to have the first `OPCODE_USED_BITS` be a bitfield named `reserveOpcode`.
/// Lastly, all operand types must be of size `sizeof(Bytecode)`, and align `alignof(Bytecode)`.
namespace operators {

/// Returns from a function without a return value.
struct Return {
    uint64_t reserveOpcode : OPCODE_USED_BITS;

    static constexpr OpCode OPCODE = OpCode::Return;
};

/// Returns from a function with a value.
struct ReturnValue {
    uint64_t reserveOpcode : OPCODE_USED_BITS;
    uint64_t src : Stack::BITS_PER_STACK_OPERAND;

    static constexpr OpCode OPCODE = OpCode::ReturnValue;
};

struct CallImmediateNoReturn {
    uint64_t reserveOpcode : OPCODE_USED_BITS;
    uint64_t argCount : 16;

    static size_t bytecodeUsed(uint16_t argCount);

    static constexpr OpCode OPCODE = OpCode::CallImmediateNoReturn;
};

struct CallSrcNoReturn {
    uint64_t reserveOpcode : OPCODE_USED_BITS;
    uint64_t src : Stack::BITS_PER_STACK_OPERAND;
    uint64_t argCount : 16;

    static size_t bytecodeUsed(uint16_t argCount);

    static constexpr OpCode OPCODE = OpCode::CallSrcNoReturn;
};

struct CallImmediateWithReturn {
    uint64_t reserveOpcode : OPCODE_USED_BITS;
    uint64_t argCount : 16;
    uint64_t retDst : Stack::BITS_PER_STACK_OPERAND;

    static size_t bytecodeUsed(uint16_t argCount);

    static constexpr OpCode OPCODE = OpCode::CallImmediateWithReturn;
};

struct CallSrcWithReturn {
    uint64_t reserveOpcode : OPCODE_USED_BITS;
    uint64_t src : Stack::BITS_PER_STACK_OPERAND;
    uint64_t argCount : 16;
    uint64_t retDst : Stack::BITS_PER_STACK_OPERAND;

    static size_t bytecodeUsed(uint16_t argCount);

    static constexpr OpCode OPCODE = OpCode::CallSrcWithReturn;
};

/// If `isScalar == false`, this is a wide instruction, with the second "bytecode" being a
/// `const Sy::Type*` instance.
struct LoadDefault {
    uint64_t reserveOpcode : OPCODE_USED_BITS;
    /// Boolean
    uint64_t isScalar : 1;
    /// Used if `isScalar == true`
    uint64_t scalarTag : SCALAR_TAG_USED_BITS;
    uint64_t dst : Stack::BITS_PER_STACK_OPERAND;

    static constexpr OpCode OPCODE = OpCode::LoadDefault;
};

struct LoadImmediateScalar {
    uint64_t reserveOpcode : OPCODE_USED_BITS;
    uint64_t scalarTag : SCALAR_TAG_USED_BITS;
    uint64_t dst : Stack::BITS_PER_STACK_OPERAND;
    uint64_t immediate : 32;

    static size_t bytecodeUsed(ScalarTag scalarTag);

    static constexpr OpCode OPCODE = OpCode::LoadImmediateScalar;
};

struct MemsetUninitialized {
    uint64_t reserveOpcode : OPCODE_USED_BITS;
    /// Boolean
    uint64_t dst : Stack::BITS_PER_STACK_OPERAND;
    uint64_t slots : 16;

    static constexpr OpCode OPCODE = OpCode::MemsetUninitialized;
};

struct SetType {
    uint64_t reserveOpcode : OPCODE_USED_BITS;
    /// Boolean
    uint64_t dst : Stack::BITS_PER_STACK_OPERAND;
    uint64_t isScalar : 1;
    /// Used if `isScalar == true`
    uint64_t scalarTag : SCALAR_TAG_USED_BITS;

    static constexpr OpCode OPCODE = OpCode::SetType;
};

struct SetNullType {
    uint64_t reserveOpcode : OPCODE_USED_BITS;
    /// Boolean
    uint64_t dst : Stack::BITS_PER_STACK_OPERAND;

    static constexpr OpCode OPCODE = OpCode::SetNullType;
};

struct Jump {
    uint64_t reserveOpcode : OPCODE_USED_BITS;
    int64_t amount : 32;

    static constexpr OpCode OPCODE = OpCode::Jump;
};

struct JumpIfFalse {
    uint64_t reserveOpcode : OPCODE_USED_BITS;
    uint64_t src : Stack::BITS_PER_STACK_OPERAND;
    int64_t amount : 32;

    static constexpr OpCode OPCODE = OpCode::JumpIfFalse;
};

struct Destruct {
    uint64_t reserveOpcode : OPCODE_USED_BITS;
    uint64_t src : Stack::BITS_PER_STACK_OPERAND;

    static constexpr OpCode OPCODE = OpCode::Destruct;
};

} // namespace operators
} // namespace sy

#endif // SY_INTERPRETER_BYTECODE_HPP_