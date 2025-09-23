#include "sync_queue.h"
#include "../mem/allocator.hpp"
#include "../util/assert.hpp"
#include "../util/unreachable.hpp"
#include "alloc_cache_align.hpp"
#include "sync_queue.hpp"
#include <new>
#include <utility>

using sy::AllocErr;
using sy::Result;
using sy::sync_queue::SyncObject;

extern "C" {
SY_API void sy_sync_queue_lock() { sy::sync_queue::lock(); }

SY_API bool sy_sync_queue_try_lock() { return sy::sync_queue::tryLock(); }

SY_API void sy_sync_queue_unlock() { return sy::sync_queue::unlock(); }

SY_API void sy_sync_queue_add_exclusive(SySyncObject obj) {
    SyncObject object = *reinterpret_cast<SyncObject*>(&obj);
    (void)sy::sync_queue::addExclusive(std::move(object));
}

SY_API void sy_sync_queue_add_shared(SySyncObject obj) {
    SyncObject object = *reinterpret_cast<SyncObject*>(&obj);
    (void)sy::sync_queue::addShared(std::move(object));
}
}

enum class LockAcquireType : uint8_t {
    Exclusive = 0,
    Shared = 1,
};

static constexpr size_t syncQueueGrowCapacity(size_t currentCapacity) {
    // Keeps within initial allocation alignment
    constexpr size_t defaultCapacity = 4;
    size_t newCapacity =
        currentCapacity == 0 ? defaultCapacity : static_cast<size_t>(static_cast<double>(currentCapacity) * 1.5);
    return newCapacity;
}

static constexpr size_t syncQueueByteAllocSize(size_t capacity) {
    size_t minBytes = (sizeof(uintptr_t)    // objects themselves
                       + sizeof(uintptr_t)) // vtable and acquire type
                      * capacity;
    size_t remainder = minBytes % ALLOC_CACHE_ALIGN;
    if (remainder == 0)
        return minBytes;
    return minBytes + (minBytes - remainder);
}

static uintptr_t* syncQueueVTablesAndAcquireType(uintptr_t* objects, size_t capacity) { return objects + capacity; }

/// @brief
/// @param toFind
/// @param objects
/// @param high
/// @return The index where to insert, or an error containing the index of the duplicate
static Result<uint16_t, uint16_t> syncQueueWhereToInsert(const uintptr_t toFind, const uintptr_t* objects,
                                                         uint16_t high) {

    high -= 1;
    // Should be placed directly at the beginning or end?
    if (toFind < objects[0]) {
        return 0;
    } else if (toFind == objects[0]) {
        return sy::Error<uint16_t>(0);
    }
    if (toFind > objects[high]) {
        return high + 1;
    } else if (toFind == objects[high]) {
        return sy::Error<uint16_t>(high);
    }

    if (high == 0) { // only one entry
        if (toFind == objects[high]) {
            return sy::Error(high);
        }
        // it is less
        return 0;
    }

    // no longer check the end
    uint16_t low = 0;
    uint16_t mid = high / 2;

    while (true) {
        const uintptr_t midObj = objects[mid];
        if (toFind == midObj) {
            // duplicate entry
            return sy::Error(mid);
        }

        if (low == mid) {
            sy_assert(toFind > midObj, "Should've been greater");
            sy_assert(toFind < objects[mid + 1], "Should've been less");
            return mid + 1;
        }

        //
        if (toFind > midObj) {
            if (toFind < objects[mid + 1]) {
                return mid + 1;
            }
            low = mid;
            mid += (high - mid) / 2;
        } else {
            high = mid;
            mid /= 2;
        }
    }
}

namespace sy {
namespace sync_queue {
class SyncQueue {
  public:
    SyncQueue() = default;

    ~SyncQueue() noexcept;

    uint16_t len() const { return this->len_; }

    bool isAcquired() const { return isAcquired_; }

    void acquire();

    bool tryAcquire();

    void release();

    [[nodiscard]] Result<void, AllocErr> add(SyncObject obj, LockAcquireType type);

  private:
    [[nodiscard]] Result<void, AllocErr> addExtraCapacity();

    SyncObject objAt(uint16_t index) const;

    LockAcquireType acquireTypeAt(uint16_t index) const;

