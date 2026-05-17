// SIMD implementations ONLY for NEON, which is required for AArch64.

#include "simd.hpp"

#if defined(__aarch64__) || defined(_M_ARM64)

#include "simd.hpp"
#include <arm_neon.h>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

bool sy::internal::equalBytes8x16(const uint8_t* lhs, const uint8_t* rhs) noexcept {
    assert_aligned(lhs, 16);
    assert_aligned(rhs, 16);
    const uint8x16_t a = vld1q_u8(lhs);
    const uint8x16_t b = vld1q_u8(rhs);
    const uint8x16_t eq = vceqq_u8(a, b);
    return vminvq_u8(eq) == 0xFF;
}

sy::Option<uint8_t> sy::internal::firstZeroIndex8x16(const uint8_t* mem) noexcept {
    assert_aligned(mem, 16);
    const uint8x16_t v = vld1q_u8(mem);
    const uint8x16_t eq = vceqzq_u8(v);
    // stupid shrn trick, basically compress 16 lanes of 0xFF/0x00 into a 64-bit value with 4 mask
    // bits per input byte.
    // Reinterpret as 8 lanes of u16, narrow-shift right by 4, reinterpret the resulting
    // uint8x8_t as a single u64. The index of the first matching byte is countr_zero(mask) / 4.
    const uint64_t mask =
        vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(eq), 4)), 0);
    if (mask == 0) {
        return std::nullopt;
    }
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward64(&idx, mask);
    return static_cast<uint8_t>(idx >> 2);
#else
    return static_cast<uint8_t>(__builtin_ctzll(mask) >> 2);
#endif
}

#endif
