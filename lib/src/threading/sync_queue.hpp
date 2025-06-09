//! API
#pragma once
#ifndef SY_THREADING_SYNC_QUEUE_HPP_
#define SY_THREADING_SYNC_QUEUE_HPP_

#include "../core.h"

namespace sy {
    namespace sync_queue {
        class SY_API SyncObject {
        public:
            class VTable {
            public:
                using LockExclusive     = void(*)(void* lock);
                using TryLockExclusive  = bool(*)(void* lock);
                using UnlockExclusive   = void(*)(void* lock);
                using LockShared        = void(*)(const void* lock);
                using TryLockShared     = bool(*)(const void* lock);
                using UnlockShared      = void(*)(const void* lock);

                LockExclusive       lockExclusive;
                TryLockExclusive    tryLockExclusive;
                UnlockExclusive     unlockExclusive;
                LockShared          lockShared;
                TryLockShared       tryLockShared;
                UnlockShared        unlockShared;
            };

            void* ptr;
            const VTable* vtable;
        };
        
        SY_API void lock();

        SY_API bool tryLock();

        SY_API void unlock();

        SY_API void addExclusive(SyncObject&& obj);

        SY_API void addShared(SyncObject&& obj);
    }
}

#endif // SY_THREADING_SYNC_QUEUE_HPP_