    uintptr_t* objects_ = nullptr;
    uint16_t len_ = 0;
    uint16_t capacity_ = 0;
    bool isAcquired_ = false;
};

class SyncQueueStack {
  public:
    SyncQueueStack();

    ~SyncQueueStack() noexcept;

    SyncQueue& top();

    void push();

    void pop();

    [[nodiscard]] Result<void, AllocErr> ensureCapacityForOneAfter();

  private:
    /// All queues up to `capacity` are valid.
    SyncQueue* queues;
    size_t current;
    size_t capacity;
};

static thread_local SyncQueueStack queues{};
} // namespace sync_queue
} // namespace sy

SY_API void sy::sync_queue::lock() {
    queues.top().acquire();
    queues.push();
}

SY_API bool sy::sync_queue::tryLock() {
    const bool success = queues.top().tryAcquire();
    if (success) {
        queues.push();
    }
    return success;
}

SY_API void sy::sync_queue::unlock() {
    queues.top().release();
    queues.pop();
}

SY_API Result<void, AllocErr> sy::sync_queue::addExclusive(SyncObject obj) noexcept {
    if (!queues.ensureCapacityForOneAfter()) {
        return Error(AllocErr::OutOfMemory);
    }
    return queues.top().add(obj, LockAcquireType::Exclusive);
}

SY_API Result<void, AllocErr> sy::sync_queue::addShared(SyncObject obj) noexcept {
    if (!queues.ensureCapacityForOneAfter()) {
        return Error(AllocErr::OutOfMemory);
    }
    return queues.top().add(std::move(obj), LockAcquireType::Shared);
}

sy::sync_queue::SyncQueue::~SyncQueue() noexcept {
    Allocator alloc{};
    alloc.freeAlignedArray(reinterpret_cast<uint8_t*>(this->objects_), syncQueueByteAllocSize(this->capacity_),
                           ALLOC_CACHE_ALIGN);

    this->objects_ = nullptr;
    this->len_ = 0;
    this->capacity_ = 0;
    this->isAcquired_ = false;
}

void sy::sync_queue::SyncQueue::acquire() {
    for (uint16_t i = 0; i < this->len_; i++) {
        SyncObject obj = this->objAt(i);
        LockAcquireType acq = this->acquireTypeAt(i);
        switch (acq) {
        case LockAcquireType::Exclusive: {
            obj.vtable->lockExclusive(obj.ptr);
        } break;
        case LockAcquireType::Shared: {
            obj.vtable->lockShared(obj.ptr);
        } break;
        default:
            unreachable();
        }
    }
    this->isAcquired_ = true;
}

bool sy::sync_queue::SyncQueue::tryAcquire() {
    uint16_t i = 0;
    bool didAcquireAll = true;
    for (; i < this->len_; i++) {
        SyncObject obj = this->objAt(i);
        const LockAcquireType acq = this->acquireTypeAt(i);
        switch (acq) {
        case LockAcquireType::Exclusive: {
            if (!obj.vtable->tryLockExclusive(obj.ptr)) {
                didAcquireAll = false;
                break;
            }
        } break;
        case LockAcquireType::Shared: {
            if (!obj.vtable->tryLockShared(obj.ptr)) {
                didAcquireAll = false;
                break;
            }
        } break;
        default:
            unreachable();
        }
    }

    if (didAcquireAll) {
        this->isAcquired_ = true;
        return true;
    }

    while (i > 0) {
        i -= 1;

        SyncObject obj = this->objAt(i);
        const LockAcquireType acq = this->acquireTypeAt(i);
        switch (acq) {
        case LockAcquireType::Exclusive: {
            obj.vtable->unlockExclusive(obj.ptr);
        } break;
        case LockAcquireType::Shared: {
            obj.vtable->unlockShared(obj.ptr);
        } break;
        default:
            unreachable();
        }
    }
    this->len_ = 0; // "Clear" the currently held sync objects
    return false;
}

void sy::sync_queue::SyncQueue::release() {
    for (uint16_t i = 0; i < this->len_; i++) {
        SyncObject obj = this->objAt(i);
        const LockAcquireType acq = this->acquireTypeAt(i);
        switch (acq) {
        case LockAcquireType::Exclusive: {
            obj.vtable->unlockExclusive(obj.ptr);
        } break;
        case LockAcquireType::Shared: {
            obj.vtable->unlockShared(obj.ptr);
        } break;
        default:
            unreachable();
        }
    }
    this->isAcquired_ = false;
    this->len_ = 0;
}

