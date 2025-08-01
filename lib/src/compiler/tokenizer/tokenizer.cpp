#include "tokenizer.hpp"
#include "../../util/assert.hpp"
#include "../../threading/alloc_cache_align.hpp"

using namespace sy;

static_assert(sizeof(Token) == sizeof(uint32_t));

Tokenizer::~Tokenizer()
{
    if(len_ == 0) return;

    Token* tokens = const_cast<Token*>(this->tokens_);
    alloc_.freeArray(tokens, this->len_);

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
    auto bigMemRes = self.alloc_.allocAlignedArray<Token>(capacity, ALLOC_CACHE_ALIGN);
    if(bigMemRes.hasValue() == false) {
        return std::variant<Tokenizer, CompileError>(CompileError::createOutOfMemory());
    }
    Token* bigTokens = bigMemRes.value();

    uint32_t start = 0;
    uint32_t tokenIter = 0;
    Token previous = Token(TokenType::Error, 0);
    bool keepFinding = true;
    while(keepFinding) {
        sy_assert(tokenIter < source.len(), "Infinite loop detected");

        auto [token, newStart] = Token::parseToken(source, start, previous);

        sy_assert(token.tag() != TokenType::Error, "Unexpected token error");

        bigTokens[tokenIter] = token;
        tokenIter += 1;
        start = newStart;
        
        previous = token;
        if(token.tag() == TokenType::EndOfFile) {
            keepFinding = false;
        }
    }

    // Now shrink the allocation to not use insane amounts of memory
    // Doesn't need to be specially aligned as the tokens will be used for reading only,
    // so no real false sharing.
    auto shrunkMemRes = self.alloc_.allocArray<Token>(tokenIter);
    if(shrunkMemRes.hasValue() == false) {
        self.alloc_.freeArray(bigTokens, capacity);
        return std::variant<Tokenizer, CompileError>(CompileError::createOutOfMemory());
    }
    Token* smallTokens = shrunkMemRes.value();
    for(uint32_t i = 0; i < tokenIter; i++) {
        smallTokens[i] = bigTokens[i];
    }
    self.tokens_ = smallTokens;
    self.len_ = tokenIter;

    self.alloc_.freeArray(bigTokens, capacity);

    return std::variant<Tokenizer, CompileError>(std::move(self));
}

Token TokenIter::next()
{
    const Token* endToken = &this->tokenizer_->tokens_[this->tokenizer_->len_];
    sy_assert((this->current_ + 1) != endToken, "Out of bounds token access");

    this->current_ += 1;
    return *this->current_;
}

Token TokenIter::peek() const
{
    const Token* endToken = &this->tokenizer_->tokens_[this->tokenizer_->len_];
    sy_assert((this->current_ + 1) != endToken, "Out of bounds token access");
    return this->current_[1];
}
