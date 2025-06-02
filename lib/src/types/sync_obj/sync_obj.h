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

SyOwned sy_owned_init(void* value, const size_t sizeType, const size_t alignType);

SyOwned sy_owned_init_script_typed(void* value, const struct SyType* typeInfo);

void sy_owned_destroy(SyOwned* self, void (*destruct)(void* ptr), const size_t sizeType);

void sy_owned_destroy_script_typed(SyOwned* self, const struct SyType* typeInfo);

void sy_owned_lock_exclusive(SyOwned* self);

bool sy_owned_try_lock_exclusive(SyOwned* self);

void sy_owned_unlock_exclusive(SyOwned* self);

void sy_owned_lock_shared(const SyOwned* self);

bool sy_owned_try_lock_shared(const SyOwned* self);

void sy_owned_unlock_shared(const SyOwned* self);

const void* sy_owned_get(const SyOwned* self);

void* sy_owned_get_mut(SyOwned* self);

SySyncObject sy_owned_to_queue_obj(const SyOwned* self);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SY_TYPES_SYNC_OBJ_SYNC_OBJ_H_