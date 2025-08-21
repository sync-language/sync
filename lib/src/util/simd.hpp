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

namespace simd_detail {
    std::optional<uint32_t> countTrailingZeroes32(uint32_t mask);

    std::optional<uint32_t> countTrailingZeroes64(uint64_t mask);
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

#endif // SY_UTIL_SIMD_HPP_


