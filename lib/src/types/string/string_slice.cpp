#include "string_slice.h"
#include "string_slice.hpp"
#include "../../util/assert.hpp"

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
            const char c = str[i];
            if (c == 0) {
                return false;
            }
            else if ((c & asciiZeroBit) == 0) {
                i += 1;
            }
            else if ((c & twoByteBitmask) == twoByteCodePoint) {
                if ((str[i + 1] & trailingBytesBitmask) != trailingBytesCodePoint) {
                    return false;
                }
                i += 2;
            }
            else if ((c & threeByteBitmask) == threeByteCodePoint) {
                if ((str[i + 1] & trailingBytesBitmask) != trailingBytesCodePoint) {
                    return false;
                }
                if ((str[i + 2] & trailingBytesBitmask) != trailingBytesCodePoint) {
                    return false;
                }
                i += 3;
            }
            else if ((c & fourByteBitmask) == fourByteCodePoint) {
                if ((str[i + 1] & trailingBytesBitmask) != trailingBytesCodePoint) {
                    return false;
                }
                if ((str[i + 2] & trailingBytesBitmask) != trailingBytesCodePoint) {
                    return false;
                }
                if ((str[i + 3] & trailingBytesBitmask) != trailingBytesCodePoint) {
                    return false;
                }
                i += 4;
            }
            else {
                return false;
            }
        }
        return true;
    }
}

sy::StringSlice::StringSlice(const char *ptr, size_t len)
    : _inner{ptr, len}
{
    sy_assert(sliceValidUtf8(*this), "Invalid utf8 string slice");
}
