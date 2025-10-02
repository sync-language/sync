#pragma once
#ifndef SY_COMPILER_TOKENIZER_TOKENIZER_HPP_
#define SY_COMPILER_TOKENIZER_TOKENIZER_HPP_

#include "../../core.h"
#include "../../mem/allocator.hpp"
#include "../../types/result/result.hpp"
#include "../compile_info.hpp"
#include "token.hpp"
#include <optional>
#include <variant>

namespace sy {
class TokenIter;

class Tokenizer {
  public:
    ~Tokenizer() noexcept;

    Tokenizer(Tokenizer&& other) noexcept;

    Tokenizer& operator=(Tokenizer&& other) noexcept;

    Tokenizer(const Tokenizer& other) = delete;

    Tokenizer& operator=(const Tokenizer& other) = delete;

    static Result<Tokenizer, sy::CompileError> create(sy::Allocator allocator, sy::StringSlice source) noexcept;

    TokenIter iter() const noexcept;

    StringSlice source() const { return this->source_; }

  private:
    Tokenizer(sy::Allocator allocator) : alloc_(allocator) {}

  private:
    friend class TokenIter;

    sy::Allocator alloc_;
    sy::StringSlice source_{};
    /// Tokens found within the source, in order of appearance. When used with
    /// `Tokenizer::ends_` member, also specifies the range. Uses struct of
    /// arrays for better cache utilization, since most tokens don't need a
    /// range.
    const Token* tokens_ = nullptr;
    /// The end character index of the tokens found within the source, in order
    /// of appearance. For many tokens this won't matter, but for literals
    /// and identifiers, it's very useful to figure out how many characters
    /// the token spans. Uses struct of arrays for better cache utilization,
    /// since most tokens don't need a range.
    const uint32_t* ends_ = nullptr;
    /// The line numbers of the tokens found within the source, in order
    /// of appearance.
    const uint32_t* lineNumbers_ = nullptr;
    uint32_t len_ = 0;
};

class TokenIter {
  public:
    TokenIter() = default;

    TokenIter(const Tokenizer* tokenizer) noexcept;

    /// @brief Steps forward the iterator by one token. If there are no more
    /// tokens, the iterator will invalidate itself and return `std::nullopt`.
    /// @return The next token, or `std::nullopt` if there is no next token.
    /// If this is the first call to `next()`, returns the first element in
    /// the iterator.
    [[nodiscard]] std::optional<Token> next() noexcept;

    /// @return The token at the current iterator position.
    [[nodiscard]] Token current() const noexcept;

    /// @return The token after the current one, or `std::nullopt` .
    /// if there is no next token.
    [[nodiscard]] std::optional<Token> peek() const noexcept;

    /// @brief Some tokens, notably literals and identifiers, need to know how
    /// many characters they span so that their data can be parsed out.
    /// @return The end index (exclusive) of the token found in the source text.
    [[nodiscard]] uint32_t currentEnd() const noexcept;

    /// @brief Some tokens, notably literals and identifiers, need to know how
    /// many characters they span so that their data can be parsed out.
    /// @return The end index (exclusive) of the token found in the source text.
    [[nodiscard]] uint32_t currentLineNumber() const noexcept;

    /// @return A string slice of the current token.
    [[nodiscard]] sy::StringSlice currentSlice() const noexcept;

    [[nodiscard]] StringSlice source() const noexcept;

  private:
    const Token* current_ = nullptr;
    const Tokenizer* tokenizer_ = nullptr;
};
} // namespace sy

#endif // SY_COMPILER_TOKENIZER_TOKENIZER_HPP_
