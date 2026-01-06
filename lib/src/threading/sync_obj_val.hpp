#pragma once
#ifndef SY_THREADING_SYNC_OBJ_VAL_HPP_
#define SY_THREADING_SYNC_OBJ_VAL_HPP_

#include "../core/core.h"
#include "../mem/allocator.hpp"
#include "../types/result/result.hpp"
#include "alloc_cache_align.hpp"
#include <atomic>
#include <shared_mutex>

namespace sy {
class Type;

#if defined(_MSC_VER)
// Supress warning for struct padding due to alignment specifier
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4324?view=msvc-170
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
class alignas(ALLOC_CACHE_ALIGN) SyncObjVal {
  public:
    /// Does not initialize the object's memory itself, only zero initialized.
    static Result<SyncObjVal*, AllocErr> create(Allocator alloc, const size_t inSizeType, const uint16_t inAlignType);

    void destroy();

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
    void destroyHeldObjectCFunction(void (*destruct)(void* ptr));

    void destroyHeldObjectScriptFunction(const sy::Type* typeInfo);

    const void* valueMem() const;

    void* valueMemMut();

    bool noWeakRefs() const { return this->weakCount.load() == 0; }

  private:
    uintptr_t valueMemLocation() const;

    SyncObjVal(Allocator alloc, size_t inSizeType, uint16_t inAlignType);

    ~SyncObjVal() = default;

  private:
    mutable std::shared_mutex lock{};
    Allocator allocator;
    size_t sizeType;
    std::atomic<size_t> sharedCount;
    std::atomic<size_t> weakCount;
    std::atomic<bool> isExpired;
    uint16_t alignType;
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
} // namespace sy

#endif // SY_THREADING_SYNC_OBJ_VAL_HPP_