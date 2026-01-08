#include "tokenizer.hpp"
#include "../../core/core_internal.h"
#include "../../program/program_error.hpp"
#include "../../threading/alloc_cache_align.hpp"

using namespace sy;

static_assert(sizeof(Token) == sizeof(uint32_t));

constexpr static size_t tokenizerAllocBytes(size_t capacity) {
    return (sizeof(Token) + sizeof(uint32_t) + sizeof(uint32_t)) * capacity;
}

Tokenizer::~Tokenizer() noexcept {
    if (len_ == 0)
        return;

    Token* tokens = const_cast<Token*>(this->tokens_);
    uint8_t* baseMem = reinterpret_cast<uint8_t*>(tokens);
    this->alloc_.freeArray(baseMem, tokenizerAllocBytes(this->len_));

    this->tokens_ = nullptr;
    this->ends_ = nullptr;
    this->lineNumbers_ = nullptr;
    this->len_ = 0;
}

Tokenizer::Tokenizer(Tokenizer&& other) noexcept
    : alloc_(other.alloc_), source_(other.source_), tokens_(other.tokens_), ends_(other.ends_),
      lineNumbers_(other.lineNumbers_), len_(other.len_) {
    other.tokens_ = nullptr;
    other.ends_ = nullptr;
    other.lineNumbers_ = nullptr;
    other.len_ = 0;
}

Tokenizer& Tokenizer::operator=(Tokenizer&& other) noexcept {
    if (this->len_ != 0) {
        Token* tokens = const_cast<Token*>(this->tokens_);
        uint8_t* baseMem = reinterpret_cast<uint8_t*>(tokens);
        this->alloc_.freeArray(baseMem, tokenizerAllocBytes(this->len_));
    }

    this->alloc_ = other.alloc_;
    this->source_ = other.source_;
    this->tokens_ = other.tokens_;
    this->ends_ = other.ends_;
    this->lineNumbers_ = other.lineNumbers_;
    this->len_ = other.len_;
    other.tokens_ = nullptr;
    other.ends_ = nullptr;
    other.lineNumbers_ = nullptr;
    other.len_ = 0;
    return *this;
}

