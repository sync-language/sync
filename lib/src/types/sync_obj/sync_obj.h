//! API
#pragma once
#ifndef SY_TYPES_SYNC_OBJ_SYNC_OBJ_H_
#define SY_TYPES_SYNC_OBJ_SYNC_OBJ_H_

#include "../../core.h"
#include "../../threading/sync_queue.h"

struct SyType;

/// Not meaningful when zero initialized. Still able to be destroyed without crashing.
typedef struct SyOwned {
    void* inner;
} SyOwned;

/// Not meaningful when zero initialized. Still able to be destroyed without crashing.
typedef struct SyShared {
    void* inner;
} SyShared;

/// Not meaningful when zero initialized. Still able to be destroyed without crashing.
typedef struct SyWeak {
    void* inner;
} SyWeak;

#ifdef __cplusplus
extern "C" {
#endif

SY_API SyOwned sy_owned_init(void* value, const size_t sizeType, const size_t alignType);

SY_API SyOwned sy_owned_init_script_typed(void* value, const struct SyType* typeInfo);

SY_API void sy_owned_destroy(SyOwned* self, void (*destruct)(void* ptr), const size_t sizeType);

SY_API void sy_owned_destroy_script_typed(SyOwned* self, const struct SyType* typeInfo);

SY_API SyWeak sy_owned_make_weak(const SyOwned* self);

SY_API void sy_owned_lock_exclusive(SyOwned* self);

SY_API bool sy_owned_try_lock_exclusive(SyOwned* self);

SY_API void sy_owned_unlock_exclusive(SyOwned* self);

SY_API void sy_owned_lock_shared(const SyOwned* self);

SY_API bool sy_owned_try_lock_shared(const SyOwned* self);

SY_API void sy_owned_unlock_shared(const SyOwned* self);

SY_API const void* sy_owned_get(const SyOwned* self);

SY_API void* sy_owned_get_mut(SyOwned* self);

SY_API SySyncObject sy_owned_to_queue_obj(const SyOwned* self);

SY_API SyShared sy_shared_init(void* value, const size_t sizeType, const size_t alignType);

SY_API SyShared sy_shared_init_script_typed(void* value, const struct SyType* typeInfo);

SY_API SyShared sy_shared_clone(const SyShared* self);

SY_API void sy_shared_destroy(SyShared* self, void (*destruct)(void* ptr), const size_t sizeType);

SY_API void sy_shared_destroy_script_typed(SyShared* self, const struct SyType* typeInfo);

SY_API SyWeak sy_shared_make_weak(const SyShared* self);

SY_API void sy_shared_lock_exclusive(SyShared* self);

SY_API bool sy_shared_try_lock_exclusive(SyShared* self);

SY_API void sy_shared_unlock_exclusive(SyShared* self);

SY_API void sy_shared_lock_shared(const SyShared* self);

SY_API bool sy_shared_try_lock_shared(const SyShared* self);

SY_API void sy_shared_unlock_shared(const SyShared* self);

SY_API const void* sy_shared_get(const SyShared* self);

SY_API void* sy_shared_get_mut(SyShared* self);

SY_API SySyncObject sy_shared_to_queue_obj(const SyShared* self);

SY_API void sy_weak_lock_exclusive(SyWeak* self);

SY_API bool sy_weak_try_lock_exclusive(SyWeak* self);

SY_API void sy_weak_unlock_exclusive(SyWeak* self);

SY_API void sy_weak_lock_shared(const SyWeak* self);

SY_API bool sy_weak_try_lock_shared(const SyWeak* self);

SY_API void sy_weak_unlock_shared(const SyWeak* self);

SY_API bool sy_weak_expired(const SyWeak* self);

SY_API const void* sy_weak_get(const SyWeak* self);

SY_API void* sy_weak_get_mut(SyWeak* self);

SY_API SySyncObject sy_weak_to_queue_obj(const SyWeak* self);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SY_TYPES_SYNC_OBJ_SYNC_OBJ_H_