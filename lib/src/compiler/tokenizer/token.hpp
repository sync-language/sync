#pragma once
#ifndef SY_COMPILER_TOKENIZER_TOKEN_HPP_
#define SY_COMPILER_TOKENIZER_TOKEN_HPP_

#include "../../core.h"
#include "../../types/string/string_slice.hpp"

/// Maximum length of a source file mustn't exceed 24 bits
static constexpr size_t MAX_SOURCE_LEN = 0x00FFFFFF;

enum class TokenType : uint8_t {
    Error = 0,
    EndOfFile = 1,

    ConstKeyword,
    MutKeyword,
    ReturnKeyword,
    FnKeyword,
    PubKeyword,
    IfKeyword,
    ElseKeyword,
    SwitchKeyword,
    WhileKeyword,
    ForKeyword,
    BreakKeyword,
    ContinueKeyword,
    StructKeyword,
    EnumKeyword,
    // UnionKeyword,
    // DynKeyword,
    // InterfaceKeyword,
    SyncKeyword,
    // UnsafeKeyword,
    TrueKeyword,
    FalseKeyword,
    NullKeyword,
    AndKeyword,
    OrKeyword,
    // ImportKeyword,
    // ModKeyword,
    // ExternKeyword,

    BoolPrimitive,
    I8Primitive,
    I16Primitive,
    I32Primitive,
    I64Primitive,
    U8Primitive,
    U16Primitive,
    U32Primitive,
    U64Primitive,
    USizePrimitive,
    F32Primitive,
    F64Primitive,
    CharPrimitive,
    StrPrimitive,
    StringPrimitive,
    // PartialOrdPrimitive,
    // OrdPrimitive,
    // MapPrimitive,
    // SetPrimitive,
    SyncOwnedPrimitive,
    SyncSharedPrimitive,
    SyncWeakPrimitive,
    // SyncMapPrimitive,
    // SyncSetPrimitive,

    IntLiteral,
    FloatLiteral,
    CharLiteral,
    StringLiteral,

    Identifier,

    EqualOperator,
    AssignOperator,
    NotEqualOperator,
    ErrorUncheckedUnwrapOperator,
    OptionUncheckedUnwrapOperator,
    NotOperator,
    // DereferenceOperator,
    LessOrEqualOperator,
    LessOperator,
    GreaterOrEqualOperator,
    GreaterOperator,
    AddAssignOperator,
    AddOperator,
    SubtractAssignOperator,
    SubtractOperator,
    MultiplyAssignOperator,
    MultiplyOperator,
    DivideAssignOperator,
    DivideOperator,
    PowerAssignOperator,
    PowerOperator,
    ModuloAssignOperator,
    ModuloOperator,
    BitshiftRightAssignOperator,
    BitshiftRightOperator,
    BitshiftLeftAssignOperator,
    BitshiftLeftOperator,
    BitAndAssignOperator,
    BitAndOperator,
    BitOrAssignOperator,
    BitOrOperator,
    BitXorAssignOperator,
    BitXorOperator,
    BitNotAssignOperator,
    BitNotOperator,

    LeftParenthesesSymbol,
    RightParenthesesSymbol,
    LeftBracketSymbol,
    RightBracketSymbol,
    LeftBraceSymbol,
    RightBraceSymbol,
    SemicolonSymbol,
    DotSymbol,
    OptionalSymbol,
    ErrorSymbol,
    ImmutableReferenceSymbol,
    MutableReferenceSymbol,
};

sy::StringSlice tokenTypeToString(TokenType tokenType);

class Token {
private:
    uint32_t tag_ : 8;
    uint32_t location_ : 24;
};

#endif //SY_COMPILER_TOKENIZER_TOKEN_HPP_