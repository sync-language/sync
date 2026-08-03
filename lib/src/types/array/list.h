//! API
#pragma once
#ifndef SY_TYPES_ARRAY_LIST_H_
#define SY_TYPES_ARRAY_LIST_H_

#include "../../core/builtin_traits/builtin_traits.h"
#include "../../mem/allocator.h"

struct SyType;

typedef struct SyList {
    /// Internal only. Default to `nullptr`.
    void* data_;
    /// The number of elements in the array. Default to `0`.
    size_t len;
    /// Internal only. Default to `0`.
    size_t capacity_;
    /// Internal only. Default to `nullptr`.
    void* allocated_;
    SyAllocator allocator;
} SyList;

#ifdef __cplusplus
extern "C" {
#endif

SY_API void sy_list_destroy(SyList* self, size_t typeSize, size_t typeAlign,
                            SyNativeDestructorFn typeDestruct);

SY_API void sy_list_destroy_script(SyList* self, const struct SyType* dataType);

SY_API SyExceptional sy_list_clone(const SyList* self, SyList* out, size_t typeSize,
                                   size_t typeAlign, SyNativeCloneFn typeClone,
                                   SyNativeDestructorFn typeDestruct);

SY_API SyExceptional sy_list_clone_script(const SyList* self, SyList* out,
                                          const struct SyType* dataType);

SY_API SyAllocErr sy_list_push(SyList* self, void* obj, size_t typeSize, size_t typeAlign);

SY_API SyAllocErr sy_list_push_script(SyList* self, void* obj, const struct SyType* dataType);

SY_API SyAllocErr sy_list_push_front(SyList* self, void* obj, size_t typeSize, size_t typeAlign);

SY_API SyAllocErr sy_list_push_front_script(SyList* self, void* obj, const struct SyType* dataType);

SY_API SyAllocErr sy_list_insert_at(SyList* self, void* obj, size_t index, size_t typeSize,
                                    size_t typeAlign);

SY_API SyAllocErr sy_list_insert_at_script(SyList* self, void* obj, size_t index,
                                           const SyType* dataType);

SY_API void sy_list_remove_at(SyList* self, size_t index, size_t typeSize,
                              SyNativeDestructorFn typeDestruct);

SY_API void sy_list_remove_at_script(SyList* self, size_t index, const struct SyType* dataType);

SY_API SyAllocErr sy_list_reserve(SyList* self, size_t minCapacity, size_t typeSize,
                                  size_t typeAlign);

SY_API SyAllocErr sy_list_reserve_script(SyList* self, size_t minCapacity,
                                         const struct SyType* dataType);

SY_API SyAllocErr sy_list_reserve_front(SyList* self, size_t minCapacity, size_t typeSize,
                                        size_t typeAlign);

SY_API SyAllocErr sy_list_reserve_front_script(SyList* self, size_t minCapacity,
                                               const struct SyType* dataType);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SY_TYPES_ARRAY_LIST_H_
