#pragma once
#ifndef SY_COMPILER_TOKENIZER_FILE_LITERALS_HPP_
#define SY_COMPILER_TOKENIZER_FILE_LITERALS_HPP_

#include "../../core.h"
#include "../../types/string/char.hpp"
#include "../../types/string/string_slice.hpp"
#include "../compile_info.hpp"
#include <variant>
#include <type_traits>

class NumberLiteral {
    enum class RepKind {
        Unsigned64,
        Signed64,
        Float64
    };

    [[nodiscard]] std::variant<uint64_t, sy::CompileError> asUnsigned64() const;
    
    [[nodiscard]] std::variant<int64_t, sy::CompileError> asSigned64() const;

    [[nodiscard]] std::variant<double, sy::CompileError> asFloat64() const;

private:

    union Rep {
        uint64_t    unsigned64;
        int64_t     signed64;
        double      float64;
    };

    RepKind kind_;
    Rep     rep_;
};

class CharLiteral {
public:
    sy::Char val;
};

class StringLiteral {
public:
    char* ptr;
    size_t len;
    size_t capacity;
};

#endif // SY_COMPILER_TOKENIZER_FILE_LITERALS_HPP_
