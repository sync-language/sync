//! API
#pragma once
#ifndef SY_TYPES_FUNCTION_FUNCTION_H_
#define SY_TYPES_FUNCTION_FUNCTION_H_

#include "../../core.h"
#include "../string/string_slice.h"
#include "../../program/program.h"

#if defined(__x86_64__) || defined (_M_AMD64)

#ifdef SY_FUNCTION_MIN_ALIGN
#error "Do not override default minimum function alignment for X86_64
#endif

/// For X86_64, function alignment must be at least 16-byte aligned for their call stacks. As a result, the minimum
/// required alignment for `SyFunction` calls must be 16. It's possible that functions will have special alignment,
/// for example when using specially aligned types or some implementation specific SIMD operations.
/// # Sources
/// http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-optimization-manual.pdf 
/// https://learn.microsoft.com/en-us/cpp/build/stack-usage?view=msvc-170
#define SY_FUNCTION_MIN_ALIGN 16

#elif defined(__aarch64__) || defined(_M_ARM64)

#ifdef SY_FUNCTION_MIN_ALIGN
#error "Do not override default minimum function alignment for ARM64
#endif

/// For ARM64, function alignment must be at least 16-byte aligned for their call stacks. As a result, the minimum
/// required alignment for `SyFunction` calls must be 16. It's possible that functions will have special alignment,
/// for example when using specially aligned types or some implementation specific SIMD operations.
/// https://learn.microsoft.com/en-us/cpp/build/arm64-windows-abi-conventions?view=msvc-170
#define SY_FUNCTION_MIN_ALIGN 16

#else

#ifndef SY_FUNCTION_MIN_ALIGN
/// For unknown targets, the minimum function alignment is simply set as the target's pointer alignment (same as size)
#define SY_FUNCTION_MIN_ALIGN sizeof(void*)
#endif // SY_FUNCTION_MIN_ALIGN

#endif // defined X86_64 or ARM64

struct SyType;
struct SyProgramRuntimeError;

typedef enum SyFunctionType {
    SyFunctionTypeC = 0,
    SyFunctionTypeScript = 1,

    _SY_FUNCTION_TYPE_USED_BITS = 1,
    // Ensure 32 bit size
    _SY_FUNCTION_TYPE_MAX_VALUE = 0x7FFFFFFF,
} SyFunctionType;

/// Script type as const reference.
typedef struct SyFunction {
    /// As fully qualified name, namespaced.
    SyStringSlice           name;
    /// Un-namespaced name. For example if `name == "example.func"` then `identifierName == "func"`.
    SyStringSlice           identifierName;
    /// If `NULL`, the function does not return any value
    const struct SyType*    returnType;
    /// If `NULL`, the function take no arguments, otherwise valid when `i < argsLen`.
    const struct SyType**   argsTypes;
    /// If zero, the function take no arguments
    uint16_t                argsLen;
    /// Alignment required for this function call. Any value under `SY_FUNCTION_MIN_ALIGN` will be rounded up to it.
    /// This is used to determine the necessary alignment of function calls for both script and C functions.
    /// It is possible that a function will have non-standard alignment, such as functions using SIMD types.
    /// # Debug Asserts
    /// Alignment must be a multiple of 2.
    /// `alignment % 2 == 0`.
    uint16_t                  alignment;
    /// Determines if this function is a C function or script function.
    SyFunctionType          tag;
    /// Both for C functions and script functions. Given `tag` and `info`, the function will be correctly called.
    const void*             fptr;
} SyFunction;

/// Helper struct to push function arguments to C functions or into the next script stack frame.
typedef struct SyFunctionCallArgs {
    const SyFunction*   func;
    uint16_t            pushedCount;
    /// Internal use only.
    uint16_t            _offset;
} SyFunctionCallArgs;

typedef struct SyCFunctionHandler {
    uint32_t _handle;
} SyCFunctionHandler;

#ifdef __cplusplus
extern "C" {
#endif

/// Starts the process of calling a function. See `sy_function_push_arg(...)` and `sy_function_call(...)`.
SY_API SyFunctionCallArgs sy_function_start_call(const SyFunction* self);

/// Pushs an argument onto the the script or C stack for the next function call.
/// @return `true` if the push was successful, or `false`, if the stack would overflow by pushing the argument.
SY_API bool sy_function_call_args_push(SyFunctionCallArgs* self, const void* argMem, const struct SyType* typeInfo);

SY_API SyProgramRuntimeError sy_function_call(SyFunctionCallArgs self, void* retDst);

SY_API void sy_c_function_handler_take_arg(SyCFunctionHandler* self, void* outValue, size_t argIndex);

SY_API void sy_c_function_handler_set_return_value(
    SyCFunctionHandler* self, const void* retValue, const struct SyType* type);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SY_TYPES_FUNCTION_FUNCTION_H_
