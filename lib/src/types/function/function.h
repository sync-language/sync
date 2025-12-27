//! API
#pragma once
#ifndef SY_TYPES_FUNCTION_FUNCTION_H_
#define SY_TYPES_FUNCTION_FUNCTION_H_

#include "../../core.h"
#include "../../program/program.h"
#include "../string/string_slice.h"
#include "function_align.h"

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
    /// Un-namespaced name. For example if `qualifiedName == "example.func"` then `name == "func"`.
    SyStringSlice name;
    /// As fully qualified name, namespaced.
    SyStringSlice qualifiedName;
    /// If `NULL`, the function does not return any value
    const struct SyType* returnType;
    /// If `NULL`, the function take no arguments, otherwise valid when `i < argsLen`.
    const struct SyType** argsTypes;
    /// If zero, the function take no arguments
    uint16_t argsLen;
    /// Alignment required for this function call. Any value under `SY_FUNCTION_MIN_ALIGN` will be rounded up to it.
    /// This is used to determine the necessary alignment of function calls for both script and C functions.
    /// It is possible that a function will have non-standard alignment, such as functions using SIMD types.
    /// # Debug Asserts
    /// Alignment must be a multiple of 2.
    /// `alignment % 2 == 0`.
    uint16_t alignment;
    /// If `true`, this function can be called in a comptime context within Sync source code.
    bool comptimeSafe;
    /// Determines if this function is a C function or script function.
    SyFunctionType tag;
    /// Both for C functions and script functions. Given `tag` and `info`, the function will be correctly called.
    /// For C functions, this should be a function with the signature of `sy_c_function_t`.
    const void* fptr;
} SyFunction;

/// Helper struct to push function arguments to C functions or into the next script stack frame.
typedef struct SyFunctionCallArgs {
    const SyFunction* func;
    uint16_t pushedCount;
    /// Internal use only.
    uint16_t _offset;
} SyFunctionCallArgs;

typedef struct SyCFunctionHandler {
    uint32_t _handle;
} SyCFunctionHandler;

/// Function singature for C functions.
typedef SyProgramRuntimeError (*sy_c_function_t)(SyCFunctionHandler handler);

#ifdef __cplusplus
extern "C" {
#endif

// /// Starts the process of calling a function. See `sy_function_push_arg(...)` and `sy_function_call(...)`.
// SY_API SyFunctionCallArgs sy_function_start_call(const SyFunction* self);

// /// Pushs an argument onto the the script or C stack for the next function call.
// /// @return `true` if the push was successful, or `false`, if the stack would overflow by pushing the argument.
// SY_API bool sy_function_call_args_push(SyFunctionCallArgs* self, void* argMem, const struct SyType* typeInfo);

// SY_API SyProgramRuntimeError sy_function_call(SyFunctionCallArgs self, void* retDst);

// SY_API void sy_c_function_handler_take_arg(SyCFunctionHandler* self, void* outValue, size_t argIndex);

// SY_API void sy_c_function_handler_set_return_value(SyCFunctionHandler* self, const void* retValue,
//                                                    const struct SyType* type);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SY_TYPES_FUNCTION_FUNCTION_H_
