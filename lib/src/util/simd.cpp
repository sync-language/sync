#include "simd.hpp"

#if defined(_WIN32) || defined(WIN32)

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#endif // WIN32 def

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

#endif // SYNC_LIB_NO_TESTS