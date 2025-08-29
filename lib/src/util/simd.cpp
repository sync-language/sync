#include "simd.hpp"
#include "assert.hpp"

#if defined(_WIN32) || defined(WIN32)

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#endif // WIN32 def

#if defined(__SSE2__) || defined(__AVX2__)
// #include <intrin.h>
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

#include <iostream>

std::optional<uint32_t> simd_detail::firstZeroIndex8x16(const uint8_t *alignedPtr)
{
    assert_aligned(alignedPtr, 16);

    #if defined(__SSE2__) || defined(__AVX2__) // stupid
    const __m128i zeroVec = _mm_set1_epi8(0);
    const __m128i buf = *(const __m128i*)alignedPtr;
    const __m128i result = _mm_cmpeq_epi8(zeroVec, buf);
    int resultMask = _mm_movemask_epi8(result);
    return countTrailingZeroes32(static_cast<uint32_t>(resultMask));
    // TODO fix neon
    // #elif __ARM_NEON__
    // const uint8x16_t zeroVec = {0};
    // const uint8x16_t buf = *(const uint8x16_t*)(alignedPtr);
    // const uint16x8_t equalMask = vreinterpretq_u16_u8(vceqq_u8(zeroVec, buf));
    // // https://community.arm.com/arm-community-blogs/b/servers-and-cloud-computing-blog/posts/porting-x86-vector-bitmask-optimizations-to-arm-neon
    // const uint8x8_t res = vshrn_n_u16(equalMask, 4);
    // const uint64_t matches = vget_lane_u64(vreinterpret_u64_u8(res), 0);
    // return countTrailingZeroes32(static_cast<uint32_t>(matches));
    #else
    for(uint32_t i = 0; i < 16; i++) {
        if(alignedPtr[i] == 0) {
            return std::optional<uint32_t>(i);
        }
    }
    return std::nullopt;
    #endif
}

SimdMask<16> simd_detail::equalMask8x16(const uint8_t *alignedPtr, uint8_t value)
{
    assert_aligned(alignedPtr, 16);

    #if defined(__SSE2__) || defined(__AVX2__) // stupid
    const __m128i valueToFind = _mm_set1_epi8(value);
    const __m128i bufferToSearch = *reinterpret_cast<const __m128i*>(alignedPtr);
    const __m128i result = _mm_cmpeq_epi8(valueToFind, bufferToSearch);
    const int mask = _mm_movemask_epi8(result);
    return SimdMask<16>(static_cast<uint16_t>(mask));
    #else // TODO ARM NEON
    uint16_t out = 0;
    for(size_t i = 0; i < 16; i++) {
        if(alignedPtr[i] == value) {
            out |= (1U << i);
        }
    }
    return SimdMask<16>(out);
    #endif
}

SimdMask<32> simd_detail::equalMask8x32(const uint8_t *alignedPtr, uint8_t value)
{
    assert_aligned(alignedPtr, 32);

    #if __AVX2__
    const __m256i valueToFind = _mm256_set1_epi8(value);
    const __m256i bufferToSearch = *reinterpret_cast<const __m256i*>(alignedPtr);
    const __m256i result = _mm256_cmpeq_epi8(valueToFind, bufferToSearch);
    const int mask = _mm256_movemask_epi8(result);
    return SimdMask<32>(static_cast<uint32_t>(mask));
    #else // TODO ARM NEON
    uint32_t out = 0;
    for(size_t i = 0; i < 32; i++) {
        if(alignedPtr[i] == value) {
            out |= (1U << i);
        }
    }
    return SimdMask<32>(out);
    #endif
}

#if defined(__AVX2__)
static bool is_avx512f_supported() {
#if defined(_WIN32) || defined(WIN32)
	return IsProcessorFeaturePresent(PF_AVX512F_INSTRUCTIONS_AVAILABLE);
#else
    return false; // TODO linux
#endif
}

