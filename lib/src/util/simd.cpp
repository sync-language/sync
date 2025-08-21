#include "simd.hpp"
#include "assert.hpp"

#if defined(_WIN32) || defined(WIN32)

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#endif // WIN32 def

#if __SSE2__
#include <immintrin.h>
#elif __ARM_NEON__
#include <arm_neon.h>
#endif

#define assert_aligned(ptr, alignment) sy_assert((((uintptr_t)ptr) % alignment == 0), "Pointer not properly aligned");

std::optional<uint32_t> simd_detail::countTrailingZeroes32(uint32_t mask)
{
    #if defined(_WIN32) || defined(WIN32)
    unsigned long out;
    if(_BitScanForward(&out, mask) == 0) {
        return std::nullopt;
    }
    return std::optional<uint32_t>(static_cast<uint32_t>(out));
    #elif __GNUC__
    uint32_t out;
    if(mask == 0) {
        return std::nullopt;
    }
    out = (uint32_t)__builtin_ctz(mask);
    return std::optional<uint32_t>(out);
    #else
    for(uint32_t i = 0; i < 32; i++) {
        if(mask & (1 << i) != 0) {
           return std::optional<uint32_t>(i); 
        }
    }
    return std::nullopt;
    #endif
}

std::optional<uint32_t> simd_detail::countTrailingZeroes64(uint64_t mask)
{
    #if defined(_WIN32) || defined(WIN32)
    unsigned long out;
    if(_BitScanForward64(&out, mask) == 0) {
        return std::nullopt;
    }
    return std::optional<uint32_t>(static_cast<uint32_t>(out));
    #elif __GNUC__
    if(mask == 0) {
        return std::nullopt;
    }
    const uint32_t out = (uint32_t)__builtin_ctzll(mask);
    return std::optional<uint32_t>(out);
    #else
    for(uint32_t i = 0; i < 64; i++) {
        if(mask & (1ULL << i) != 0) {
           return std::optional<uint32_t>(i); 
        }
    }
    return std::nullopt;
    #endif
}

std::optional<uint32_t> simd_detail::firstZeroIndex8x16(const uint8_t *alignedPtr)
{
    assert_aligned(alignedPtr, 16);

    #if __SSE2__
    const __m128i zeroVec = _mm_set1_epi8(0);
    const __m128i buf = *(const __m128i*)alignedPtr;
    const __m128i result = _mm_cmpeq_epi8(zeroVec, buf);
    int resultMask = _mm_movemask_epi8(result);
    return countTrailingZeroes32(static_cast<uint32_t>(resultMask));
    #elif __ARM_NEON__
    const uint8x16_t zeroVec = {0};
    const uint8x16_t buf = *(const uint8x16_t*)(alignedPtr);
    const uint16x8_t equalMask = vreinterpretq_u16_u8(vceqq_u8(zeroVec, buf));
    // https://community.arm.com/arm-community-blogs/b/servers-and-cloud-computing-blog/posts/porting-x86-vector-bitmask-optimizations-to-arm-neon
    const uint8x8_t res = vshrn_n_u16(equalMask, 4);
    const uint64_t matches = vget_lane_u64(vreinterpret_u64_u8(res), 0);
    return countTrailingZeroes32(static_cast<uint32_t>(matches));
    #else
    for(uint32_t i = 0; i < 16; i++) {
        if(alignedPtr[i] == 0) {
            return std::optional<uint32_t>(i);
        }
    }
    return std::nullopt;
    #endif
}

#ifndef SYNC_LIB_NO_TESTS

#include "../doctest.h"
#include <iostream>

