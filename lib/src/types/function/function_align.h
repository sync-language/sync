//! API
#pragma once
#ifndef SY_TYPES_FUNCTION_FUNCTION_ALIGN_H_
#define SY_TYPES_FUNCTION_FUNCTION_ALIGN_H_

#include "../../core.h"

#if defined(__x86_64__) || defined (_M_AMD64)

#ifdef SY_FUNCTION_MIN_ALIGN
#error "Do not override default minimum function alignment for X86_64
#endif

// For X86_64, function alignment must be at least 16-byte aligned for their call stacks. As a result, the minimum
// required alignment for `SyFunction` calls must be 16. It's possible that functions will have special alignment,
// for example when using specially aligned types or some implementation specific SIMD operations.
// # Sources
// http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-optimization-manual.pdf 
// https://learn.microsoft.com/en-us/cpp/build/stack-usage?view=msvc-170

#ifdef __cplusplus
constexpr uint16_t SY_FUNCTION_MIN_ALIGN = 16;
#else
#define SY_FUNCTION_MIN_ALIGN 16
#endif

#elif defined(__aarch64__) || defined(_M_ARM64)

#ifdef SY_FUNCTION_MIN_ALIGN
#error "Do not override default minimum function alignment for ARM64
#endif

// For ARM64, function alignment must be at least 16-byte aligned for their call stacks. As a result, the minimum
// required alignment for `SyFunction` calls must be 16. It's possible that functions will have special alignment,
// for example when using specially aligned types or some implementation specific SIMD operations.
// https://learn.microsoft.com/en-us/cpp/build/arm64-windows-abi-conventions?view=msvc-170

#ifdef __cplusplus
constexpr uint16_t SY_FUNCTION_MIN_ALIGN = 16;
#else
#define SY_FUNCTION_MIN_ALIGN 16
#endif

#else

#ifndef SY_FUNCTION_MIN_ALIGN
// For unknown targets, the minimum function alignment is simply set as the target's pointer alignment (same as size)
#ifdef __cplusplus
constexpr uint16_t SY_FUNCTION_MIN_ALIGN = sizeof(void*);
#else
#define SY_FUNCTION_MIN_ALIGN sizeof(void*)
#endif

#endif // SY_FUNCTION_MIN_ALIGN

#endif // defined X86_64 or ARM64

#endif // SY_TYPES_FUNCTION_FUNCTION_ALIGN_H_