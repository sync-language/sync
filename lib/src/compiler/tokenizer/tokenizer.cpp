#include "tokenizer.hpp"
#include "../../util/assert.hpp"
#include "../../threading/alloc_cache_align.hpp"

using namespace sy;

static_assert(sizeof(Token) == sizeof(uint32_t));

Tokenizer::~Tokenizer()
{
    if(len_ == 0) return;

    Token* tokens = const_cast<Token*>(this->tokens_);
    uint32_t* ends = const_cast<uint32_t*>(this->ends_);
    alloc_.freeArray(tokens, this->len_);
    alloc_.freeArray(ends, this->len_);

    this->tokens_ = nullptr;
    this->len_ = 0;
}

Tokenizer::Tokenizer(Tokenizer &&other)
    : alloc_(other.alloc_),
    source_(other.source_),
    tokens_(other.tokens_), 
    ends_(other.ends_), 
    len_(other.len_)
{
    other.tokens_ = nullptr;
    other.len_ = 0;
}

Tokenizer &Tokenizer::operator=(Tokenizer &&other)
{
    if(this->len_ != 0) {
        Token* tokens = const_cast<Token*>(this->tokens_);
        alloc_.freeArray(tokens, this->len_);
    }

    this->alloc_ = other.alloc_;
    this->source_ = other.source_;
    this->tokens_ = other.tokens_;
    this->ends_ = other.ends_;
    this->len_ = other.len_;
    other.tokens_ = nullptr;
    other.ends_ = nullptr;
    other.len_ = 0;
    return *this;
}

std::variant<Tokenizer, CompileError> Tokenizer::create(Allocator allocator, StringSlice source)
{
    Tokenizer self(allocator);
    self.source_ = source;

    if(source.len() > MAX_SOURCE_LEN) {
        CompileError::FileTooBig fileTooBig;
        fileTooBig.fileSize = source.len();
        fileTooBig.maxFileSize = MAX_SOURCE_LEN;
        return std::variant<Tokenizer, CompileError>(CompileError::createFileTooBig(fileTooBig));
    }

    // The total amount of tokens will always be less than or equal 
    // to the number of bytes in the source
    // As a result, we can over-allocate upfront.
    const size_t capacity = source.len();
    auto bigTokenMemRes = self.alloc_.allocAlignedArray<Token>(capacity, ALLOC_CACHE_ALIGN);
    if(bigTokenMemRes.hasValue() == false) {
        return std::variant<Tokenizer, CompileError>(CompileError::createOutOfMemory());
    }
    Token* bigTokens = bigTokenMemRes.value();
    auto bigEndsMemRes = self.alloc_.allocAlignedArray<uint32_t>(capacity, ALLOC_CACHE_ALIGN);
    if(bigTokenMemRes.hasValue() == false) {
        self.alloc_.freeAlignedArray(bigTokens, capacity, ALLOC_CACHE_ALIGN);
        return std::variant<Tokenizer, CompileError>(CompileError::createOutOfMemory());
    }
    uint32_t* bigEnds = bigEndsMemRes.value();

    uint32_t start = 0;
    uint32_t tokenIter = 0;
    bool keepFinding = true;
    while(keepFinding) {
        sy_assert(tokenIter < source.len(), "Infinite loop detected");

        auto [token, newStart] = Token::parseToken(source, start);

        sy_assert(token.tag() != TokenType::Error, "Unexpected token error");

        bigTokens[tokenIter] = token;
        bigEnds[tokenIter] = newStart;
        tokenIter += 1;
        start = newStart;
        
        if(token.tag() == TokenType::EndOfFile) {
            keepFinding = false;
        }
    }

    // Now shrink the allocation to not use insane amounts of memory
    // Doesn't need to be specially aligned as the tokens will be used for reading only,
    // so no real false sharing.
    auto shrunkTokenMemRes = self.alloc_.allocArray<Token>(tokenIter);
    if(shrunkTokenMemRes.hasValue() == false) {
        self.alloc_.freeAlignedArray(bigTokens, capacity, ALLOC_CACHE_ALIGN);
        self.alloc_.freeAlignedArray(bigEnds, capacity, ALLOC_CACHE_ALIGN);
        return std::variant<Tokenizer, CompileError>(CompileError::createOutOfMemory());
    }
    Token* smallTokens = shrunkTokenMemRes.value();
    auto shrunkEndsMemRes = self.alloc_.allocAlignedArray<uint32_t>(capacity, ALLOC_CACHE_ALIGN);
    if(shrunkEndsMemRes.hasValue() == false) {
        self.alloc_.freeAlignedArray(bigTokens, capacity, ALLOC_CACHE_ALIGN);
        self.alloc_.freeAlignedArray(bigEnds, capacity, ALLOC_CACHE_ALIGN);
        self.alloc_.freeArray(smallTokens, capacity);
        return std::variant<Tokenizer, CompileError>(CompileError::createOutOfMemory());
    }
    uint32_t* smallEnds = shrunkEndsMemRes.value();

    for(uint32_t i = 0; i < tokenIter; i++) {
        smallTokens[i] = bigTokens[i];
    }
    for(uint32_t i = 0; i < tokenIter; i++) {
        smallEnds[i] = bigEnds[i];
    }
    self.tokens_ = smallTokens;
    self.ends_ = smallEnds;
    self.len_ = tokenIter;

    self.alloc_.freeAlignedArray(bigTokens, capacity, ALLOC_CACHE_ALIGN);
    self.alloc_.freeAlignedArray(bigEnds, capacity, ALLOC_CACHE_ALIGN);

    return std::variant<Tokenizer, CompileError>(std::move(self));
}