Result<void, AllocErr> sy::sync_queue::SyncQueue::add(SyncObject obj, LockAcquireType type) {
    sy_assert(this->len_ < UINT16_MAX, "Cannot add any more objects to sync");
    if (this->len_ == this->capacity_) {
        if (!this->addExtraCapacity()) {
            return Error(AllocErr::OutOfMemory);
        }
    }

    const uintptr_t objPtr = reinterpret_cast<uintptr_t>(obj.ptr);
    const uintptr_t objVTable = reinterpret_cast<uintptr_t>(obj.vtable);

    if (this->len_ == 0) {
        this->objects_[0] = objPtr;
        syncQueueVTablesAndAcquireType(this->objects_, this->capacity_)[0] = objVTable | static_cast<uintptr_t>(type);
        return {};
    }

    const auto foundRes = syncQueueWhereToInsert(objPtr, this->objects_, this->len_);
    if (foundRes.hasErr()) {
        // TODO figure out what to do on duplicates with different acquire types
        return {};
    }
    const uint16_t foundIndex = foundRes.value();

    uintptr_t* vtables = syncQueueVTablesAndAcquireType(this->objects_, this->capacity_);

    uint16_t moveIter = this->len_;
    while (moveIter > foundIndex) {
        moveIter -= 1;
        this->objects_[moveIter + 1] = this->objects_[moveIter];
        vtables[moveIter + 1] = vtables[moveIter];
    }

    this->objects_[foundIndex] = objPtr;
    vtables[foundIndex] = objVTable | static_cast<uintptr_t>(type);

    return {};
}

Result<void, AllocErr> sy::sync_queue::SyncQueue::addExtraCapacity() {
    const size_t newCapacity = syncQueueGrowCapacity(this->capacity_);
    sy_assert(newCapacity <= UINT16_MAX, "Sync queue maximum capacity is max uint16");
    const size_t newAllocSize = syncQueueByteAllocSize(newCapacity);

    Allocator alloc{};
    auto res = alloc.allocAlignedArray<uint8_t>(newAllocSize, ALLOC_CACHE_ALIGN);
    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    uint8_t* mem = res.value();

    uintptr_t* newObjects = reinterpret_cast<uintptr_t*>(mem);
    uintptr_t* newVTables = syncQueueVTablesAndAcquireType(newObjects, newCapacity);

    if (this->capacity_ > 0) {
        const uintptr_t* oldVTables = syncQueueVTablesAndAcquireType(this->objects_, this->capacity_);

        for (uint16_t i = 0; i < this->len_; i++) {
            newObjects[i] = this->objects_[i];
            newVTables[i] = oldVTables[i];
        }

        alloc.freeAlignedArray(reinterpret_cast<uint8_t*>(this->objects_), syncQueueByteAllocSize(this->capacity_),
                               ALLOC_CACHE_ALIGN);
    }
    this->objects_ = newObjects;
    this->capacity_ = static_cast<uint16_t>(newCapacity);
    return {};
}

SyncObject sy::sync_queue::SyncQueue::objAt(uint16_t index) const {
    sy_assert(index < this->len_, "Index out of bounds");
    const uintptr_t* vtables = syncQueueVTablesAndAcquireType(this->objects_, this->capacity_);

    SyncObject obj;
    obj.ptr = reinterpret_cast<void*>(this->objects_[index]);
    obj.vtable = reinterpret_cast<const SyncObject::VTable*>(vtables[index] & (~(static_cast<uintptr_t>(1))));
    return obj;
}

LockAcquireType sy::sync_queue::SyncQueue::acquireTypeAt(uint16_t index) const {
    sy_assert(index < this->len_, "Index out of bounds");
    const uintptr_t* vtables = syncQueueVTablesAndAcquireType(this->objects_, this->capacity_);
    const uintptr_t flag = vtables[index] & static_cast<uintptr_t>(1);
    return static_cast<LockAcquireType>(flag);
}

