#include "sync_queue.h"
#include "sync_queue.hpp"
#include "../util/assert.hpp"
#include "../util/unreachable.hpp"
#include "../mem/allocator.hpp"
#include "alloc_cache_align.hpp"
#include <utility>
#include <new>

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

class SyncQueue {
public:

    SyncQueue() = default;

    ~SyncQueue() noexcept;

    uint16_t len() const { return this->_len; }
    
    bool isAcquired() const { return _isAcquired; }

    void acquire();
     
    bool tryAcquire();

    void release();

    void add(InQueueSyncObj&& obj);

private:

    void addExtraCapacity();

private:
    InQueueSyncObj* _objects = nullptr;
    uint16_t        _len = 0;
    uint16_t        _capacity = 0;
    bool            _isAcquired = false;
};

class SyncQueueStack {
public:

    SyncQueueStack();

    ~SyncQueueStack() noexcept;

    SyncQueue& top();

    void push();

    void pop();

private:

    void ensureCapacityForCurrent();

private:

    /// All queues up to `capacity` are valid.
    SyncQueue* queues;
    size_t current;
    size_t capacity;
};

static thread_local SyncQueueStack queues;

SY_API void sy::sync_queue::lock() {
    queues.top().acquire();
}

SY_API bool sy::sync_queue::tryLock() {
    return queues.top().tryAcquire();
}

SY_API void sy::sync_queue::unlock() {
    queues.top().release();
}

SY_API void sy::sync_queue::addExclusive(SyncObject &&obj) {
    queues.top().add(InQueueSyncObj(std::move(obj), LockAcquireType::Exclusive));
}

SY_API void sy::sync_queue::addShared(SyncObject&& obj) {
    queues.top().add(InQueueSyncObj(std::move(obj), LockAcquireType::Shared));
}

SyncQueue::~SyncQueue() noexcept
{
    if(this->_objects == nullptr) return;

    sy::Allocator alloc{};
    alloc.freeAlignedArray(this->_objects, this->_capacity, ALLOC_CACHE_ALIGN);

    this->_objects = nullptr;
    this->_len = 0;
    this->_capacity = 0;
    this->_isAcquired = false;
}

void SyncQueue::acquire()
{
    for(uint16_t i = 0; i < this->_len; i++) {
        const InQueueSyncObj& inQueue = this->_objects[i];
        SyncObject obj = inQueue.getObj();
        switch(inQueue.getAcquireType()) {
            case LockAcquireType::Exclusive: {
                obj.vtable->lockExclusive(obj.ptr);
            } break;
            case LockAcquireType::Shared: {
                obj.vtable->lockShared(obj.ptr);
            } break;
            default: unreachable();
        }
    }
    this->_isAcquired = true;
}

bool SyncQueue::tryAcquire()
{
    uint16_t i = 0;
    bool didAcquireAll = true;
    for(; i < this->_len; i++) {
        const InQueueSyncObj& inQueue = this->_objects[i];
        SyncObject obj = inQueue.getObj();
        switch(inQueue.getAcquireType()) {
            case LockAcquireType::Exclusive: {
                if(!obj.vtable->tryLockExclusive(obj.ptr)) {
                    didAcquireAll = false;
                    break;
                }
            } break;
            case LockAcquireType::Shared: {
                if(!obj.vtable->tryLockShared(obj.ptr)) {
                    didAcquireAll = false;
                    break;
                }
            } break;
            default: unreachable();
        }
    }

    if(didAcquireAll) {
        this->_isAcquired = true;
        return true;
    }

    while(i > 0) {
        i -= 1;
      
        const InQueueSyncObj& inQueue = this->_objects[i];
        SyncObject obj = inQueue.getObj();
        switch(inQueue.getAcquireType()) {
            case LockAcquireType::Exclusive: {
                obj.vtable->unlockExclusive(obj.ptr);
            } break;
            case LockAcquireType::Shared: {
                obj.vtable->unlockShared(obj.ptr);
            } break;
            default: unreachable();
        }
    }
    this->_len = 0; // "Clear" the currently held sync objects
    return false;
}

void SyncQueue::release()
{
    for(uint16_t i = 0; i < this->_len; i++) {
        const InQueueSyncObj& inQueue = this->_objects[i];
        SyncObject obj = inQueue.getObj();
        switch(inQueue.getAcquireType()) {
            case LockAcquireType::Exclusive: {
                obj.vtable->unlockExclusive(obj.ptr);
            } break;
            case LockAcquireType::Shared: {
                obj.vtable->unlockShared(obj.ptr);
            } break;
            default: unreachable();
        }
    }
    this->_isAcquired = false;
    this->_len = 0;
}