TokenIter Tokenizer::iter() const noexcept
{
    return TokenIter(this);
}

TokenIter::TokenIter(const Tokenizer *tokenizer)
    : current_(tokenizer->tokens_ - 1), tokenizer_(tokenizer)
{}

std::optional<Token> TokenIter::next()
{
    sy_assert(this->current_ != nullptr, "Invalid iterator");

    const Token* endToken = &this->tokenizer_->tokens_[this->tokenizer_->len_];
    if((this->current_ + 1) == endToken) {
        this->current_ = nullptr;
        return std::nullopt;
    }

    this->current_ += 1;
    return std::optional<Token>(*this->current_);
}

Token TokenIter::current() const
{
    sy_assert(this->current_ != nullptr, "Invalid iterator");
    return *this->current_;
}

std::optional<Token> TokenIter::peek() const
{
    sy_assert(this->current_ != nullptr, "Invalid iterator");

    const Token* endToken = &this->tokenizer_->tokens_[this->tokenizer_->len_];
    if((this->current_ + 1) == endToken) {
        return std::nullopt;
    }
    return std::optional<Token>(this->current_[1]);
}

uint32_t TokenIter::currentEnd() const
{
    sy_assert(this->current_ != nullptr, "Invalid iterator");
    const ptrdiff_t diff = this->current_ - this->tokenizer_->tokens_;
    return this->tokenizer_->ends_[diff];
}

sy::StringSlice TokenIter::currentSlice() const
{
    const Token cur = this->current();
    const uint32_t start = cur.location();
    const uint32_t end = this->currentEnd();

    const sy::StringSlice fullSlice = this->tokenizer_->source_;
    sy_assert(fullSlice.len() >= end, "Out of bounds access");
    return sy::StringSlice(&fullSlice.data()[start], end - start);
}

#ifndef SYNC_LIB_NO_TESTS

#include "../../doctest.h"
#include <cstring>

TEST_CASE("one token") {
    auto result = Tokenizer::create(Allocator(), "const");
    CHECK(std::holds_alternative<Tokenizer>(result));
    const Tokenizer tokenizer = std::get<Tokenizer>(std::move(result));
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
    CHECK(std::holds_alternative<Tokenizer>(result));
    const Tokenizer tokenizer = std::get<Tokenizer>(std::move(result));
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
    CHECK(std::holds_alternative<Tokenizer>(result));
    const Tokenizer tokenizer = std::get<Tokenizer>(std::move(result));
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

#endif // SYNC_LIB_NO_TESTS