#pragma once
#ifndef SY_COMPILER_TOKENIZER_TOKENIZER_HPP_
#define SY_COMPILER_TOKENIZER_TOKENIZER_HPP_

#include "../../core.h"
#include "token.hpp"
#include "../../mem/allocator.hpp"
#include "../compile_info.hpp"
#include <variant>

class TokenIter;

class Tokenizer {
public:

    ~Tokenizer();

    Tokenizer(Tokenizer&& other);

    Tokenizer& operator=(Tokenizer&& other);

    static std::variant<Tokenizer, sy::CompileError> create(sy::Allocator allocator, sy::StringSlice source);

private:

    Tokenizer(sy::Allocator allocator) : alloc_(allocator) {}

private:
    friend class TokenIter;

    sy::Allocator alloc_;
    const Token* tokens_ = nullptr;
    uint32_t len_ = 0;
};

class TokenIter {
public:

    Token next();

    [[nodiscard]] Token current() const { return *current_; }

    [[nodiscard]] Token peek() const;

private:
    const Token*        current_;
    const Tokenizer*    tokenizer_;

};

#endif //SY_COMPILER_TOKENIZER_TOKENIZER_HPP_
