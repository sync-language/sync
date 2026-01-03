#pragma once
#ifndef SY_COMPILER_TOKENIZER_FILE_LITERALS_HPP_
#define SY_COMPILER_TOKENIZER_FILE_LITERALS_HPP_

#include "../../core/core.h"
#include "../../mem/allocator.hpp"
#include "../../program/program_error.hpp"
#include "../../types/result/result.hpp"
#include "../../types/string/char.hpp"
#include "../../types/string/string.hpp"
#include "../../types/string/string_slice.hpp"
#include <type_traits>
#include <variant>

namespace sy {

// TODO support 0x (hex), and 0b (binary). Maybe also scientific notation?
class NumberLiteral {
  public:
    enum class RepKind { Unsigned64, Signed64, Float64 };

    [[nodiscard]] static Result<NumberLiteral, sy::ProgramError> create(const sy::StringSlice source,
                                                                        const uint32_t start, const uint32_t end);

    [[nodiscard]] Result<uint64_t, sy::ProgramError> asUnsigned64() const;

    [[nodiscard]] Result<int64_t, sy::ProgramError> asSigned64() const;

    [[nodiscard]] double asFloat64() const;

  private:
    union Rep {
        uint64_t unsigned64;
        int64_t signed64;
        double float64;
    };

    RepKind kind_;
    Rep rep_;
};

class CharLiteral {
  public:
    [[nodiscard]] static Result<CharLiteral, sy::ProgramError> create(const sy::StringSlice source,
                                                                      const uint32_t start, const uint32_t end);

    sy::Char val;
};

class StringLiteral {
  public:
    [[nodiscard]] static Result<StringLiteral, sy::ProgramError>
    create(const sy::StringSlice source, const uint32_t start, const uint32_t end, sy::Allocator alloc);

    StringLiteral() = default;

    StringLiteral(StringLiteral&& other) noexcept;

    StringLiteral& operator=(StringLiteral&& other) noexcept;

    ~StringLiteral() noexcept;

    sy::StringUnmanaged str;
    sy::Allocator alloc;
};
} // namespace sy

#endif // SY_COMPILER_TOKENIZER_FILE_LITERALS_HPP_
