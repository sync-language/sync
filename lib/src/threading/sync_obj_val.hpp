#pragma once
#ifndef SY_THREADING_SYNC_OBJ_VAL_HPP_
#define SY_THREADING_SYNC_OBJ_VAL_HPP_

#include "../core.h"
#include "alloc_cache_align.hpp"
#include <atomic>
#include <shared_mutex>

namespace sy {
    struct Type;
}

class alignas(ALLOC_CACHE_ALIGN) SyncObjVal {
public:

    void lockExclusive() { this->lock.lock(); }

    bool tryLockExclusive() { return this->lock.try_lock(); }

    void unlockExclusive() { return this->lock.unlock(); }

    void lockShared() const { this->lock.lock_shared(); }

    bool tryLockShared() const { return this->lock.try_lock_shared(); }

    void unlockShared() const { return this->lock.unlock_shared(); }

    bool expired() const { return isExpired.load(); }

    void addWeakCount();

    bool removeWeakCount();

    void addSharedCount();

    bool removeSharedCount();

    /// Lock should NOT be acquired
    /// Marks the obj ref as expired.
    void destroyHeldObject();

    const void* valueMem(const size_t alignType) const;

    void* valueMemMut(const size_t alignType);

private:

    /// Does not initialize the object's memory itself, only zero initialized.
    static SyncObjVal* createNew(const size_t sizeType, const size_t alignType);

    uintptr_t valueMemLocation(const size_t alignType) const;

    SyncObjVal();

private:
    mutable std::shared_mutex lock{};
    std::atomic<size_t> sharedCount;
    std::atomic<size_t> weakCount;
    std::atomic<bool> isExpired;
};

#endif // SY_THREADING_SYNC_OBJ_VAL_HPP_