#pragma once
#ifndef SY_COMPILER_TOKENIZER_TOKENIZER_HPP_
#define SY_COMPILER_TOKENIZER_TOKENIZER_HPP_

#include "../../core.h"
#include "token.hpp"
#include "../../mem/allocator.hpp"
#include "../compile_info.hpp"
#include <variant>

class Tokenizer {
public:

    ~Tokenizer();

    Tokenizer(Tokenizer&& other);

    Tokenizer& operator=(Tokenizer&& other);

    static std::variant<Tokenizer, sy::CompileError> create(sy::Allocator allocator, sy::StringSlice source);

private:

    Tokenizer(sy::Allocator allocator) : alloc_(allocator) {}

private:
    sy::Allocator alloc_;
    const Token* tokens_ = nullptr;
    uint32_t len_ = 0;
};

#endif //SY_COMPILER_TOKENIZER_TOKENIZER_HPP_
