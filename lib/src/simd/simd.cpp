// Query implementations, and implementations that don't use the CPU instructions, doing it the
// scalar way when necessary.

#include "simd.hpp"

#if defined(_WIN32) || defined(WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>

#endif // WIN32 def

#if defined(__SSE2__) || defined(__AVX2__) || defined(__AVX512F__)
// #include <intrin.h>
#include <immintrin.h>
#elif __ARM_NEON__
#include <arm_neon.h>
#endif

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#endif

bool sy::internal::cpuHasSSE42() noexcept {
    static const bool has = []() -> bool {
#if defined(_WIN32) || defined(WIN32)
        return (IsProcessorFeaturePresent(PF_SSE3_INSTRUCTIONS_AVAILABLE) != FALSE) &&
               (IsProcessorFeaturePresent(PF_SSSE3_INSTRUCTIONS_AVAILABLE) != FALSE) &&
               (IsProcessorFeaturePresent(PF_SSE4_1_INSTRUCTIONS_AVAILABLE) != FALSE) &&
               (IsProcessorFeaturePresent(PF_SSE4_2_INSTRUCTIONS_AVAILABLE) != FALSE);
#elif defined(__GNUC__) || defined(__clang__)
        __builtin_cpu_init();
        return __builtin_cpu_supports("sse4.2") != 0;
#else
        return false;
#endif
    }();
    return has;
}

bool sy::internal::cpuHasAVX2() noexcept {
    static const bool has = []() -> bool {
        if (!cpuHasSSE42()) {
            return false; // tiering
        }
#if defined(_WIN32) || defined(WIN32)
        return IsProcessorFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE) != FALSE;
#elif defined(__GNUC__) || defined(__clang__)
        __builtin_cpu_init();
        return __builtin_cpu_supports("avx2") != 0;
#else
        return false;
#endif
    }();
    return has;
}

bool sy::internal::cpuHasAVX512BW() noexcept {
    static const bool has = []() -> bool {
        if (!cpuHasSSE42()) {
            return false; // tiering
        }
        if (!cpuHasAVX2()) {
            return false; // tiering
        }
#if defined(_WIN32) || defined(WIN32)
        // Gate on F: this validates the OS XSAVE-state check (ZMM regs) for free.
        // Then read CPUID.(EAX=7,ECX=0):EBX[30] for the BW bit.
        if (IsProcessorFeaturePresent(PF_AVX512F_INSTRUCTIONS_AVAILABLE) == FALSE)
            return false;
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
        int info[4];
        __cpuidex(info, 7, 0);
        return (info[1] & (1 << 30)) != 0;
#else
        return false;
#endif
#elif defined(__GNUC__) || defined(__clang__)
        __builtin_cpu_init();
        return __builtin_cpu_supports("avx512bw") != 0;
#else
        return false;
#endif
    }();
    return has;
}

bool sy::internal::cpuHasSVE() noexcept {
    static const bool has = []() -> bool {
#if (defined(_WIN32) || defined(WIN32)) && defined(_M_ARM64) &&                                    \
    defined(PF_ARM_SVE_INSTRUCTIONS_AVAILABLE)
        return IsProcessorFeaturePresent(PF_ARM_SVE_INSTRUCTIONS_AVAILABLE) != FALSE;
#elif defined(__ARM_FEATURE_SVE)
        return true; // compiled with SVE enabled
#else
        return false;
#endif
    }();
    return has;
}

bool sy::internal::cpuHasRVV() noexcept {
#if defined(__riscv_v) || defined(__riscv_vector)
    return true;
#else
    return false;
#endif
}

#if !defined(__wasm__)
#if !defined(__x86_64__) && !defined(_M_X64) && !defined(__aarch64__) && !defined(_M_ARM64) &&     \
    !defined(__riscv) && !(__riscv_xlen == 64)
sy::Option<uint8_t> sy::internal::firstZeroIndex8x16(const uint8_t* mem) noexcept {
    assert_aligned(mem, 16);
    for (size_t i = 0; i < 16; i++) {
        if (mem[i] == 0) {
            return i;
        }
    }
    return {};
}
#endif // not x64 or arm64 or riscv64
#endif // not wasm

#if !defined(__wasm__)
#if !defined(__x86_64__) && !defined(_M_X64) && !defined(__aarch64__) && !defined(_M_ARM64)
bool equalBytes8x16(const uint8_t* lhs, const uint8_t* rhs) noexcept {
    assert_aligned(lhs, 16);
    assert_aligned(rhs, 16);
    const uint64_t* la = reinterpret_cast<const uint64_t*>(lhs);
    const uint64_t* ra = reinterpret_cast<const uint64_t*>(rhs);
    return ((la[0] ^ ra[0]) | (la[1] ^ ra[1])) == 0;
}
#endif // not x64 or arm64
#endif // not wasm
