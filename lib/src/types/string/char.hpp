//! API
#pragma once
#ifndef SY_TYPES_STRING_CHAR_HPP_
#define SY_TYPES_STRING_CHAR_HPP_

#include "../../core/core.h"

namespace sy {
class Char {
  public:
    constexpr Char() = default;
    constexpr Char(char c) : val_(static_cast<uint32_t>(c)) {}

    [[nodiscard]] constexpr char cchar() const { return static_cast<char>(val_); }

    [[nodiscard]] constexpr bool operator==(const Char& other) const { return this->val_ == other.val_; }

  private:
    uint32_t val_ = 0;
};
} // namespace sy

#endif // SY_TYPES_STRING_CHAR_HPP_