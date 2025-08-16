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
    : alloc_(other.alloc_), tokens_(other.tokens_), len_(other.len_)
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
    this->tokens_ = other.tokens_;
    this->len_ = other.len_;
    other.tokens_ = nullptr;
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