void SyncQueue::add(InQueueSyncObj &&obj)
{
    sy_assert(this->_len < UINT16_MAX, "Cannot add any more objects to sync");
    addExtraCapacity();

    const uintptr_t objPtrVal = reinterpret_cast<uintptr_t>(obj.getObj().ptr);
    for(uint16_t i = 0; i < this->_len; i++) {
        const uintptr_t inQueuePtrVal = reinterpret_cast<uintptr_t>(this->_objects[i].getObj().ptr);
        if(objPtrVal == inQueuePtrVal) {
            // duplicate entry
            // todo what happens when mismatch of lock acquire type?
            return;
        }

        // The entries are sorted from lowest address to highest address

        if(inQueuePtrVal < objPtrVal) {
            continue;
        }

        uint16_t moveIter = this->_len; // guaranteed to be greater than 0
        while(moveIter > i) {
            moveIter -= 1;
            sy_assert(this->_capacity > (moveIter + 1), "Why no capacity?");
            this->_objects[moveIter + 1] = std::move(this->_objects[moveIter]);
        }

        this->_objects[this->_len] = std::move(obj);
        this->_len += 1;
        return;
    }

    this->_objects[this->_len] = std::move(obj);
    this->_len += 1;
}

void SyncQueue::addExtraCapacity()
{
    size_t newCapacity = this->_capacity == 0 ?
        ALLOC_CACHE_ALIGN / sizeof(InQueueSyncObj)
        : this->_capacity * 1.5;
    sy_assert(newCapacity <= UINT16_MAX, "Sync queue maximum capacity is max uint16");

    sy::Allocator alloc{};
    InQueueSyncObj* newObjects = alloc.allocAlignedArray<InQueueSyncObj>(newCapacity, ALLOC_CACHE_ALIGN).get();
    if(this->_capacity > 0) {
        for(uint16_t i = 0; i < this->_len; i++) {
            newObjects[i] = this->_objects[i];
        }
        alloc.freeAlignedArray(this->_objects, this->_capacity, ALLOC_CACHE_ALIGN);
    }
    this->_objects = newObjects;
    this->_capacity = newCapacity;
}

SyncQueueStack::SyncQueueStack()
    : current(0)
{
    constexpr size_t DEFAULT_CAPACITY = ALLOC_CACHE_ALIGN / sizeof(SyncQueue);
    sy::Allocator alloc{};
    SyncQueue* newQueues = alloc.allocAlignedArray<SyncQueue>(DEFAULT_CAPACITY, ALLOC_CACHE_ALIGN).get();
    for(size_t i = 0; i < DEFAULT_CAPACITY; i++) {
        SyncQueue* placed = new (&newQueues[i]) SyncQueue;
        (void)placed;
    }
    this->queues = newQueues;
    this->capacity = DEFAULT_CAPACITY;
}

SyncQueueStack::~SyncQueueStack() noexcept
{
    if(this->queues == nullptr) return;

    sy::Allocator alloc{};
    for(size_t i = 0; i < this->capacity; i++) {
        this->queues[i].~SyncQueue();
    }
    alloc.freeAlignedArray(this->queues, this->capacity, ALLOC_CACHE_ALIGN);

    this->queues = nullptr;
    this->current = 0;
    this->capacity = 0;
}

SyncQueue &SyncQueueStack::top()
{
    sy_assert(this->current < this->capacity, "Invalid memory access");
    return this->queues[this->current];
}

void SyncQueueStack::push()
{
    this->current += 1;
    this->ensureCapacityForCurrent();
}

void SyncQueueStack::pop()
{
    sy_assert(this->current > 0, "No more queues to pop");
    this->current -= 1;
}

void SyncQueueStack::ensureCapacityForCurrent()
{
    if(this->current < this->capacity) return;

    const size_t newCapacity = this->capacity == 0 ?
        ALLOC_CACHE_ALIGN / sizeof(SyncQueue)
        : this->capacity * 1.5;
    
    sy::Allocator alloc{};
    SyncQueue* newQueues = alloc.allocAlignedArray<SyncQueue>(newCapacity, ALLOC_CACHE_ALIGN).get();
    for(size_t i = 0; i < newCapacity; i++) {
        SyncQueue* placed = new (&newQueues[i]) SyncQueue;
        (void)placed;
    }

    if(this->capacity > 0) {
        for(size_t i = 0; i < this->current; i++) {
            newQueues[i] = std::move(this->queues[i]);
        }
        alloc.freeAlignedArray(this->queues, this->capacity, ALLOC_CACHE_ALIGN);
    }
    this->queues = newQueues;
    this->capacity = newCapacity;
}
