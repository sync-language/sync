//! API
#pragma once
#ifndef SY_TYPES_FUNCTION_FUNCTION_H_
#define SY_TYPES_FUNCTION_FUNCTION_H_

#include "../../core.h"
#include "../string/string_slice.h"

struct SyType;

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
    /// Alignment required for this function call. Any value under
    /// [16](https://learn.microsoft.com/en-us/cpp/build/stack-usage?view=msvc-170) will be rounded up to 16.
    /// This is used to determine the necessary alignment of function calls
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
    uint16_t            _inner[2];
} SyFunctionCallArgs;

/// Holds the destination for the return value and type of C and script functions.
/// Should be zero initialized for functions that return nothing.
typedef struct SyFunctionReturn {
    void*                   destValue;
    const struct SyType**   destType;
} SyFunctionReturn;

#ifdef __cplusplus
extern "C" {
#endif

/// Starts the process of calling a function. See `sy_function_push_arg(...)` and `sy_function_call(...)`.
SY_API SyFunctionCallArgs sy_function_start_call(const SyFunction* self);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SY_TYPES_FUNCTION_FUNCTION_H_
