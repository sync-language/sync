// SIMD implementations ONLY for wasm.

#if defined(__wasm__)

#include "simd.hpp"

#if defined(__wasm_simd128__)
#include <wasm_simd128.h>
#endif

bool sy::internal::equalBytes8x16(const uint8_t* lhs, const uint8_t* rhs) noexcept {
    assert_aligned(lhs, 16);
    assert_aligned(rhs, 16);

    if constexpr (cpuHasWasmSIMD()) {
#if defined(__wasm_simd128__)
        const v128_t a = wasm_v128_load(lhs);
        const v128_t b = wasm_v128_load(rhs);
        return wasm_i8x16_all_true(wasm_i8x16_eq(a, b));
#endif
    } else {
        const uint64_t* la = reinterpret_cast<const uint64_t*>(lhs);
        const uint64_t* ra = reinterpret_cast<const uint64_t*>(rhs);
        return ((la[0] ^ ra[0]) | (la[1] ^ ra[1])) == 0;
    }
}

sy::Option<uint8_t> sy::internal::firstZeroIndex8x16(const uint8_t* mem) noexcept {
    assert_aligned(mem, 16);

    if constexpr (cpuHasWasmSIMD()) {
#if defined(__wasm_simd128__)
        const v128_t v = wasm_v128_load(mem);
        const v128_t eq = wasm_i8x16_eq(v, wasm_i8x16_const_splat(0));
        const uint32_t mask = static_cast<uint32_t>(wasm_i8x16_bitmask(eq));
        if (mask == 0) {
            return std::nullopt;
        }
        return static_cast<uint8_t>(__builtin_ctz(mask));
#endif
    } else {
        for (uint8_t i = 0; i < 16; ++i) {
            if (mem[i] == 0) {
                return i;
            }
        }
        return std::nullopt;
    }
}
#endif
