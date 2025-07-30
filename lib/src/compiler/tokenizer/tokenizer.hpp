#pragma once
#ifndef SY_COMPILER_TOKENIZER_TOKENIZER_HPP_
#define SY_COMPILER_TOKENIZER_TOKENIZER_HPP_

#include "../../core.h"
#include "token.hpp"
#include "../../mem/allocator.hpp"

class Tokenizer {
public:

    Tokenizer(sy::Allocator allocator, sy::StringSlice source);

private:
    sy::Allocator alloc_;
    Token* tokens_;
    uint32_t len_;
    uint32_t capacity_;
};

#endif //SY_COMPILER_TOKENIZER_TOKENIZER_HPP_
