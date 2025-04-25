//! API
#pragma once
#ifndef SY_TYPES_STRING_STRING_SLICE_H_
#define SY_TYPES_STRING_STRING_SLICE_H_

#include "../../core.h"

/// Is essentially a [C++ std::string_view](https://en.cppreference.com/w/cpp/header/string_view) 
/// or a [Rust &str](https://doc.rust-lang.org/std/primitive.str.html). Is
/// trivially copyable. Does not need to be null terminated, nor have any special alignment.
typedef struct SyStringSlice {
    /// Must be UTF8. Does not have to be null terminated. Is not read from if `len == 0`.
    const char* ptr;
    /// Does not include possible null terminator. Is measured in bytes.
    size_t len;
} SyStringSlice;

#endif // SY_TYPES_STRING_STRING_SLICE_H_