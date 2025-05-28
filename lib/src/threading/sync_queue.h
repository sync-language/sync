//! API
#pragma once
#ifndef SY_THREADING_SYNC_QUEUE_H_
#define SY_THREADING_SYNC_QUEUE_H_

#include "../core.h"

typedef void(*SySyncQueueLockExclusive)(void* lock);
typedef bool(*SySyncQueueTryLockExclusive)(void* lock);
typedef void(*SySyncQueueUnlockExclusive)(void* lock);
typedef void(*SySyncQueueLockShared)(const void* lock);
typedef bool(*SySyncQueueTryLockShared)(const void* lock);
typedef void(*SySyncQueueUnlockShared)(const void* lock);

typedef struct SySyncObjectVTable {
    SySyncQueueLockExclusive lockExclusive;
    SySyncQueueTryLockExclusive tryLockExclusive;
    SySyncQueueUnlockExclusive unlockExclusive;
    SySyncQueueLockShared lockShared;
    SySyncQueueTryLockShared tryLockShared;
    SySyncQueueUnlockShared unlockShared;
} SySyncObjectVTable;

typedef struct SySyncObject {
    void* ptr;
    const SySyncObjectVTable* vtable;
} SySyncObject;

#ifdef __cplusplus
extern "C" {
#endif

SY_API void sy_sync_queue_lock();

SY_API bool sy_sync_queue_try_lock();

SY_API void sy_sync_queue_unlock();

SY_API void sy_sync_queue_add_exclusive(SySyncObject obj);

SY_API void sy_sync_queue_add_shared(SySyncObject obj);

#ifdef __cplusplus
} // extern "C"
#endif


#endif // SY_THREADING_SYNC_QUEUE_H_