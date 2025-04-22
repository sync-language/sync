//! API
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
#define bool  _Bool
#define false 0
#define true  1
#else // __STDC_VERSION__ some other
#error C99 or higher compiler required. Need _Bool.
#endif // __STDC_VERSION__

#else // __cplusplus

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

#endif // SY_CORE_H_