sy::sync_queue::SyncQueueStack::SyncQueueStack() : current(0) {
    constexpr size_t DEFAULT_CAPACITY = ALLOC_CACHE_ALIGN / sizeof(SyncQueue);
    sy::Allocator alloc{};
    SyncQueue* newQueues = alloc.allocAlignedArray<SyncQueue>(DEFAULT_CAPACITY, ALLOC_CACHE_ALIGN).value();
    for (size_t i = 0; i < DEFAULT_CAPACITY; i++) {
        SyncQueue* placed = new (&newQueues[i]) SyncQueue;
        (void)placed;
    }
    this->queues = newQueues;
    this->capacity = DEFAULT_CAPACITY;
}

sy::sync_queue::SyncQueueStack::~SyncQueueStack() noexcept {
    if (this->queues == nullptr)
        return;

    sy::Allocator alloc{};
    for (size_t i = 0; i < this->capacity; i++) {
        this->queues[i].~SyncQueue();
    }
    alloc.freeAlignedArray(this->queues, this->capacity, ALLOC_CACHE_ALIGN);

    this->queues = nullptr;
    this->current = 0;
    this->capacity = 0;
}

sy::sync_queue::SyncQueue& sy::sync_queue::SyncQueueStack::top() {
    sy_assert(this->current < this->capacity, "Invalid memory access");
    return this->queues[this->current];
}

void sy::sync_queue::SyncQueueStack::push() {
    this->current += 1;
    sy_assert(this->current < this->capacity, "SyncQueueStack did not allocate enough queues");
}

void sy::sync_queue::SyncQueueStack::pop() {
    sy_assert(this->current > 0, "No more queues to pop");
    this->current -= 1;
}

Result<void, AllocErr> sy::sync_queue::SyncQueueStack::ensureCapacityForOneAfter() {
    if ((this->current + 1) < this->capacity)
        return {};

    const size_t newCapacity = this->capacity == 0 ? ALLOC_CACHE_ALIGN / sizeof(SyncQueue)
                                                   : static_cast<size_t>(static_cast<double>(this->capacity) * 1.5);

    sy::Allocator alloc{};
    auto res = alloc.allocAlignedArray<SyncQueue>(newCapacity, ALLOC_CACHE_ALIGN);
    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }
    SyncQueue* newQueues = res.value();
    for (size_t i = 0; i < newCapacity; i++) {
        SyncQueue* placed = new (&newQueues[i]) SyncQueue;
        (void)placed;
    }

    if (this->capacity > 0) {
        for (size_t i = 0; i < this->current; i++) {
            newQueues[i] = std::move(this->queues[i]);
        }
        alloc.freeAlignedArray(this->queues, this->capacity, ALLOC_CACHE_ALIGN);
    }
    this->queues = newQueues;
    this->capacity = newCapacity;

    return {};
}

#if SYNC_LIB_WITH_TESTS

#include "../doctest.h"
#include <shared_mutex>

using namespace sy;
using sync_queue::SyncObject;

