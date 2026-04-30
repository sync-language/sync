//! API
#pragma once
#ifndef SY_TYPES_STRING_STRING_H_
#define SY_TYPES_STRING_STRING_H_

#include "../../core/core.h"
#include "../../mem/allocator.h"
#include "string_slice.h"

typedef struct SyString {
    /// PRIVATE: Internal only, not ABI stable.
    void* impl_;
} SyString;

typedef struct SyStringBuilder {
    /// PRIVATE: Internal only, not ABI stable.
    void* impl_;
    /// PRIVATE: Internal only, not ABI stable.
    size_t fullAllocatedCapacity_;
} SyStringBuilder;

#ifdef __cplusplus
extern "C" {
#endif

SY_API SyAllocErr sy_string_init(SyStringSlice str, SyAllocator alloc, SyString* out);

SY_API size_t sy_string_len(const SyString* self);

SY_API SyStringSlice sy_string_as_slice(const SyString* self);

SY_API const char* sy_string_cstr(const SyString* self);

SY_API size_t sy_string_hash(const SyString* self);

SY_API bool sy_string_eq(const SyString* lhs, const SyString* rhs);

SY_API bool sy_string_eq_slice(const SyString* lhs, SyStringSlice rhs);

SY_API SyAllocErr sy_string_concat(const SyString* self, SyStringSlice str, SyString* out);

SY_API SyAllocErr sy_string_builder_init(SyAllocator alloc, SyStringBuilder* out);

SY_API SyAllocErr sy_string_builder_init_with_capacity(size_t inCapacity, SyAllocator alloc,
                                                       SyStringBuilder* out);

SY_API SyAllocErr sy_string_builder_build(SyStringBuilder* self, SyString* out);

SY_API SyAllocErr sy_string_builder_write(SyStringBuilder* self, SyStringSlice str);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SY_TYPES_STRING_STRING_H_
