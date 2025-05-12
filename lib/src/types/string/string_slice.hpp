//! API
#pragma once
#ifndef SY_TYPES_STRING_STRING_SLICE_HPP_
#define SY_TYPES_STRING_STRING_SLICE_HPP_

#include "../../core.h"

namespace sy {
    namespace c {
        #include "string_slice.h"
    }

    class SY_API StringSlice {
    public:
        StringSlice() = default;

        StringSlice(const StringSlice&) = default;
        StringSlice& operator=(const StringSlice&) = default;
        StringSlice(StringSlice&&) = default;
        StringSlice& operator=(StringSlice&&) = default;

        template<size_t N>
        StringSlice(char const (&inStr)[N]) : _inner{inStr, N - 1} {}

        [[nodiscard]] const char* data() const { return _inner.ptr; }

        [[nodiscard]] size_t len() const { return _inner.len; }

    private:
        c::SyStringSlice _inner = {0, 0};
    };
}

#endif // SY_TYPES_STRING_STRING_SLICE_HPP_