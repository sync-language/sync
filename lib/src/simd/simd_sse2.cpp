// SIMD implementations ONLY for SSE2, which is required for x86_64.

#if defined(__x86_64__) || defined(_M_X64)

#include "simd.hpp"
#include <emmintrin.h>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

bool sy::internal::equalBytes8x16(const uint8_t* lhs, const uint8_t* rhs) noexcept {
    assert_aligned(lhs, 16);
    assert_aligned(rhs, 16);
    const __m128i a = _mm_load_si128(reinterpret_cast<const __m128i*>(lhs));
    const __m128i b = _mm_load_si128(reinterpret_cast<const __m128i*>(rhs));
    const __m128i eq = _mm_cmpeq_epi8(a, b);
    return _mm_movemask_epi8(eq) == 0xFFFF;
}

sy::internal::SimdMask16 sy::internal::matchBytesMask(const uint8_t* mem, uint8_t value) noexcept {
    assert_aligned(mem, 16);
    const __m128i v = _mm_load_si128(reinterpret_cast<const __m128i*>(mem));
    const __m128i needle = _mm_set1_epi8(static_cast<char>(value));
    const __m128i eq = _mm_cmpeq_epi8(v, needle);
    const uint16_t mask = static_cast<uint16_t>(_mm_movemask_epi8(eq));
    return SimdMask16{mask};
}

sy::Option<uint8_t> sy::internal::firstZeroIndex8x16(const uint8_t* mem) noexcept {
    assert_aligned(mem, 16);
    const __m128i v = _mm_load_si128(reinterpret_cast<const __m128i*>(mem));
    const __m128i eq = _mm_cmpeq_epi8(v, _mm_setzero_si128());
    const int mask = _mm_movemask_epi8(eq);
    if (mask == 0) {
        return {};
    }
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward(&idx, static_cast<unsigned long>(mask));
    return static_cast<uint8_t>(idx);
#else
    return static_cast<uint8_t>(__builtin_ctz(static_cast<unsigned int>(mask)));
#endif
}

#endif
