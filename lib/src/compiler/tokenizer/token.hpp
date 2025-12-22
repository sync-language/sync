#pragma once
#ifndef SY_COMPILER_TOKENIZER_TOKEN_HPP_
#define SY_COMPILER_TOKENIZER_TOKEN_HPP_

#include "../../core.h"
#include "../../types/string/string_slice.hpp"
#include <tuple>

namespace sy {
/// Maximum length of a source file mustn't exceed 24 bits
static constexpr size_t MAX_SOURCE_LEN = 0x00FFFFFF;

enum class TokenType : uint8_t {
    Error = 0,
    EndOfFile = 1,

    ConstKeyword,
    MutKeyword,
    ComptimeKeyword,
    ReturnKeyword,
    ThrowKeyword,
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
    DynKeyword,
    LifetimeDynKeyword,
    TraitKeyword,
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
    // AssertKeyword,
    UniqueKeyword,
    SharedKeyword,
    WeakKeyword,

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
    TypePrimitive,
    ListPrimitive,
    MapPrimitive,
    SetPrimitive,

    NumberLiteral,
    CharLiteral,
    StringLiteral,

    Identifier,

    EqualOperator,
    AssignOperator,
    NotEqualOperator,
    ErrorUnwrapOperator,
    OptionUnwrapOperator,
    // NotOperator,
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
    // MultiplyOperator,
    DivideAssignOperator,
    DivideOperator,
    // PowerAssignOperator,
    // PowerOperator,
    ModuloAssignOperator,
    ModuloOperator,
    BitshiftRightAssignOperator,
    BitshiftRightOperator,
    BitshiftLeftAssignOperator,
    BitshiftLeftOperator,
    BitAndAssignOperator,
    // BitAndOperator,
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
    ColonSymbol,
    SemicolonSymbol,
    DotSymbol,
    CommaSymbol,
    OptionalSymbol,
    // ErrorSymbol,
    // ImmutableReferenceSymbol,
    MutableReferenceSymbol,
    AmpersandSymbol,
    ExclamationSymbol,
    AsteriskSymbol,

    LifetimePointer,
    ConcreteLifetime,
};

StringSlice tokenTypeToString(TokenType tokenType);

class Token {
  public:
    constexpr Token(TokenType inTag, uint32_t inLocation) : tag_(static_cast<uint8_t>(inTag)), location_(inLocation) {}

    /// @return First tuple value is the token. Second tuple value is the next
    /// location to start. If the second tuple value is `-1`, then the end of
    /// file has been reached but a token was at the end. Third tuple value
    /// is the line number
    static std::tuple<Token, uint32_t> parseToken(const StringSlice source, const uint32_t start, uint32_t* lineNumber);

    [[nodiscard]] constexpr TokenType tag() const { return static_cast<TokenType>(tag_); }

    [[nodiscard]] constexpr uint32_t location() const { return location_; }

  private:
    uint32_t tag_ : 8;
    uint32_t location_ : 24;
};
} // namespace sy

#endif // SY_COMPILER_TOKENIZER_TOKEN_HPP_
