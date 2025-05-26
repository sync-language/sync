//! API
#pragma once
#ifndef SY_TYPES_STRING_STRING_SLICE_HPP_
#define SY_TYPES_STRING_STRING_SLICE_HPP_

#include "../../core.h"

namespace sy {
    namespace c {
        #include "string_slice.h"

        using SyStringSlice = SyStringSlice;
    }

    class SY_API StringSlice {
    public:
        StringSlice() = default;

        StringSlice(const StringSlice&) = default;
        StringSlice& operator=(const StringSlice&) = default;
        StringSlice(StringSlice&&) = default;
        StringSlice& operator=(StringSlice&&) = default;

        template<size_t N>
        constexpr StringSlice(char const (&inStr)[N]) : _inner{inStr, N - 1} {}

        /// @param len Length of the string in bytes, not including null terminator
        /// @param ptr Valid utf8 string. Does not need to be null terminated.
        StringSlice(const char* ptr, size_t len);

        [[nodiscard]] const char* data() const { return _inner.ptr; }

        [[nodiscard]] size_t len() const { return _inner.len; }

        char operator[](const size_t index) const;

    private:
        c::SyStringSlice _inner = {0, 0};
    };
}

#endif // SY_TYPES_STRING_STRING_SLICE_HPP_