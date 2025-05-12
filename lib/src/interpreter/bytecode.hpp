#pragma once
#ifndef SY_INTERPRETER_BYTECODE_HPP_
#define SY_INTERPRETER_BYTECODE_HPP_

#include "../core.h"
#include "stack.hpp"

namespace sy {
    struct Type;
}

enum class OpCode : uint8_t {
    Nop = 0,
    Return,
    ReturnValue,
    CallImmediateNoReturn,
    CallSrcNoReturn,
    CallImmediateWithReturn,
    CallSrcWithReturn,
    /// May be 2 wide instruction if loading the default value for non scalar types.
    /// For scalar types, loads zero values.
    LoadDefault,
    /// May be 2 wide instruction if loading uninitialized memory for non scalar type.
    /// Loads value 0xAA into all bytes of the memory an object will occupy.
    /// This is the same as [undefined](https://ziglang.org/documentation/master/#undefined) in zig.
    /// Primarily, this is useful for doing struct and array initialization.
    LoadUninitialized,
    LoadImmediate,
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
    uint64_t value = 0;

    template<typename OperandsT>
    Bytecode(const OperandsT& operands) {
        static_assert(sizeof(OperandsT) == sizeof(Bytecode));
        static_assert(alignof(OperandsT) == alignof(Bytecode));
        assertOpCodeMatch(operands.reserveOpcode, OperandsT::OPCODE); // make sure mistakes arent made
        *this = reinterpret_cast<const Bytecode&>(operands);
    }

    OpCode getOpcode() const;

    template<typename OperandsT>
    OperandsT toOperands() const {
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
namespace operands {

    /// Returns from a function without a return value.
    struct Return {
        uint64_t reserveOpcode: OPCODE_USED_BITS;

        static constexpr OpCode OPCODE = OpCode::Return;
    };

    /// Returns from a function with a value.
    struct ReturnValue {
        uint64_t reserveOpcode: OPCODE_USED_BITS;
        uint64_t src: Stack::BITS_PER_STACK_OPERAND;

        static constexpr OpCode OPCODE = OpCode::ReturnValue;
    };

    struct CallImmediateNoReturn {
        uint64_t reserveOpcode: OPCODE_USED_BITS;
        uint64_t argCount: 16;

        static size_t bytecodeUsed(uint16_t argCount);
        
        static constexpr OpCode OPCODE = OpCode::CallImmediateNoReturn;
    };
    
    /// If `isScalar == false`, this is a wide instruction, with the second "bytecode" being a 
    /// `const Sy::Type*` instance.
    struct LoadDefault {
        uint64_t reserveOpcode: OPCODE_USED_BITS;
        /// Boolean
        uint64_t isScalar: 1;
        /// Used if `isScalar == true`
        uint64_t scalarTag: SCALAR_TAG_USED_BITS;
        uint64_t dst: Stack::BITS_PER_STACK_OPERAND;

        static constexpr OpCode OPCODE = OpCode::LoadDefault;
    };

    /// If `isScalar == false`, this is a wide instruction, with the second "bytecode" being a 
    /// `const Sy::Type*` instance.
    struct LoadUninitialized {
        uint64_t reserveOpcode: OPCODE_USED_BITS;
        /// Boolean
        uint64_t isScalar: 1;
        /// Used if `isScalar == true`
        uint64_t scalarTag: SCALAR_TAG_USED_BITS;
        uint64_t dst: Stack::BITS_PER_STACK_OPERAND;

        static constexpr OpCode OPCODE = OpCode::LoadUninitialized;
    };

}

#endif // SY_INTERPRETER_BYTECODE_HPP_