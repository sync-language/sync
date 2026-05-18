#pragma once
#ifndef SY_COMPILER_TOKENIZER_TOKEN_HPP_
#define SY_COMPILER_TOKENIZER_TOKEN_HPP_

#include "../../core/core.h"
#include "../../mem/allocator.hpp"
#include "../../program/program_error.hpp"
#include "../../types/array/dynamic_array.hpp"
#include "../../types/option/option.hpp"
#include "../../types/result/result.hpp"
#include "../../types/string/string_slice.hpp"
#include <tuple>

namespace sy {
namespace internal {
struct Test_TokenRef;
struct Test_TokenIter;
struct Test_TokenStore;
} // namespace internal

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
    TryKeyword,
    CatchKeyword,
    FnKeyword,
    PubKeyword,
    IfKeyword,
    ElseKeyword,
    SwitchKeyword,
    WhileKeyword,
    ForKeyword,
    InKeyword,
    BreakKeyword,
    ContinueKeyword,
    StructKeyword,
    EnumKeyword,
    DynKeyword,
    LifetimeDynKeyword,
    TraitKeyword,
    WhereKeyword,
    SelfIdentifierKeyword,
    SelfTypeKeyword,
    ImplKeyword,
    SpecificKeyword,
    SyncKeyword,
    TrueKeyword,
    FalseKeyword,
    NullKeyword,
    AndKeyword,
    OrKeyword,
    UniqueKeyword,
    SharedKeyword,
    WeakKeyword,
    AsKeyword,
    PanicKeyword,
    AssertKeyword,
    PrintKeyword,
    ExternKeyword,
    ImportKeyword,
    ParallelKeyword,
    AwaitKeyword,
    TestKeyword,

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
    TypePrimitive,
    ListPrimitive,
    MapPrimitive,
    SetPrimitive,
    TaskPrimitive,

    NumberLiteral,
    CharLiteral,
    StringLiteral,
    FormatString,

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
    Slice,
    SliceLifetime,
};

StringSlice tokenTypeToString(TokenType tokenType);

class Token {
  public:
    constexpr Token(TokenType inTag, uint32_t inLocation)
        : tag_(static_cast<uint8_t>(inTag)), location_(inLocation) {}

    /// @return First tuple value is the token. Second tuple value is the next
    /// location to start. If the second tuple value is `-1`, then the end of
    /// file has been reached but a token was at the end. Third tuple value
    /// is the line number
    static std::tuple<Token, uint32_t> parseToken(const StringSlice source, const uint32_t start,
                                                  uint32_t* lineNumber);

    [[nodiscard]] constexpr TokenType tag() const { return static_cast<TokenType>(tag_); }

    [[nodiscard]] constexpr uint32_t location() const { return location_; }

  private:
    uint32_t tag_ : 8;
    uint32_t location_ : 24;
};

struct TokenSpan {
    size_t byteLocation;
    size_t length;
};

class TokenStore;
class NewTokenIter;

class TokenRef {
  public:
    TokenRef() = default;
    ~TokenRef() noexcept = default;
    TokenRef(TokenRef&& other) noexcept = default;
    TokenRef& operator=(TokenRef&& other) noexcept = default;
    TokenRef(const TokenRef& other) = default;
    TokenRef& operator=(const TokenRef& other) = default;

    TokenType kind() const noexcept;

    TokenSpan span() const noexcept;

    StringSlice text() const noexcept;

  private:
    friend struct internal::Test_TokenRef;
    friend class TokenStore;
    friend class NewTokenIter;

    const TokenStore* store_ = nullptr;
    uint32_t index_ = 0;
};

class NewTokenIter {
  public:
    NewTokenIter(const TokenStore* store) noexcept;

    ~NewTokenIter() noexcept = default;
    NewTokenIter(NewTokenIter&& other) noexcept = default;
    NewTokenIter& operator=(NewTokenIter&& other) noexcept = default;
    NewTokenIter(const NewTokenIter& other) = default;
    NewTokenIter& operator=(const NewTokenIter& other) = default;

    Option<TokenRef> next() noexcept;

  private:
    friend struct Test_TokenIter;
    friend class TokenStore;

    TokenRef current_;
};

class TokenStore {
  public:
    ~TokenStore() noexcept;

    TokenStore(TokenStore&& other) noexcept;

    TokenStore& operator=(TokenStore&& other) noexcept;

    TokenStore(const TokenStore& other) = delete;

    TokenStore& operator=(const TokenStore& other) = delete;

    [[nodiscard]] static Result<TokenStore, ProgramError> init(Allocator alloc,
                                                               StringSlice source) noexcept;

    [[nodiscard]] NewTokenIter iter() const noexcept { return NewTokenIter(this); }

    [[nodiscard]] StringSlice source() const noexcept {
        return StringSlice(this->source_.data, this->source_.len);
    };

  private:
    friend struct Test_TokenStore;
    friend class NewTokenIter;
    friend class TokenRef;

    struct SimdAllocatedSource {
        char* data = nullptr;
        size_t len = 0;
        size_t capacity = 0;
        size_t align = 0;
    };

    TokenStore(Allocator alloc) : alloc_(alloc) {}

    Result<void, AllocErr> ensureCapacity(size_t capacity) noexcept;

    static Result<SimdAllocatedSource, AllocErr>
    allocatedSimdPaddedSource(Allocator alloc, size_t sourceLen) noexcept;

    void freeData() noexcept;

    sy::Allocator alloc_;
    SimdAllocatedSource source_{};
    /// SoA with `spans_`. Length of `tokensLen_`. Allocation capacity of `capacity_`.
    TokenType* tokens_ = nullptr;
    /// SoA with `tokens_`. Length of `tokensLen_`. Allocation capacity of `capacity_`.
    TokenSpan* spans_ = nullptr;
    size_t tokensLen_ = 0;
    size_t tokensCapacity_ = 0;
    /// Which byte position a new line occurred. Used for binary search.
    DynArray<size_t> newLineBytes_{};
};

} // namespace sy

#endif // SY_COMPILER_TOKENIZER_TOKEN_HPP_
