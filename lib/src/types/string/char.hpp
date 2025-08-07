//! API
#pragma once
#ifndef SY_TYPES_STRING_CHAR_HPP_
#define SY_TYPES_STRING_CHAR_HPP_

#include "../../core.h"

namespace sy {
    class Char {
        constexpr Char() = default;
        constexpr Char(char c) : val_(static_cast<uint32_t>(c)) {}

        [[nodiscard]] constexpr char cchar() const { return static_cast<char>(val_); }

    private:
        uint32_t val_ = 0;
    };
}

#endif // SY_TYPES_STRING_CHAR_HPP_