sy::Result<Tokenizer, sy::ProgramError> Tokenizer::create(Allocator allocator, StringSlice source) noexcept {
    Tokenizer self(allocator);
    self.source_ = source;

    if (source.len() > MAX_SOURCE_LEN) {
        return Error(ProgramError::CompileSourceFileTooBig);
    }

    // The total amount of tokens will always be less than or equal
    // to the number of bytes in the source
    // As a result, we can over-allocate upfront.
    const size_t capacity = source.len() == 0 ? 1 : source.len();
    size_t totalBigBytes = tokenizerAllocBytes(capacity);

    Token* bigTokens;
    uint32_t* bigEnds;
    uint32_t* bigLineNumbers;
    {
        auto bigMemRes = self.alloc_.allocArray<uint8_t>(totalBigBytes);
        if (bigMemRes.hasErr()) {
            return Error(ProgramError::OutOfMemory);
        }
        uint8_t* baseMem = bigMemRes.value();
        bigTokens = reinterpret_cast<Token*>(baseMem);
        bigEnds = reinterpret_cast<uint32_t*>(baseMem + (sizeof(Token) * capacity));
        bigLineNumbers =
            reinterpret_cast<uint32_t*>(baseMem + (sizeof(Token) * capacity) + (sizeof(uint32_t) * capacity));
    }

    uint32_t tokenIter = 0;
    if (source.len() == 0) {
        bigTokens[0] = Token(TokenType::EndOfFile, 0);
        bigEnds[0] = 0;
        bigLineNumbers[0] = 1;
        tokenIter += 1;
    } else {
        uint32_t start = 0;
        uint32_t lineNumber = 1;
        bool keepFinding = true;
        while (keepFinding) {
            sy_assert(tokenIter < source.len(), "Infinite loop detected");

            auto [token, newStart] = Token::parseToken(source, start, &lineNumber);

            sy_assert(token.tag() != TokenType::Error, "Unexpected token error");

            bigTokens[tokenIter] = token;
            bigEnds[tokenIter] = newStart;
            bigLineNumbers[tokenIter] = lineNumber;
            tokenIter += 1;
            start = newStart;

            if (token.tag() == TokenType::EndOfFile || tokenIter == source.len()) {
                keepFinding = false;
            }
        }
    }

    // Now shrink the allocation to not use insane amounts of memory
    // Doesn't need to be specially aligned as the tokens will be used for reading only,
    // so no real false sharing.

    Token* smallTokens;
    uint32_t* smallEnds;
    uint32_t* smallLineNumbers;
    {
        auto smallMemRes = self.alloc_.allocArray<uint8_t>(tokenizerAllocBytes(tokenIter));
        if (smallMemRes.hasErr()) {
            uint8_t* baseBigMem = reinterpret_cast<uint8_t*>(bigTokens);
            self.alloc_.freeArray(baseBigMem, totalBigBytes);
            return Error(ProgramError::OutOfMemory);
        }
        uint8_t* baseMem = smallMemRes.value();
        smallTokens = reinterpret_cast<Token*>(baseMem);
        smallEnds = reinterpret_cast<uint32_t*>(baseMem + (sizeof(Token) * tokenIter));
        smallLineNumbers =
            reinterpret_cast<uint32_t*>(baseMem + (sizeof(Token) * tokenIter) + (sizeof(uint32_t) * tokenIter));
    }

    for (uint32_t i = 0; i < tokenIter; i++) {
        smallTokens[i] = bigTokens[i];
    }
    for (uint32_t i = 0; i < tokenIter; i++) {
        smallEnds[i] = bigEnds[i];
    }
    for (uint32_t i = 0; i < tokenIter; i++) {
        smallLineNumbers[i] = bigLineNumbers[i];
    }
    self.tokens_ = smallTokens;
    self.ends_ = smallEnds;
    self.lineNumbers_ = smallLineNumbers;
    self.len_ = tokenIter;

    self.alloc_.freeArray(reinterpret_cast<uint8_t*>(bigTokens), totalBigBytes);

    return self;
}

TokenIter Tokenizer::iter() const noexcept { return TokenIter(this); }

TokenIter::TokenIter(const Tokenizer* tokenizer) noexcept : current_(nullptr), tokenizer_(tokenizer) {
    if (tokenizer_->tokens_ != nullptr) {
        current_ = tokenizer->tokens_ - 1;
    }
}

std::optional<Token> TokenIter::next() noexcept {
    sy_assert(this->current_ != nullptr, "Invalid iterator");

    const Token* endToken = &this->tokenizer_->tokens_[this->tokenizer_->len_];
    if ((this->current_ + 1) == endToken) {
        this->current_ = nullptr;
        return std::nullopt;
    }

    this->current_ += 1;
    return std::optional<Token>(*this->current_);
}

Token TokenIter::current() const noexcept {
    sy_assert(this->current_ != nullptr, "Invalid iterator");
    return *this->current_;
}

std::optional<Token> TokenIter::peek() const noexcept {
    sy_assert(this->current_ != nullptr, "Invalid iterator");

    const Token* endToken = &this->tokenizer_->tokens_[this->tokenizer_->len_];
    if ((this->current_ + 1) == endToken) {
        return std::nullopt;
    }
    return std::optional<Token>(this->current_[1]);
}

uint32_t TokenIter::currentEnd() const noexcept {
    sy_assert(this->current_ != nullptr, "Invalid iterator");
    const ptrdiff_t diff = this->current_ - this->tokenizer_->tokens_;
    return this->tokenizer_->ends_[diff];
}

uint32_t sy::TokenIter::currentLineNumber() const noexcept {
    sy_assert(this->current_ != nullptr, "Invalid iterator");
    const ptrdiff_t diff = this->current_ - this->tokenizer_->tokens_;
    return this->tokenizer_->lineNumbers_[diff];
}

sy::StringSlice TokenIter::currentSlice() const noexcept {
    const Token cur = this->current();
    const uint32_t start = cur.location();
    const uint32_t end = this->currentEnd();

    const sy::StringSlice fullSlice = this->tokenizer_->source_;
    sy_assert(fullSlice.len() >= end, "Out of bounds access");
    return sy::StringSlice(&fullSlice.data()[start], end - start);
}

