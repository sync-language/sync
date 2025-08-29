//! API
#pragma once
#ifndef SY_CORE_H_
#define SY_CORE_H_

#if defined(__GNUC__) || defined (__clang__)
#define SY_API __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#define SY_API __declspec(dllexport)
#else
#define SY_API
#endif

#ifndef __cplusplus

#include <stddef.h>
#include <stdint.h>

#if (__STDC_VERSION__ >= 201112L) // C11
#include <stdbool.h>
#elif (__STDC_VERSION__ == 199901L) // C99

#ifndef bool
#define bool  _Bool
#endif
#ifndef false
#define false 0
#endif
#ifndef true
#define true  1
#endif

#else // __STDC_VERSION__ some other
#error C99 or higher compiler required. Need _Bool.
#endif // __STDC_VERSION__

#else // __cplusplus

#undef bool
#undef true
#undef false

#include <cstddef>
#include <cstdint>

using std::size_t;
using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

#endif // __cplusplus

// In order to test the private (actually protected) logic within classes 
// without being forced to expose them as part of the public API, we need to 
// leverage test classes, being classes that derive the classes we actually 
// want to test. As a result, while all non-inherited classes (overwhelming 
// majority of the classes in the library) should be marked final, for 
// testing purposes, we need to derive them.
// On top of that, we use SY_CLASS_TEST_PRIVATE for anything that should be
// private to the class, but must be protected for testing purposes.

#if SYNC_LIB_WITH_TESTS
#define SY_CLASS_FINAL
#define SY_CLASS_TEST_PRIVATE protected
#else
#define SY_CLASS_FINAL final
#define SY_CLASS_TEST_PRIVATE private
#endif

#endif // SY_CORE_H_
