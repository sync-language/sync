#pragma once
#ifndef SY_COMPILER_TOKENIZER_TOKENIZER_HPP_
#define SY_COMPILER_TOKENIZER_TOKENIZER_HPP_

#include "../../core.h"
#include "token.hpp"
#include "../../mem/allocator.hpp"

class Tokenizer {
public:

    static sy::AllocExpect<Tokenizer> create(sy::Allocator allocator, sy::StringSlice source);

private:

    Tokenizer(sy::Allocator allocator);

private:
    sy::Allocator alloc_;
    Token* tokens_ = nullptr;
    uint32_t len_ = 0;
    uint32_t capacity_ = 0;
};

#endif //SY_COMPILER_TOKENIZER_TOKENIZER_HPP_
