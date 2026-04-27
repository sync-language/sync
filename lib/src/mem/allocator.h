//! API
#pragma once
#ifndef SY_MEM_ALLOCATOR_H_
#define SY_MEM_ALLOCATOR_H_

#include "../core/core.h"

///
typedef void* (*sy_allocator_alloc_fn)(void* self, size_t len, size_t align);

///
typedef void (*sy_allocator_free_fn)(void* self, void* buf, size_t len, size_t align);

typedef struct SyAllocatorVTable {
    sy_allocator_alloc_fn allocFn;
    sy_allocator_free_fn freeFn;
} SyAllocatorVTable;

/// Should not be copied. The default allocator can be dereferenced and used in most places:
/// ```
/// *sy_defaultAllocator
/// ```
typedef struct SyAllocator {
    void* ptr;
    const SyAllocatorVTable* vtable;
} SyAllocator;

typedef enum SyAllocErr {
    /// This NONE variant means no error happened.
    SY_ALLOC_ERR_NONE = 0,
    SY_ALLOC_ERR_OUT_OF_MEMORY = 1,
} SyAllocErr;

#ifdef __cplusplus
extern "C" {
#endif

/// @returns `NULL` if memory allocation fails for whatever reason.
SY_API void* sy_allocator_alloc(SyAllocator* self, size_t len, size_t align);

/// @param `buf` Non-null.
SY_API void sy_allocator_free(SyAllocator* self, void* buf, size_t len, size_t align);

SY_API extern SyAllocator* const sy_defaultAllocator;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SY_MEM_ALLOCATOR_H_
