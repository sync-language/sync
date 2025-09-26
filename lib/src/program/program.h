//! API
#pragma once
#ifndef SY_PROGRAM_PROGRAM_H_
#define SY_PROGRAM_PROGRAM_H_

#include "../core.h"

struct SyFunction;

typedef struct SyProgram {
    // void* _inner;
} SyProgram;

typedef enum SyProgramRuntimeErrorKind {
    SyProgramRuntimeErrorKindNone = 0,
    SyProgramRuntimeErrorKindStackOverflow = 1,

    _SY_PROGRAM_RUNTIME_ERROR_KIND_MAX_VALUE = 0x7FFFFFFF,
} SyProgramRuntimeErrorKind;

typedef struct SyProgramRuntimeError {
    SyProgramRuntimeErrorKind kind;
    // void*                       _inner;
} SyProgramRuntimeError;

typedef struct SyCallStack {
    const struct SyFunction* const* functions;
    size_t len;
} SyCallStack;

#endif // _SY_PROGRAM_PROGRAM_H_