TEST_SUITE("simd mask") {
    TEST_CASE("empty") {
        { // 16
            SimdMask<16> self(0);
            for(uint32_t index : self) {
                (void)index;
                CHECK(false);
            }
        }
        { // 32
            SimdMask<32> self(0);
            for(uint32_t index : self) {
                (void)index;
                CHECK(false);
            }
        }
        { // 64
            SimdMask<64> self(0);
            for(uint32_t index : self) {
                (void)index;
                CHECK(false);
            }
        }
    }

    TEST_CASE("all bits set") {
        { // 16
            SimdMask<16> self(0xFFFF);
            uint32_t i = 0;
            for(uint32_t index : self) {
                CHECK_EQ(index, i);
                i++;
            }
            CHECK_EQ(i, 16);
        }
        { // 32
            SimdMask<32> self(0xFFFFFFFFU);
            uint32_t i = 0;
            for(uint32_t index : self) {
                CHECK_EQ(index, i);
                i++;
            }
            CHECK_EQ(i, 32);
        }
        { // 64
            SimdMask<64> self(0xFFFFFFFFFFFFFFFFULL);
            uint32_t i = 0;
            for(uint32_t index : self) {
                CHECK_EQ(index, i);
                i++;
            }
            CHECK_EQ(i, 64);
        }
    }

    TEST_CASE("mixed bits set") {
        { // 16
            SimdMask<16> self(0b0101010101010101);
            uint32_t i = 0;
            for(uint32_t index : self) {
                CHECK_EQ(index, i);
                i += 2;
            }
            CHECK_EQ(i, 16);
        }
        { // 32
            SimdMask<32> self(0b01010101010101010101010101010101U);
            uint32_t i = 0;
            for(uint32_t index : self) {
                CHECK_EQ(index, i);
                i += 2;
            }
            CHECK_EQ(i, 32);
        }
        { // 64
            SimdMask<64> self(0b0101010101010101010101010101010101010101010101010101010101010101ULL);
            uint32_t i = 0;
            for(uint32_t index : self) {
                CHECK_EQ(index, i);
                i += 2;
            }
            CHECK_EQ(i, 64);
        }
    }
}

TEST_SUITE("find first zero") {
    TEST_CASE("all zeroes") {
        { // 16
            ByteSimd<16> self;
            auto result = self.firstZeroIndex();
            CHECK(result.has_value());
            CHECK_EQ(result.value(), 0);
        }
        { // 32
            ByteSimd<32> self;
            auto result = self.firstZeroIndex();
            CHECK(result.has_value());
            CHECK_EQ(result.value(), 0);
        }
        { // 64
            ByteSimd<64> self;
            auto result = self.firstZeroIndex();
            CHECK(result.has_value());
            CHECK_EQ(result.value(), 0);
        }
    }
    
    TEST_CASE("all non zero") {
        { // 16
            ByteSimd<16> self(1);
            auto result = self.firstZeroIndex();
            CHECK_FALSE(result.has_value());
        }
        { // 32
            ByteSimd<32> self(1);
            auto result = self.firstZeroIndex();
            CHECK_FALSE(result.has_value());
        }
        { // 64
            ByteSimd<64> self(1);
            auto result = self.firstZeroIndex();
            CHECK_FALSE(result.has_value());
        }
    }

    TEST_CASE("all non zero except first") {
        { // 16
            ByteSimd<16> self(1);
            self.bytes[0] = 0;
            auto result = self.firstZeroIndex();
            CHECK(result.has_value());
            CHECK_EQ(result.value(), 0);
        }
        { // 32
            ByteSimd<32> self(1);
            self.bytes[0] = 0;
            auto result = self.firstZeroIndex();
            CHECK(result.has_value());
            CHECK_EQ(result.value(), 0);
        }
        { // 64
            ByteSimd<64> self(1);
            self.bytes[0] = 0;
            auto result = self.firstZeroIndex();
            CHECK(result.has_value());
            CHECK_EQ(result.value(), 0);
        }
    }

    TEST_CASE("all non zero except last") {
        { // 16
            ByteSimd<16> self(1);
            self.bytes[15] = 0;
            auto result = self.firstZeroIndex();
            CHECK(result.has_value());
            CHECK_EQ(result.value(), 15);
        }
        { // 32
            ByteSimd<32> self(1);
            self.bytes[31] = 0;
            auto result = self.firstZeroIndex();
            CHECK(result.has_value());
            CHECK_EQ(result.value(), 31);
        }
        { // 64
            ByteSimd<64> self(1);
            self.bytes[63] = 0;
            auto result = self.firstZeroIndex();
            CHECK(result.has_value());
            CHECK_EQ(result.value(), 63);
        }
    }
}

#endif // SYNC_LIB_NO_TESTS