static SimdMask<64> equalMask8x64AVX512(const uint8_t* p, uint8_t v) {
    const __m512i valueToFind = _mm512_set1_epi8(v);
    const __m512i bufferToSearch = *reinterpret_cast<const __m512i*>(p);
    const unsigned long long mask = _mm512_cmpeq_epi8_mask(valueToFind, bufferToSearch);
    return SimdMask<64>(static_cast<uint64_t>(mask));
};

static SimdMask<64> equalMask8x64AVX2(const uint8_t* p, uint8_t v) {
    auto low = simd_detail::equalMask8x32(&p[0], v);
    auto high = simd_detail::equalMask8x32(&p[32], v);
    return SimdMask<64>(static_cast<uint64_t>(low.mask) 
        | (static_cast<uint64_t>(high.mask) << 32));
};

static bool equalBytes8x64AVX512(const uint8_t* lhs, const uint8_t* rhs) {
    const __m512i lhsVec = *reinterpret_cast<const __m512i*>(lhs);
    const __m512i rhsVec = *reinterpret_cast<const __m512i*>(rhs);
    const unsigned long long mask = _mm512_cmpeq_epi8_mask(lhsVec, rhsVec);
    constexpr unsigned long long matched = ~0ULL;
    return mask == matched;
}

static bool equalBytes8x64AVX2(const uint8_t* lhs, const uint8_t* rhs) {
    if(!simd_detail::equalBytes8x32(&lhs[0], &rhs[0])) {
        return false;
    }
    return simd_detail::equalBytes8x32(&lhs[32], &rhs[32]);
}

#endif

SimdMask<64> simd_detail::equalMask8x64(const uint8_t *alignedPtr, uint8_t value)
{
    assert_aligned(alignedPtr, 64);

    #if defined(__AVX2__) // not every x86_64 cpu supports AVX512F so we use this questionable workaround
    using FuncT = SimdMask<64> (*)(const uint8_t*, uint8_t);
    static const FuncT func = []() {
        if(is_avx512f_supported()) {
            return equalMask8x64AVX512;
        } else {
            return equalMask8x64AVX2;
        }
    }();

    return func(alignedPtr, value);
    #else // TODO ARM NEON
    uint64_t out = 0;
    for(size_t i = 0; i < 64; i++) {
        if(alignedPtr[i] == value) {
            out |= (1ULL << i);
        }
    }
    return SimdMask<64>(out);
    #endif
}

bool simd_detail::equalBytes8x16(const uint8_t* lhs, const uint8_t* rhs)
{
    assert_aligned(lhs, 16);
    assert_aligned(rhs, 16);

    #if defined(__SSE2__) || defined(__AVX2__) // stupid
    const __m128i lhsVec = *reinterpret_cast<const __m128i*>(lhs);
    const __m128i rhsVec = *reinterpret_cast<const __m128i*>(rhs);
    const __m128i result = _mm_cmpeq_epi8(lhsVec, rhsVec);
    const int mask = _mm_movemask_epi8(result);
    return mask == 0xFFFF; // 16 bits set
    #else // TODO ARM NEON
    for(size_t i = 0; i < 16; i++) {
        if(lhs[i] != rhs[i]) return false;
    }
    return true;
    #endif
}

bool simd_detail::equalBytes8x32(const uint8_t* lhs, const uint8_t* rhs)
{
    assert_aligned(lhs, 32);
    assert_aligned(rhs, 32);

    #if defined(__AVX2__)
    const __m256i lhsVec = *reinterpret_cast<const __m256i*>(lhs);
    const __m256i rhsVec = *reinterpret_cast<const __m256i*>(rhs);
    const __m256i result = _mm256_cmpeq_epi8(lhsVec, rhsVec);
    const int mask = _mm256_movemask_epi8(result);
    constexpr int matched = ~0;
    return mask == matched; // 32 bits set
    #else // TODO ARM NEON
    for(size_t i = 0; i < 32; i++) {
        if(lhs[i] != rhs[i]) return false;
    }
    return true;
    #endif
}

