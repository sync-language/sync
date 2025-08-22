#pragma once
#ifndef SY_UTIL_SIMD_HPP_
#define SY_UTIL_SIMD_HPP_

#include "../core.h"
#include <optional>
#include <type_traits>

#if defined(__AVX512BW__)
// _mm512_cmpeq_epi8_mask
constexpr size_t SUGGESTED_SIMD_WIDTH = 64;
#elif defined(__AVX2__)
// _mm256_cmpeq_epi8
// _mm256_movemask_epi8
constexpr size_t SUGGESTED_SIMD_WIDTH = 32;
#elif defined(__SSE2__)
// _mm_cmpeq_epi8
// _mm_movemask_epi8
constexpr size_t SUGGESTED_SIMD_WIDTH = 16;
#elif defined(__ARM_NEON__)
constexpr size_t SUGGESTED_SIMD_WIDTH = 16;
#else
constexpr size_t SUGGESTED_SIMD_WIDTH = alignof(uint64_t);
#endif

template<int Width>
class SimdMask;

namespace simd_detail {
    std::optional<uint32_t> countTrailingZeroes32(uint32_t mask);

    std::optional<uint32_t> countTrailingZeroes64(uint64_t mask);

    std::optional<uint32_t> firstZeroIndex8x16(const uint8_t* alignedPtr);
}

template<int Width>
class SimdMask {
public:
    using integer_t = 
        std::conditional_t<Width == 16, uint16_t, 
            std::conditional_t<Width == 32, uint32_t, uint64_t>>;

    integer_t mask;

    struct Iterator {

        bool operator!=(const Iterator& other) const { return this->index != other.index; }
        
        uint32_t operator*() const {
            return index;
        }

        Iterator& operator++() {
            this->calculateNewIndex();
            return *this;
        }

    private:
        friend class SimdMask;

        Iterator() : data(0), index(static_cast<uint32_t>(-1)) {}

        Iterator(integer_t inData) : data(inData) {
            this->calculateNewIndex();
        }

        void calculateNewIndex() {
            std::optional<uint32_t> optFound = [](integer_t d){
                if constexpr(std::is_same_v<integer_t, uint64_t>) {
                    return simd_detail::countTrailingZeroes64(d);
                } else {
                    return simd_detail::countTrailingZeroes32(d);
                }
            }(this->data);

            if(!optFound.has_value()) {
                data = 0;
                index = static_cast<uint32_t>(-1);
            } else {
                index = optFound.value();
                if constexpr(std::is_same_v<integer_t, uint64_t>) {
                    data = data & (~1ULL << index);
                } else {
                    data = data & (~1U << index);
                }
            }
        }

        uint32_t index;
        integer_t data;
    };

    SimdMask(integer_t inMask) : mask(inMask) {}

    Iterator begin() const { return Iterator(mask); }

    Iterator end() const { return Iterator(); }
};

namespace simd_detail {
    SimdMask<16> equalMask8x16(const uint8_t* alignedPtr, uint8_t value);

    SimdMask<32> equalMask8x32(const uint8_t* alignedPtr, uint8_t value);

    SimdMask<64> equalMask8x64(const uint8_t* alignedPtr, uint8_t value);
}

template<int Width>
class alignas(Width) ByteSimd final {
public:

    static_assert(Width == 16 || Width == 32 || Width == 64, 
        "Byte simd width must be 16, 32, or 64");

    uint8_t bytes[Width]{};

    ByteSimd() = default;

    ByteSimd(uint8_t fill);

    ByteSimd(const uint8_t* inBytes);

    std::optional<uint32_t> firstZeroIndex() const;

    SimdMask<Width> equalMask(const uint8_t value) const;
};

template <int Width>
inline ByteSimd<Width>::ByteSimd(uint8_t fill)
{    
    for(int i = 0; i < Width; i++) {
        this->bytes[i] = fill;
    }
}

template <int Width>
inline ByteSimd<Width>::ByteSimd(const uint8_t *inBytes)
{
    for(int i = 0; i < Width; i++) {
        this->bytes[i] = inBytes[i];
    }
}

template <int Width>
inline std::optional<uint32_t> ByteSimd<Width>::firstZeroIndex() const
{
    if constexpr (Width == 16) {
        return simd_detail::firstZeroIndex8x16(this->bytes);
    } 
    else if constexpr (Width == 32) {
        { // first 16
            auto result = simd_detail::firstZeroIndex8x16(this->bytes);
            if(result.has_value()) return result;
        }
        { // second 16
            auto result = simd_detail::firstZeroIndex8x16(&this->bytes[16]);
            if(result.has_value()) return std::optional<uint32_t>(result.value() + 16);
            return std::nullopt;
        }
    }
    else {
        { // first 16
            auto result = simd_detail::firstZeroIndex8x16(this->bytes);
            if(result.has_value()) return result;
        }
        { // second 16
            auto result = simd_detail::firstZeroIndex8x16(&this->bytes[16]);
            if(result.has_value()) return std::optional<uint32_t>(result.value() + 16);
        }
        { // third 16
            auto result = simd_detail::firstZeroIndex8x16(&this->bytes[32]);
            if(result.has_value()) return std::optional<uint32_t>(result.value() + 32);
        }
        { // last 16
            auto result = simd_detail::firstZeroIndex8x16(&this->bytes[48]);
            if(result.has_value()) return std::optional<uint32_t>(result.value() + 48);
            return std::nullopt;
        }
    }
}

template <int Width>
inline SimdMask<Width> ByteSimd<Width>::equalMask(const uint8_t value) const
{
    if constexpr (Width == 16) {
        return simd_detail::equalMask8x16(this->bytes, value);
    } 
    else if constexpr (Width == 32) {
        return simd_detail::equalMask8x32(this->bytes, value);
    }
    else {
        return simd_detail::equalMask8x64(this->bytes, value);
    }
}

#endif // SY_UTIL_SIMD_HPP_