TEST_CASE("SyncQueue binary search") {
    { // one value
        const uintptr_t values[1] = {5};
        CHECK_EQ(syncQueueWhereToInsert(0, values, 1).value(), 0);
        CHECK_EQ(syncQueueWhereToInsert(2, values, 1).value(), 0);
        CHECK_EQ(syncQueueWhereToInsert(4, values, 1).value(), 0);
        CHECK_EQ(syncQueueWhereToInsert(6, values, 1).value(), 1);
        CHECK_EQ(syncQueueWhereToInsert(7, values, 1).value(), 1);

        CHECK_EQ(syncQueueWhereToInsert(5, values, 1).err(), 0);
    }
    { // two values
        const uintptr_t values[2] = {10, 12};
        CHECK_EQ(syncQueueWhereToInsert(0, values, 2).value(), 0);
        CHECK_EQ(syncQueueWhereToInsert(2, values, 2).value(), 0);
        CHECK_EQ(syncQueueWhereToInsert(9, values, 2).value(), 0);
        CHECK_EQ(syncQueueWhereToInsert(11, values, 2).value(), 1);
        CHECK_EQ(syncQueueWhereToInsert(13, values, 2).value(), 2);
        CHECK_EQ(syncQueueWhereToInsert(16, values, 2).value(), 2);

        CHECK_EQ(syncQueueWhereToInsert(10, values, 2).err(), 0);
        CHECK_EQ(syncQueueWhereToInsert(12, values, 2).err(), 1);
    }
    { // many values
        const uintptr_t values[12] = {1, 3, 6, 8, 10, 12, 15, 20, 45, 100, 550, 10502};
        CHECK_EQ(syncQueueWhereToInsert(0, values, 12).value(), 0);
        CHECK_EQ(syncQueueWhereToInsert(2, values, 12).value(), 1);
        CHECK_EQ(syncQueueWhereToInsert(4, values, 12).value(), 2);
        CHECK_EQ(syncQueueWhereToInsert(7, values, 12).value(), 3);
        CHECK_EQ(syncQueueWhereToInsert(9, values, 12).value(), 4);
        CHECK_EQ(syncQueueWhereToInsert(11, values, 12).value(), 5);
        CHECK_EQ(syncQueueWhereToInsert(13, values, 12).value(), 6);
        CHECK_EQ(syncQueueWhereToInsert(16, values, 12).value(), 7);
        CHECK_EQ(syncQueueWhereToInsert(21, values, 12).value(), 8);
        CHECK_EQ(syncQueueWhereToInsert(50, values, 12).value(), 9);
        CHECK_EQ(syncQueueWhereToInsert(200, values, 12).value(), 10);
        CHECK_EQ(syncQueueWhereToInsert(1000, values, 12).value(), 11);
        CHECK_EQ(syncQueueWhereToInsert(10503, values, 12).value(), 12);
        CHECK_EQ(syncQueueWhereToInsert(10504, values, 12).value(), 12);

        CHECK_EQ(syncQueueWhereToInsert(1, values, 12).err(), 0);
        CHECK_EQ(syncQueueWhereToInsert(3, values, 12).err(), 1);
        CHECK_EQ(syncQueueWhereToInsert(6, values, 12).err(), 2);
        CHECK_EQ(syncQueueWhereToInsert(8, values, 12).err(), 3);
        CHECK_EQ(syncQueueWhereToInsert(10, values, 12).err(), 4);
        CHECK_EQ(syncQueueWhereToInsert(12, values, 12).err(), 5);
        CHECK_EQ(syncQueueWhereToInsert(15, values, 12).err(), 6);
        CHECK_EQ(syncQueueWhereToInsert(20, values, 12).err(), 7);
        CHECK_EQ(syncQueueWhereToInsert(45, values, 12).err(), 8);
        CHECK_EQ(syncQueueWhereToInsert(100, values, 12).err(), 9);
        CHECK_EQ(syncQueueWhereToInsert(550, values, 12).err(), 10);
        CHECK_EQ(syncQueueWhereToInsert(10502, values, 12).err(), 11);
    }
}

static const SyncObject::VTable cppRwLockVTable = {
    [](void* lock) { reinterpret_cast<std::shared_mutex*>(lock)->lock(); },
    [](void* lock) { return reinterpret_cast<std::shared_mutex*>(lock)->try_lock(); },
    [](void* lock) { reinterpret_cast<std::shared_mutex*>(lock)->unlock(); },
    [](const void* lock) {
        const_cast<std::shared_mutex*>(reinterpret_cast<const std::shared_mutex*>(lock))->lock_shared();
    },
    [](const void* lock) {
        return const_cast<std::shared_mutex*>(reinterpret_cast<const std::shared_mutex*>(lock))->try_lock_shared();
    },
    [](const void* lock) {
        const_cast<std::shared_mutex*>(reinterpret_cast<const std::shared_mutex*>(lock))->unlock_shared();
    },
};

TEST_SUITE("one lock") {
    TEST_CASE("exclusive") {
        std::shared_mutex rwlock;
        SyncObject obj{&rwlock, &cppRwLockVTable};
        sync_queue::addExclusive(std::move(obj));
        sync_queue::lock();
        sync_queue::unlock();
    }

    TEST_CASE("shared") {
        std::shared_mutex rwlock;
        SyncObject obj{&rwlock, &cppRwLockVTable};
        sync_queue::addShared(std::move(obj));
        sync_queue::lock();
        sync_queue::unlock();
    }
}

#endif // SYNC_LIB_NO_TESTS