bool simd_detail::equalBytes8x64(const uint8_t* lhs, const uint8_t* rhs)
{
    assert_aligned(lhs, 64);
    assert_aligned(rhs, 64);

    #if defined(__AVX2__)
    using FuncT = bool (*)(const uint8_t*, const uint8_t*);
    static const FuncT func = []() {
        if(is_avx512f_supported()) {
            return equalBytes8x64AVX512;
        } else {
            return equalBytes8x64AVX2;
        }
    }();

    return func(lhs, rhs);
    #else // TODO ARM NEON
    for(size_t i = 0; i < 64; i++) {
        if(lhs[i] != rhs[i]) return false;
    }
    return true;
    #endif
}

#if SYNC_LIB_WITH_TESTS

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

TEST_SUITE("equal mask") {
    TEST_CASE("all not equal") {
        { // 16
            ByteSimd<16> self;
            SimdMask<16> mask = self.equalMask(1);
            for(uint32_t index : mask) {
                (void)index;
                CHECK(false);
            }
        }
        { // 32
            ByteSimd<32> self;
            SimdMask<32> mask = self.equalMask(1);
            for(uint32_t index : mask) {
                (void)index;
                CHECK(false);
            }
        }
        { // 64
            ByteSimd<64> self;
            SimdMask<64> mask = self.equalMask(1);
            for(uint32_t index : mask) {
                (void)index;
                CHECK(false);
            }
        }
    }
    
    TEST_CASE("all equal zero") {
        { // 16
            ByteSimd<16> self;
            SimdMask<16> mask = self.equalMask(0);
            uint32_t i = 0;
            for(uint32_t index : mask) {
                CHECK_EQ(index, i);
                i++;
            }
            CHECK_EQ(i, 16);
        }
        { // 32
            ByteSimd<32> self;
            SimdMask<32> mask = self.equalMask(0);
            uint32_t i = 0;
            for(uint32_t index : mask) {
                CHECK_EQ(index, i);
                i++;
            }
            CHECK_EQ(i, 32);
        }
        { // 64
            ByteSimd<64> self;
            SimdMask<64> mask = self.equalMask(0);
            uint32_t i = 0;
            for(uint32_t index : mask) {
                CHECK_EQ(index, i);
                i++;
            }
            CHECK_EQ(i, 64);
        }
    }

    TEST_CASE("all equal non-zero") {
        { // 16
            ByteSimd<16> self(5);
            SimdMask<16> mask = self.equalMask(5);
            uint32_t i = 0;
            for(uint32_t index : mask) {
                CHECK_EQ(index, i);
                i++;
            }
            CHECK_EQ(i, 16);
        }
        { // 32
            ByteSimd<32> self(5);
            SimdMask<32> mask = self.equalMask(5);
            uint32_t i = 0;
            for(uint32_t index : mask) {
                CHECK_EQ(index, i);
                i++;
            }
            CHECK_EQ(i, 32);
        }
        { // 64
            ByteSimd<64> self(5);
            SimdMask<64> mask = self.equalMask(5);
            uint32_t i = 0;
            for(uint32_t index : mask) {
                CHECK_EQ(index, i);
                i++;
            }
            CHECK_EQ(i, 64);
        }
    }

    TEST_CASE("some set") {
        { // 16
            uint8_t arr[16] = {0};
            arr[5] = 2;
            arr[10] = 2;
            ByteSimd<16> self(arr);
            SimdMask<16> mask = self.equalMask(2);
            uint32_t i = 0;
            for(uint32_t index : mask) {
                if(index == 5) {
                    CHECK_EQ(i, 0);
                }
                else if(index == 10) {
                    CHECK_EQ(i, 1);
                }
                else {
                    FAIL("Invalid index found");
                }
                i++;
            }
            CHECK_EQ(i, 2);
        }
        { // 32
            uint8_t arr[32] = {0};
            arr[16] = 2;
            arr[24] = 2;
            ByteSimd<32> self(arr);
            SimdMask<32> mask = self.equalMask(2);
            uint32_t i = 0;
            for(uint32_t index : mask) {
                if(index == 16) {
                    CHECK_EQ(i, 0);
                }
                else if(index == 24) {
                    CHECK_EQ(i, 1);
                }
                else {
                    FAIL("Invalid index found");
                }
                i++;
            }
            CHECK_EQ(i, 2);
        }
        { // 64
            uint8_t arr[64] = {0};
            arr[45] = 2;
            arr[54] = 2;
            ByteSimd<64> self(arr);
            SimdMask<64> mask = self.equalMask(2);
            uint32_t i = 0;
            for(uint32_t index : mask) {
                if(index == 45) {
                    CHECK_EQ(i, 0);
                }
                else if(index == 54) {
                    CHECK_EQ(i, 1);
                }
                else {
                    FAIL("Invalid index found");
                }
                i++;
            }
            CHECK_EQ(i, 2);
        }
    }
}

