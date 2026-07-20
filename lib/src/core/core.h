//! API
#ifndef SY_CORE_CORE_H_
#define SY_CORE_CORE_H_

#if defined(__GNUC__) || defined(__clang__)
#define SY_API __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#if SYNC_EXPORT_DLL
#define SY_API __declspec(dllexport)
#elif SYNC_IMPORT_DLL
#define SY_API __declspec(dllimport)
#else
#define SY_API
#endif
#else
#define SY_API
#endif

#ifndef __cplusplus

#include <stddef.h>
#include <stdint.h>

#if (__STDC_VERSION__ >= 202311L) // C23
// bool, true, false are part of C23
#elif (__STDC_VERSION__ >= 201112L) // C11
#include <stdbool.h>
#elif (__STDC_VERSION__ == 199901L) // C99

#ifndef bool
#define bool _Bool
#endif
#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
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

using std::ptrdiff_t;
using std::size_t;

using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::int8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

#endif // __cplusplus

#ifndef SYNC_NO_PAGES
// PUBLICLY AVAILABLE. NO NDA VIOLATION!!!!!
// https://github.com/microsoft/Xbox-GDK-Samples/blob/73f8aa373138ecd6331a05ac908a88c287602db8/Samples/Live/InGameStore/InGameStore.cpp#L293
// https://github.com/LuaJIT/LuaJIT/blob/e17ee83326f73d2bbfce5750ae8dc592a3b63c27/src/lj_arch.h#L86
// https://github.com/LuaJIT/LuaJIT/blob/e17ee83326f73d2bbfce5750ae8dc592a3b63c27/src/lj_arch.h#L171
// https://iter.ca/post/switch-oss/
#if defined(_GAMING_XBOX) || defined(__EMSCRIPTEN__) || defined(__ORBIS__) ||                      \
    defined(__PROSPERO__) || defined(__NX__) || defined(NN_NINTENDO_SDK)
#define SYNC_NO_PAGES 1
#endif
#endif // SYNC_NO_PAGES

#ifndef SYNC_BACKTRACE_SUPPORTED
#ifndef NDEBUG
#if defined(_GAMING_XBOX) || defined(__ORBIS__) || defined(__PROSPERO__) || defined(__NX__) ||     \
    defined(NN_NINTENDO_SDK)
// Don't support backtraces (at least in public code)
#else
#define SYNC_BACKTRACE_SUPPORTED 1
#endif
#endif // NDEBUG
#endif // SYNC_BACKTRACE_SUPPORTED

#ifndef SYNC_HAS_EXCEPTIONS
#ifdef __cplusplus
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
#define SYNC_HAS_EXCEPTIONS 1
#else
#define SYNC_HAS_EXCEPTIONS 0
#endif
#endif // __cplusplus
#endif // SYNC_HAS_EXCEPTIONS

#ifdef __cplusplus
extern "C" {
#endif

/// Sets the current fatal error handler for sync. Thread-safety is not guaranteed. The default
/// fatal error handler can be overridden at compile time if
/// `SYNC_CUSTOM_DEFAULT_FATAL_ERROR_HANDLER` is defined.
/// @param errHandler Non-null function pointer. Is expected to eventually terminate the program.
/// @warning If `errHandler` is `NULL` the current fatal error handler is invoked.
SY_API void sy_set_fatal_error_handler(void (*errHandler)(const char* message));

/// Sets the callback to write error messages to. Thread-safety is not guaranteed. The default
/// string error message writer can be overridden at compile time if
/// `SYNC_CUSTOM_DEFAULT_WRITE_STRING_ERROR` is defined.
/// @param writeStrErr Non-null function pointer.
/// @warning If `writeStrErr` is `NULL` the current fatal error handler is invoked.
SY_API void sy_set_write_string_error(void (*writeStrErr)(const char* message));

#ifdef __cplusplus
}
#endif

// #ifdef __cplusplus
// namespace sy {} // namespace sy
// #else
// // https://en.cppreference.com/cpp/error/exception
// typedef enum SyExceptional {
//     SY_EXCEPTIONAL_NONE = 0,
//     SY_EXCEPTIONAL_OOM = 1,
//     SY_EXCEPTIONAL_BOUNDS = 2,
//     SY_EXCEPTIONAL_SYSTEM = 3,
//     SY_EXCEPTIONAL_IO = 4,
//     SY_EXCEPTIONAL_ARITHMETIC = 5,
//     SY_EXCEPTIONAL_CAPACITY = 6,
//     SY_EXCEPTIONAL_OTHER = 7,

//     _SY_EXCEPTIONAL_MAX = 0x7FFFFFFF,
// } SyExceptional;
// #endif // __cplusplus

// typedef void (*SyDestructFn)(void* obj);
// /// @return `0` on success, `1` on any allocation failure.
// typedef int (*SyCloneFn)(const void* src, void* dst);

#ifdef __cplusplus

#include <type_traits>
#if SYNC_HAS_EXCEPTIONS
#include <exception>
#endif

namespace sy {
template <typename T> void destructForT(void* obj) { static_cast<T*>(obj)->~T(); }

template <typename T> int cloneForT(const void* src, void* dst) {
    const T* srcT = static_cast<const T*>(src);
    T* dstT = static_cast<T*>(dst);

#if SYNC_HAS_EXCEPTIONS
    try {
        *dstT = *srcT;
    } catch (const std::bad_alloc& e) {
        (void)e;
        return 1;
    }
#endif
    // At this point if it fails then so be it.
    *dstT = *srcT;
    return 0;
}

} // namespace sy

#endif // __cplusplus

#endif // SY_CORE_CORE_H_
