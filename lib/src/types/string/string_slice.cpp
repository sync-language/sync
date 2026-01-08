#include "string_slice.h"
#include "../../core/core_internal.h"
#include "string_slice.hpp"
#include <string_view>

using sy::StringSlice;

extern "C" {
bool sliceValidUtf8(const sy::StringSlice slice) {
    const char* str = slice.data();
    const size_t len = slice.len();

    const uint8_t asciiZeroBit = 0b10000000;
    const uint8_t trailingBytesBitmask = 0b11000000;
    const uint8_t trailingBytesCodePoint = 0b10000000;
    const uint8_t twoByteCodePoint = 0b11000000;
    const uint8_t twoByteBitmask = 0b11100000;
    const uint8_t threeByteCodePoint = 0b11100000;
    const uint8_t threeByteBitmask = 0b11110000;
    const uint8_t fourByteCodePoint = 0b11110000;
    const uint8_t fourByteBitmask = 0b11111000;

    size_t i = 0;
    while (i < len) {
        const uint8_t c = static_cast<uint8_t>(str[i]);
        if (c == 0) {
            return false;
        } else if ((c & asciiZeroBit) == 0) {
            i += 1;
        } else if ((c & twoByteBitmask) == twoByteCodePoint) {
            if (i + 1 >= len) {
                return false;
            }
            if ((static_cast<uint8_t>(str[i + 1]) & trailingBytesBitmask) != trailingBytesCodePoint) {
                return false;
            }
            i += 2;
        } else if ((c & threeByteBitmask) == threeByteCodePoint) {
            if (i + 2 >= len) {
                return false;
            }
            if ((static_cast<uint8_t>(str[i + 1]) & trailingBytesBitmask) != trailingBytesCodePoint) {
                return false;
            }
            if ((static_cast<uint8_t>(str[i + 2]) & trailingBytesBitmask) != trailingBytesCodePoint) {
                return false;
            }
            i += 3;
        } else if ((c & fourByteBitmask) == fourByteCodePoint) {
            if (i + 3 >= len) {
                return false;
            }
            if ((static_cast<uint8_t>(str[i + 1]) & trailingBytesBitmask) != trailingBytesCodePoint) {
                return false;
            }
            if ((static_cast<uint8_t>(str[i + 2]) & trailingBytesBitmask) != trailingBytesCodePoint) {
                return false;
            }
            if ((static_cast<uint8_t>(str[i + 3]) & trailingBytesBitmask) != trailingBytesCodePoint) {
                return false;
            }
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}
}

sy::StringSlice::StringSlice(const char* inPtr, size_t inLen) : _ptr(inPtr), _len(inLen) {
    // Annoying private member stuff
    static_assert(offsetof(StringSlice, _ptr) == offsetof(SyStringSlice, ptr));
    static_assert(offsetof(StringSlice, _len) == offsetof(SyStringSlice, len));

    sy_assert(sliceValidUtf8(*this), "Invalid utf8 string slice");
}

char sy::StringSlice::operator[](const size_t index) const {
    sy_assert(index < this->_len, "Index out of bounds");
    return this->_ptr[index];
}

bool sy::StringSlice::operator==(const StringSlice& other) const {
    if (this->_len != other._len)
        return false;

    if (this->_ptr == other._ptr)
        return true;

    for (size_t i = 0; i < this->_len; i++) {
        if (this->_ptr[i] != other._ptr[i])
            return false;
    }
    return true;
}

size_t sy::StringSlice::hash() const {
    std::string_view sv{this->_ptr, this->_len};
    std::hash<std::string_view> h;
    return h(sv);
}