TEST_SUITE("byte simd equal") {
    TEST_CASE("both all zero") {
        { // 16
            ByteSimd<16> lhs;
            ByteSimd<16> rhs;
            CHECK_EQ(lhs, rhs);
        }
        { // 32
            ByteSimd<32> lhs;
            ByteSimd<32> rhs;
            CHECK_EQ(lhs, rhs);
        }
        { // 64
            ByteSimd<64> lhs;
            ByteSimd<64> rhs;
            CHECK_EQ(lhs, rhs);
        }
    }

    TEST_CASE("one all zero, one all one") {
        { // 16
            ByteSimd<16> lhs;
            ByteSimd<16> rhs(1);
            CHECK_NE(lhs, rhs);
        }
        { // 32
            ByteSimd<32> lhs;
            ByteSimd<32> rhs(1);
            CHECK_NE(lhs, rhs);
        }
        { // 64
            ByteSimd<64> lhs;
            ByteSimd<64> rhs(1);
            CHECK_NE(lhs, rhs);
        }
    }

    TEST_CASE("both all one") {
        { // 16
            ByteSimd<16> lhs(1);
            ByteSimd<16> rhs(1);
            CHECK_EQ(lhs, rhs);
        }
        { // 32
            ByteSimd<32> lhs(1);
            ByteSimd<32> rhs(1);
            CHECK_EQ(lhs, rhs);
        }
        { // 64
            ByteSimd<64> lhs(1);
            ByteSimd<64> rhs(1);
            CHECK_EQ(lhs, rhs);
        }
    }
    
    TEST_CASE("one all zero, one all zeroes except start") {
        { // 16
            ByteSimd<16> lhs;
            ByteSimd<16> rhs;
            rhs.bytes[0] = 1;
            CHECK_NE(lhs, rhs);
        }
        { // 32
            ByteSimd<32> lhs;
            ByteSimd<32> rhs;
            rhs.bytes[0] = 1;
            CHECK_NE(lhs, rhs);
        }
        { // 64
            ByteSimd<64> lhs;
            ByteSimd<64> rhs;
            rhs.bytes[0] = 1;
            CHECK_NE(lhs, rhs);
        }
    }
    
    TEST_CASE("one all zero, one all zeroes except end") {
        { // 16
            ByteSimd<16> lhs;
            ByteSimd<16> rhs;
            rhs.bytes[15] = 1;
            CHECK_NE(lhs, rhs);
        }
        { // 32
            ByteSimd<32> lhs;
            ByteSimd<32> rhs;
            rhs.bytes[31] = 1;
            CHECK_NE(lhs, rhs);
        }
        { // 64
            ByteSimd<64> lhs;
            ByteSimd<64> rhs;
            rhs.bytes[63] = 1;
            CHECK_NE(lhs, rhs);
        }
    }
}

#endif // SYNC_LIB_NO_TESTS