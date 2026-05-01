//! API
#pragma once
#ifndef SY_TYPES_STRING_STRING_SLICE_HPP_
#define SY_TYPES_STRING_STRING_SLICE_HPP_

#include "../../core/core.h"
#include <iosfwd>
#include <memory> // some header for hash
#include <string_view>

namespace sy {
class SY_API StringSlice {
  public:
    constexpr StringSlice() = default;

    constexpr StringSlice(const StringSlice&) = default;
    constexpr StringSlice& operator=(const StringSlice&) = default;
    constexpr StringSlice(StringSlice&&) = default;
    constexpr StringSlice& operator=(StringSlice&&) = default;

    template <size_t N> constexpr StringSlice(char const (&inStr)[N]) : _ptr(inStr), _len(N - 1) {}

    /// @param inLen Length of the string in bytes, not including null terminator
    /// @param inPtr Valid utf8 string. Does not need to be null terminated.
    StringSlice(const char* ptr, size_t len);

    [[nodiscard]] const char* data() const { return _ptr; }

    [[nodiscard]] size_t len() const { return _len; }

    char operator[](const size_t index) const;

    bool operator==(const StringSlice& other) const;
    bool operator!=(const StringSlice& other) const { return !(*this == other); }

    size_t hash() const;

    [[nodiscard]] operator std::string_view() const noexcept {
        return std::string_view(this->_ptr, this->_len);
    }

    friend std::ostream& operator<<(std::ostream& os, const StringSlice& s);

  private:
    /// Must be UTF8. Does not have to be null terminated. Is not read from if `len == 0`.
    const char* _ptr = nullptr;
    /// Does not include possible null terminator. Is measured in bytes.
    size_t _len = 0;
};
} // namespace sy

namespace std {
template <> struct hash<sy::StringSlice> {
    size_t operator()(const sy::StringSlice& obj) const { return obj.hash(); }
};
} // namespace std

#endif // SY_TYPES_STRING_STRING_SLICE_HPP_
