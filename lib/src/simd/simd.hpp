#pragma once
#ifndef SY_SIMD_SIMD_HPP_
#define SY_SIMD_SIMD_HPP_

#include "../core/core_internal.h"
#include "../types/option/option.hpp"

namespace sy {
namespace internal {
#if defined(__x86_64__) || defined(_M_X64)
/// Detects x86-64-v2: SSE3 / SSSE3 / SSE4.1 / SSE4.2 / POPCNT.
bool cpuHasSSE42() noexcept;

/// Detects x86-64-v3: AVX / AVX2 / BMI1 / BMI2 / FMA / LZCNT / MOVBE / F16.
/// Will also return `false` if `cpuHasSSE42() == false`.
bool cpuHasAVX2() noexcept;

/// Detects // x86-64-v4: AVX-512F + BW + CD + DQ + VL.
/// Will also return `false` if `cpuHasSSE42() == false`.
/// Will also return `false` if `cpuHasAVX2() == false`.
/// TODO VBMI VBMI2?
bool cpuHasAVX512BW() noexcept;
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
/// NEON is mandatory on AArch64, but Scalable Vector Extension is not always supported.
bool cpuHasSVE() noexcept;
#endif

#if defined(__riscv) && (__riscv_xlen == 64)
/// RISC-V vector extension (RVV 1.0).
bool cpuHasRVV() noexcept;
#endif

#if defined(__wasm__)
// NOTE: wasm simd is compile time detected with `__wasm_simd128__`.
constexpr bool cpuHasWasmSIMD() {
#if defined(SYNC_NO_WASM_SIMD)
    return false;
#elif defined(__wasm_simd128__)
    return true;
#else
    return false;
#endif
}
#endif

#define assert_aligned(ptr, alignment)                                                             \
    sy_assert((((uintptr_t)(ptr)) % (alignment) == 0), "Pointer not properly aligned");

/// Finds the index of the first zero byte
/// @param mem 16 byte aligned memory buffer to check.
/// @return The index, or none.
Option<uint8_t> firstZeroIndex8x16(const uint8_t* mem) noexcept;

/// Compare 16 bytes starting from `lhs` and `rhs`, checking if they are equal. Expects them both to
/// be 16 byte aligned.
///
/// For x86_64, unconditionally implemented in simd_sse2.cpp.
/// For AArch64, unconditionally implemented in simd_neon.cpp.
/// For WASM, compile time branching for SIMD in simd_wasm.cpp.
/// @param lhs 16 byte aligned comparison memory buffer.
/// @param rhs 16 byte aligned comparison memory buffer.
/// @return `true` is they are equal, otherwise `false`.
bool equalBytes8x16(const uint8_t* lhs, const uint8_t* rhs) noexcept;

struct SimdMask16 {
    uint16_t mask;

    /// Number of bytes that matched.
    uint32_t count() const noexcept;

    /// Index of the first matching byte, or none if no bytes matched.
    Option<uint8_t> first() const noexcept;

    struct Iterator {
        bool operator!=(const Iterator& other) const noexcept;
        uint32_t operator*() const noexcept;
        Iterator& operator++() noexcept;

      private:
        friend struct SimdMask16;
        Iterator(uint16_t inData) noexcept : data(inData) {}
        uint16_t data;
    };

    Iterator begin() const noexcept { return Iterator(mask); }

    Iterator end() const noexcept { return Iterator(0); }
};

/// Get a mask for every entry in the 16 bytes of `mem` that are equal to `value`.
/// @param mem 16 byte aligned comparison memory buffer.
/// @param value The value to check which bytes match.
/// @return A mask that can be iterated over.
SimdMask16 matchBytesMask(const uint8_t* mem, uint8_t value) noexcept;

} // namespace internal
} // namespace sy

#endif // SY_SIMD_SIMD_HPP_