StringSlice sy::TokenIter::source() const noexcept { return this->tokenizer_->source(); }

SourceFileLocation sy::TokenIter::sourceFileLocation() const noexcept {
    return SourceFileLocation(this->source(), this->current().location());
}

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"
#include <cstring>

TEST_CASE("one token") {
    auto result = Tokenizer::create(Allocator(), "const");
    CHECK(result);
    const Tokenizer tokenizer = result.takeValue();
    TokenIter iter = tokenizer.iter();

    auto first = iter.next();
    CHECK(first.has_value());
    CHECK_EQ(first.value().tag(), TokenType::ConstKeyword);
    CHECK_EQ(first.value().location(), 0);

    auto second = iter.next();
    CHECK(second.has_value());
    CHECK_EQ(second.value().tag(), TokenType::EndOfFile);

    auto third = iter.next();
    CHECK_FALSE(third.has_value());
}

TEST_CASE("location correct") {
    auto result = Tokenizer::create(Allocator(), " const");
    CHECK(result);
    const Tokenizer tokenizer = result.takeValue();
    TokenIter iter = tokenizer.iter();

    auto first = iter.next();
    CHECK(first.has_value());
    CHECK_EQ(first.value().tag(), TokenType::ConstKeyword);
    CHECK_EQ(first.value().location(), 1);

    auto second = iter.next();
    CHECK(second.has_value());
    CHECK_EQ(second.value().tag(), TokenType::EndOfFile);

    auto third = iter.next();
    CHECK_FALSE(third.has_value());
}

TEST_CASE("more tokens") {
    auto result = Tokenizer::create(Allocator(), " hello mut;1 pub true,i64");
    CHECK(result);
    const Tokenizer tokenizer = result.takeValue();
    TokenIter iter = tokenizer.iter();

    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::Identifier);
        CHECK_EQ(curr.value().location(), 1);
        const auto identifierStr = iter.currentSlice();
        CHECK_EQ(std::strncmp(identifierStr.data(), "hello", identifierStr.len()), 0);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::MutKeyword);
        CHECK_EQ(curr.value().location(), 7);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::SemicolonSymbol);
        CHECK_EQ(curr.value().location(), 10);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::NumberLiteral);
        CHECK_EQ(curr.value().location(), 11);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::PubKeyword);
        CHECK_EQ(curr.value().location(), 13);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::TrueKeyword);
        CHECK_EQ(curr.value().location(), 17);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::CommaSymbol);
        CHECK_EQ(curr.value().location(), 21);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::I64Primitive);
        CHECK_EQ(curr.value().location(), 22);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::EndOfFile);
    }
    {
        auto curr = iter.next();
        CHECK_FALSE(curr.has_value());
    }
}

TEST_CASE("parse i8") {
    uint32_t lineNumber = 0;
    const StringSlice source = "i8,";
    auto [token, end] = Token::parseToken(source, 0, &lineNumber);
    CHECK_EQ(token.tag(), TokenType::I8Primitive);
    CHECK_EQ(end, 2);
    auto [comma, newEnd] = Token::parseToken(source, end, &lineNumber);
    CHECK_EQ(comma.tag(), TokenType::CommaSymbol);
}

TEST_CASE("function arguments") {
    Allocator alloc;
    const Tokenizer tokenizer = Tokenizer::create(alloc, "(arg1: i8, mut arg2: u64)").takeValue();
    TokenIter iter = tokenizer.iter();

    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::LeftParenthesesSymbol);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::Identifier);
        CHECK_EQ(iter.currentSlice(), "arg1");
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::ColonSymbol);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::I8Primitive);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::CommaSymbol);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::MutKeyword);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::Identifier);
        CHECK_EQ(iter.currentSlice(), "arg2");
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::ColonSymbol);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::U64Primitive);
    }
    {
        auto curr = iter.next();
        CHECK(curr.has_value());
        CHECK_EQ(curr.value().tag(), TokenType::RightParenthesesSymbol);
    }
}

#endif // SYNC_LIB_NO_TESTS