//! API
#pragma once
#ifndef SY_THREADING_LOCKS_RWLOCK_H_
#define SY_THREADING_LOCKS_RWLOCK_H_

#include "../../core/core.h"

typedef union SyRwLock {
    struct Padding {
        uint8_t _p[32];
    } _padding;
    void* _forceAlign;
} SyRwLock;

typedef enum SyAcquireErr {
    SY_ACQUIRE_ERR_NONE = 0,
    SY_ACQUIRE_ERR_OUT_OF_MEMORY = 1,
    SY_ACQUIRE_ERR_HAS_EXCLUSIVE = 2,
    SY_ACQUIRE_ERR_HAS_READERS = 3,
    SY_ACQUIRE_ERR_DEADLOCK = 4,

    _SY_ACQUIRE_ERR_MAX = 0x7FFFFFFF
} SyAcquireErr;

#ifdef __cplusplus
extern "C" {
#endif

/// Destroys this RwLock, freeing any allocated memory.
/// @param self Non-null pointer to `SyRwLock` object.
/// @warning Asserts:
///
/// - No thread has an exclusive lock.
///
/// - No thread has a shared lock.
///
/// - No thread is trying to elevate from a shared lock to an exclusive lock.
SY_API void sy_rwlock_destroy(SyRwLock* self);

/// Acquires a shared (read-only) lock on this rwlock.
/// @param self Non-null pointer to `SyRwLock` object.
/// @return `SY_ACQUIRE_ERR_NONE` on success, or `SY_ACQUIRE_ERR_OUT_OF_MEMORY` if there was an
/// allocation failure.
SY_API SyAcquireErr sy_rwlock_lock_shared(SyRwLock* self);

/// Attempts to acquire a shared (read-only) lock on this rwlock.
/// @param self Non-null pointer to `SyRwLock` object.
/// @return `SY_ACQUIRE_ERR_NONE` on success. On failure, returns either
///
/// - `SY_ACQUIRE_ERR_OUT_OF_MEMORY` if there was an allocation failure, or
///
/// - `SY_ACQUIRE_ERR_HAS_EXCLUSIVE` if there is an exclusive lock not owned by this thread.
SY_API SyAcquireErr sy_rwlock_try_lock_shared(SyRwLock* self);

/// Unlocks a shared (read-only) lock held by this thread.
/// @param self Non-null pointer to `SyRwLock` object.
/// @warning Debug asserts:
/// - This rwlock is not exclusively locked by another thread
/// - This rwlock was shared-locked by this thread
SY_API void sy_rwlock_unlock_shared(SyRwLock* self);

/// Acquires a shared (read-only) lock on this rwlock.
/// @param self Non-null pointer to `SyRwLock` object.
/// @warning Invokes the sync fatal error handler if memory allocation fails.
SY_API void sy_rwlock_lock_shared_unchecked(SyRwLock* self);

/// Acquires an exlusive (read-write) lock on this rwlock.
/// @param self Non-null pointer to `SyRwLock` object.
/// @return `SY_ACQUIRE_ERR_NONE` on success. On failure:
///
/// - `SY_ACQUIRE_ERR_OUT_OF_MEMORY` if there was an allocation failure.
///
/// - `SY_ACQUIRE_ERR_DEADLOCK` if other threads have shared locks, or if this thread and one or
/// more others are attempting to elevate theirs at the same time.
SY_API SyAcquireErr sy_rwlock_lock_exclusive(SyRwLock* self);

/// Attempts to acquire an exlusive (read-write) lock on this rwlock.
/// @param self Non-null pointer to `SyRwLock` object.
/// @return `SY_ACQUIRE_ERR_NONE` on success. On failure:
///
/// - `SY_ACQUIRE_ERR_OUT_OF_MEMORY` if there was an allocation failure.
///
/// - `SY_ACQUIRE_ERR_HAS_EXCLUSIVE` if there is an exclusive lock not owned by this thread.
///
/// - `SY_ACQUIRE_ERR_HAS_READERS` if one or more other threads are holding a shared lock.
///
/// - `SY_ACQUIRE_ERR_DEADLOCK` if this thread and one or more others are attempting to elevate
/// theirs at the same time.
/// @warning Debug asserts:
/// - This rwlock was re-entered through exclusive locking less than UINT16_MAX times.
SY_API SyAcquireErr sy_rwlock_try_lock_exclusive(SyRwLock* self);

/// Unlocks an exclusive (read-write) lock held by this thread.
/// @param self Non-null pointer to `SyRwLock` object.
/// @warning Debug asserts:
///
/// - This rwlock is not exclusively locked by another thread
///
/// - This rwlock was exclusively locked by this thread
SY_API void sy_rwlock_unlock_exclusive(SyRwLock* self);

/// Acquires an exclusive (read-write) lock on this rwlock.
/// @param self Non-null pointer to `SyRwLock` object.
/// @warning Invokes the sync fatal error handler if memory allocation fails or if a deadlock
/// occurred.
SY_API void sy_rwlock_lock_exclusive_unchecked(SyRwLock* self);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SY_THREADING_LOCKS_RWLOCK_H_
