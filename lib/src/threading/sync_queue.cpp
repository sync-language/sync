#include "sync_queue.h"
#include "sync_queue.hpp"
#include <utility>
#include <vector>

using sy::sync_queue::SyncObject;

extern "C" {
    SY_API void sy_sync_queue_lock() {
        sy::sync_queue::lock();
    }

    SY_API bool sy_sync_queue_try_lock() {
        return sy::sync_queue::tryLock();
    }

    SY_API void sy_sync_queue_unlock() {
        return sy::sync_queue::unlock();
    }

    SY_API void sy_sync_queue_add_exclusive(SySyncObject obj) {
        SyncObject object = *reinterpret_cast<SyncObject*>(&obj);
        sy::sync_queue::addExclusive(std::move(object));
    }

    SY_API void sy_sync_queue_add_shared(SySyncObject obj) {
        SyncObject object = *reinterpret_cast<SyncObject*>(&obj);
        sy::sync_queue::addShared(std::move(object));
    }
}

enum class LockAcquireType : uint8_t {
    Exclusive = 0,
    Shared = 1,
};

#if defined(__x86_64__) || defined (_M_AMD64) || defined(__aarch64__) || defined(_M_ARM64)

static_assert(sizeof(uintptr_t) == 8);

/// Since not all of the bits in a pointer are used on X86_64 and ARM64, we can store the lock acquire type within
/// the unused bits.
class InQueueSyncObj {
public:
    InQueueSyncObj(SyncObject&& obj, const LockAcquireType acquireType) 
        : vtable(obj.vtable)
    {
        taggedPtr = 
            reinterpret_cast<uintptr_t>(obj.ptr) | 
            (static_cast<uintptr_t>(acquireType) << SHIFT_AMOUNT);
    }

    SyncObject getObj() const {
        SyncObject obj;
        obj.ptr = reinterpret_cast<void*>(taggedPtr & OBJECT_PTR_BITMASK);
        obj.vtable = vtable;
        return obj;
    }

    LockAcquireType getAcquireType() const {
        return static_cast<LockAcquireType>((taggedPtr & TAG_BITMASK) >> SHIFT_AMOUNT);
    }

private:
    static constexpr uintptr_t OBJECT_PTR_BITMASK = 0xFFFFFFFFFFFFFF;
    static constexpr uintptr_t TAG_BITMASK = ~OBJECT_PTR_BITMASK;
    static constexpr int SHIFT_AMOUNT = 56;

    uintptr_t taggedPtr;
    const SyncObject::VTable* vtable;
};
#else
class InQueueSyncObj {
public:
    InQueueSyncObj(SyncObject&& obj, const LockAcquireType acquireType) 
        : _obj(std::move(obj)), _acquireType(acquireType)
    {}

    SyncObject getObj() const {
        return _obj;
    }

    LockAcquireType getAcquireType() const {
        return _acquireType;
    }
private:
    SyncObject      _obj;
    LockAcquireType _acquireType;
};
#endif

SY_API void sy::sync_queue::lock();

SY_API bool sy::sync_queue::tryLock();

SY_API void sy::sync_queue::unlock();

SY_API void sy::sync_queue::addExclusive(SyncObject &&obj);

SY_API void sy::sync_queue::addShared(SyncObject&& obj);