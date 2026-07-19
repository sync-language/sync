//! API
#pragma once
#ifndef SY_TYPES_ARRAY_LIST_H_
#define SY_TYPES_ARRAY_LIST_H_

#include "../../core/builtin_traits/builtin_traits.h"
#include "../../mem/allocator.h"

typedef struct SyList {
    /// Internal only. Default to `nullptr`.
    void* data_;
    /// The number of elements in the array. Default to `0`.
    size_t len;
    /// Internal only. Default to `0`.
    size_t capacity_;
    /// Internal only. Default to `nullptr`.
    void* allocated_ = nullptr;
    SyAllocator allocator;
} SyList;

#ifdef __cplusplus
extern "C" {
#endif

void sy_list_destroy(SyList* self, size_t typeSize, size_t typeAlign,
                     SyNativeDestructorFn destruct);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SY_TYPES_ARRAY_LIST_H_
