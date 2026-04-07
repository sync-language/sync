#include "rwlock.h"
#include "../../core/core_internal.h"
#include "locks_internal.hpp"
#include "rwlock.hpp"
#include <cstring>
#include <thread>
#include <utility>

using namespace sy;

static_assert(alignof(internal::ThreadIdStore) == alignof(void*));
static_assert(sizeof(internal::ThreadIdStore) == 16);
static_assert(alignof(RwLock) == alignof(uint64_t));
static_assert(sizeof(RwLock) == 24);
static_assert(alignof(SyRwLock) == alignof(sy::RwLock));
static_assert(sizeof(SyRwLock) == sizeof(sy::RwLock));

bool internal::ThreadIdStore::add(uint32_t threadId) noexcept {
    const uint32_t currentLen = this->len;

    if (currentLen < SMALL_SIZE) {
        this->buf[currentLen] = threadId;
        this->len += 1;
        return true;
    }

    uint32_t** bufStorage = reinterpret_cast<uint32_t**>(&this->buf[1]);

    if (currentLen > SMALL_SIZE) {
        const uint32_t currentCapacity = this->buf[0];
        if (currentLen == currentCapacity) {
            sy_assert_release(currentCapacity <= ((UINT32_MAX / 2) - 1),
                              "[addThisThreadToReaders] reached max value for "
                              "reader capacity (how?)");

            const uint32_t newCapacity = currentCapacity * 2;
            uint32_t* newBuf = (uint32_t*)sy_aligned_malloc(
                static_cast<size_t>(newCapacity) * sizeof(uint32_t), alignof(void*));
            if (newBuf == nullptr) {
                return false;
            }

            uint32_t* oldBuf = *bufStorage;
            for (uint32_t i = 0; i < currentLen; i++) {
                newBuf[i] = oldBuf[i];
            }

            sy_aligned_free(reinterpret_cast<void*>(oldBuf), currentCapacity * sizeof(uint32_t),
                            alignof(void*));
            this->buf[0] = newCapacity;
            *bufStorage = newBuf;
        }

    } else {
        const uint32_t newCapacity = 8; // reasonable default
        uint32_t* newBuf = (uint32_t*)sy_aligned_malloc(
            static_cast<size_t>(newCapacity) * sizeof(uint32_t), alignof(void*));
        if (newBuf == nullptr) {
            return false;
        }

        newBuf[0] = this->buf[0];
        newBuf[1] = this->buf[1];
        newBuf[2] = this->buf[2];

        this->buf[0] = newCapacity;
        *bufStorage = newBuf;
    }

    (*bufStorage)[currentLen] = threadId;
    this->len += 1;
    return true;
}

void internal::ThreadIdStore::removeFirstInstance(uint32_t threadId) noexcept {
    const uint32_t currentLen = this->len;

    uint32_t* bufStorage = nullptr;

    if (currentLen <= SMALL_SIZE) {
        bufStorage = this->buf;
    } else {
        memcpy(reinterpret_cast<void*>(&bufStorage), &this->buf[1], sizeof(uint32_t*));
    }

    bool found = false;
    uint32_t foundIndex = 0;
    for (uint32_t i = 0; i < currentLen; i++) {
        if (bufStorage[i] == threadId) {
            found = true;
            foundIndex = i;
            break;
        }
    }

    if (!found) {
        return;
    }

    for (uint32_t i = foundIndex; i < (currentLen - 1); i++) {
        bufStorage[i] = bufStorage[i + 1];
    }

    this->len -= 1;

    if (this->len == SMALL_SIZE) { // was heap allocated, make inline stored
        const uint32_t currentCapacity = this->buf[0];

        this->buf[0] = bufStorage[0];
        this->buf[1] = bufStorage[1];
        this->buf[2] = bufStorage[2];
        sy_aligned_free(reinterpret_cast<void*>(bufStorage), currentCapacity * sizeof(uint32_t),
                        alignof(void*));
    }
}

bool internal::ThreadIdStore::contains(uint32_t threadId) const noexcept {
    const uint32_t currentLen = this->len;

    const uint32_t* bufStorage = nullptr;

    if (currentLen <= SMALL_SIZE) {
        bufStorage = this->buf;
    } else {
        memcpy(reinterpret_cast<void*>(&bufStorage), &this->buf[1], sizeof(uint32_t*));
    }

    for (uint32_t i = 0; i < currentLen; i++) {
        if (bufStorage[i] == threadId) {
            return true;
        }
    }
    return false;
}

bool internal::ThreadIdStore::isOnlyEntry(uint32_t threadId) const noexcept {
    const uint32_t currentLen = this->len;
    if (currentLen == 0) {
        return false;
    }

    const uint32_t* bufStorage = nullptr;

    if (currentLen <= SMALL_SIZE) {
        bufStorage = this->buf;
    } else {
        memcpy(reinterpret_cast<void*>(&bufStorage), &this->buf[1], sizeof(uint32_t*));
    }

    for (uint32_t i = 0; i < currentLen; i++) {
        if (bufStorage[i] != threadId) {
            return false;
        }
    }
    return true;
}

internal::ThreadIdStore::~ThreadIdStore() noexcept {
    if (this->len > SMALL_SIZE) {
        const uint32_t capacity = this->buf[0];
        uint32_t* bufStorage;
        memcpy(reinterpret_cast<void*>(&bufStorage), &this->buf[1], sizeof(uint32_t*));
        sy_aligned_free(reinterpret_cast<void*>(bufStorage), capacity * sizeof(uint32_t),
                        alignof(uint32_t));

        this->len = 0;
    }
}

internal::ThreadIdStore::ThreadIdStore(ThreadIdStore&& other) noexcept : len(other.len) {
    this->buf[0] = other.buf[0];
    this->buf[1] = other.buf[1];
    this->buf[2] = other.buf[2];

    other.len = 0;
    other.buf[0] = 0;
    other.buf[1] = 0;
    other.buf[2] = 0;
}

RwLock::~RwLock() noexcept {
    internal::acquireAtomicFence(this->fence_);

    const uint32_t currentExclusiveId = this->exclusiveId_.load(std::memory_order_seq_cst);
    const uint32_t currentReadersLen = this->readers_.len;
    sy_assert_release(currentExclusiveId == 0, "[sy::RwLock::~RwLock] cannot destroy rwlock when a "
                                               "thread has exclusive access");
    sy_assert_release(currentReadersLen == 0,
                      "[sy::RwLock::~RwLock] cannot destroy rwlock that was "
                      "locked by another thread");

    {
        auto moveReaders = std::move(this->readers_);
        // explicitly move them to invoke the destructors prior to atomic fence cleanup
        (void)moveReaders;
    }

    internal::releaseAtomicFence(this->fence_);
    internal::tsan_mutex_destroy(this->fence_);
}

Result<void, RwLock::AcquireErr> RwLock::lockShared() noexcept {
    while (true) {
        auto res = this->tryLockShared();
        if (res.hasValue()) {
            return res;
        }

        if (res.hasErr()) {
            if (res.err() == AcquireErr::OutOfMemory) {
                return res;
            }
        }
        std::this_thread::yield();
    }
}

Result<void, RwLock::AcquireErr> RwLock::tryLockShared() noexcept {
    const uint32_t threadId = internal::getThisThreadId();

    { // Quick check. Don't wanna go through all the steps if someone has an exclusive lock.
        const uint32_t currentExclusiveId = this->exclusiveId_.load(std::memory_order_seq_cst);
        if (currentExclusiveId != threadId && currentExclusiveId != 0) {
            return Error(AcquireErr::HasExclusive);
        }
    }

    internal::acquireAtomicFence(this->fence_);

    { // check again in case someone else acquired in the meantime
        const uint32_t currentExclusiveId = this->exclusiveId_.load(std::memory_order_seq_cst);
        if (currentExclusiveId != threadId && currentExclusiveId != 0) {
            internal::releaseAtomicFence(this->fence_);
            return Error(AcquireErr::HasExclusive);
        }
    }

    if (this->readers_.add(threadId) == false) {
        internal::releaseAtomicFence(this->fence_);
        return Error(AcquireErr::OutOfMemory);
    }
    internal::releaseAtomicFence(this->fence_);
    return {};
}

void sy::RwLock::unlockShared() noexcept {
    const uint32_t threadId = internal::getThisThreadId();

    internal::acquireAtomicFence(this->fence_);

    const uint32_t currentExclusiveId = this->exclusiveId_.load(std::memory_order_seq_cst);
    sy_assert(currentExclusiveId == threadId || currentExclusiveId == 0,
              "[sy::RwLock::unlockShared] cannot release shared lock when another thread "
              "has an exclusive lock");
    sy_assert(this->readers_.len != 0, "[sy::RwLock::unlockShared] cannot release shared "
                                       "lock if no thread has a shared lock");
    sy_assert(
        this->readers_.contains(threadId),
        "[sy::RwLock::unlockShared] cannot release shared lock that wasn't locked by this thread");
    (void)currentExclusiveId;

    this->readers_.removeFirstInstance(threadId);
    internal::releaseAtomicFence(this->fence_);
}

void sy::RwLock::lockSharedUnchecked() noexcept {
    auto res = this->lockShared();
    if (res.hasErr()) {
        switch (res.err()) {
        case AcquireErr::OutOfMemory: {
            syncFatalErrorHandlerFn(
                "[sy::RwLock::lockSharedUnchecked] shared lock failed due to out of memory");
        } break;
        default:
            break;
        }
    }
}

Result<void, RwLock::AcquireErr> RwLock::lockExclusive() noexcept {
    while (true) {
        auto res = this->tryLockExclusive();
        if (res.hasValue()) {
            return res;
        }

        if (res.hasErr()) {
            switch (res.err()) {
            case AcquireErr::OutOfMemory:
            case AcquireErr::Deadlock: {
                return res;
            } break;
            default:
                break;
            }
        }
        std::this_thread::yield();
    }
}

Result<void, RwLock::AcquireErr> RwLock::tryLockExclusive() noexcept {
    const uint32_t threadId = internal::getThisThreadId();

    { // Quick check. Don't wanna go through all the steps if someone has an exclusive lock.
        const uint32_t currentExclusiveId = this->exclusiveId_.load(std::memory_order_seq_cst);
        if (currentExclusiveId == threadId) {
            const uint16_t previousExclusiveCount =
                this->exclusiveReentrantCount_.fetch_add(1, std::memory_order_seq_cst);
            sy_assert(previousExclusiveCount < (UINT16_MAX - 1),
                      "[sy::RwLock::tryLockExclusive] re-entered rwlock too many times");
            (void)previousExclusiveCount;
            return {};
        }
        if (currentExclusiveId != 0) {
            return Error(AcquireErr::HasExclusive);
        }
    }

    internal::acquireAtomicFence(this->fence_);

    const bool thisThreadIsReader = this->readers_.contains(threadId);

    if (this->readers_.len > 0 && !this->readers_.isOnlyEntry(threadId)) {
        internal::releaseAtomicFence(this->fence_);
        if (thisThreadIsReader) {
            // cannot elevate
            return Error(AcquireErr::Deadlock);
        }
        return Error(AcquireErr::HasReaders);
    }

    { // check again in case someone else acquired in the meantime
        const uint32_t currentExclusiveId = this->exclusiveId_.load(std::memory_order_seq_cst);
        if (currentExclusiveId != threadId && currentExclusiveId != 0) {
            internal::releaseAtomicFence(this->fence_);
            return Error(AcquireErr::HasExclusive);
        }
    }

    // DO NOT remove from readers if it's a reader to maintain re-entrant functionality on both
    this->exclusiveId_.store(threadId, std::memory_order_seq_cst);
    const uint16_t previousExclusiveCount =
        this->exclusiveReentrantCount_.fetch_add(1, std::memory_order_seq_cst);
    sy_assert(previousExclusiveCount < (UINT16_MAX - 1),
              "[sy::RwLock::tryLockExclusive] re-entered rwlock too many times");
    (void)previousExclusiveCount;
    internal::releaseAtomicFence(this->fence_);
    return {};
}

void sy::RwLock::unlockExclusive() noexcept {

    const uint32_t threadId = internal::getThisThreadId();

    internal::acquireAtomicFence(this->fence_);

    const uint32_t currentExclusiveId = this->exclusiveId_.load(std::memory_order_seq_cst);
    sy_assert(
        currentExclusiveId != 0,
        "[sy::RwLock::unlockExclusive] cannot release exclusive lock when not thread has acquired");
    sy_assert(currentExclusiveId == threadId, "[sy::RwLock::unlockExclusive] cannot release "
                                              "exclusive lock that was locked by another thread");
    (void)threadId;
    (void)currentExclusiveId;

    const uint16_t currentExclusiveCount =
        this->exclusiveReentrantCount_.fetch_sub(1, std::memory_order_seq_cst);
    if (currentExclusiveCount == 1) {
        this->exclusiveId_.store(0, std::memory_order_seq_cst);
    }

    internal::releaseAtomicFence(this->fence_);
}

void sy::RwLock::lockExclusiveUnchecked() noexcept {
    auto res = this->lockExclusive();
    if (res.hasErr()) {
        switch (res.err()) {
        case AcquireErr::OutOfMemory: {
            syncFatalErrorHandlerFn(
                "[sy::RwLock::lockExclusiveUnchecked] exclusive lock failed due to out of memory");
        } break;
        case AcquireErr::Deadlock: {
            syncFatalErrorHandlerFn(
                "[sy::RwLock::lockExclusiveUnchecked] exclusive lock failed due to deadlocking");
        } break;
        default:
            break;
        }
    }
}

extern "C" {
SY_API void sy_rwlock_destroy(SyRwLock* self) {
    RwLock* impl = reinterpret_cast<RwLock*>(self);
    impl->~RwLock();
}

SY_API SyAcquireErr sy_rwlock_lock_shared(SyRwLock* self) {
    RwLock* impl = reinterpret_cast<RwLock*>(self);
    auto res = impl->lockShared();
    if (res.hasValue()) {
        return SY_ACQUIRE_ERR_NONE;
    } else {
        switch (res.err()) {
        case RwLock::AcquireErr::OutOfMemory: {
            return SY_ACQUIRE_ERR_OUT_OF_MEMORY;
        } break;
        default:
            sync_unreachable();
        }
    }
}

SY_API SyAcquireErr sy_rwlock_try_lock_shared(SyRwLock* self) {
    RwLock* impl = reinterpret_cast<RwLock*>(self);
    auto res = impl->tryLockShared();
    if (res.hasValue()) {
        return SY_ACQUIRE_ERR_NONE;
    } else {
        switch (res.err()) {
        case RwLock::AcquireErr::OutOfMemory: {
            return SY_ACQUIRE_ERR_OUT_OF_MEMORY;
        } break;
        case RwLock::AcquireErr::HasExclusive: {
            return SY_ACQUIRE_ERR_HAS_EXCLUSIVE;
        } break;
        default:
            sync_unreachable();
        }
    }
}

SY_API void sy_rwlock_unlock_shared(SyRwLock* self) {
    RwLock* impl = reinterpret_cast<RwLock*>(self);
    impl->unlockShared();
}

SY_API void sy_rwlock_lock_shared_unchecked(SyRwLock* self) {
    RwLock* impl = reinterpret_cast<RwLock*>(self);
    impl->lockSharedUnchecked();
}

SY_API SyAcquireErr sy_rwlock_lock_exclusive(SyRwLock* self) {
    RwLock* impl = reinterpret_cast<RwLock*>(self);
    auto res = impl->lockExclusive();
    if (res.hasValue()) {
        return SY_ACQUIRE_ERR_NONE;
    } else {
        switch (res.err()) {
        case RwLock::AcquireErr::OutOfMemory: {
            return SY_ACQUIRE_ERR_OUT_OF_MEMORY;
        } break;
        case RwLock::AcquireErr::Deadlock: {
            return SY_ACQUIRE_ERR_DEADLOCK;
        } break;
        default:
            sync_unreachable();
        }
    }
}

SY_API SyAcquireErr sy_rwlock_try_lock_exclusive(SyRwLock* self) {
    RwLock* impl = reinterpret_cast<RwLock*>(self);
    auto res = impl->tryLockExclusive();
    if (res.hasValue()) {
        return SY_ACQUIRE_ERR_NONE;
    } else {
        switch (res.err()) {
        case RwLock::AcquireErr::OutOfMemory: {
            return SY_ACQUIRE_ERR_OUT_OF_MEMORY;
        } break;
        case RwLock::AcquireErr::HasExclusive: {
            return SY_ACQUIRE_ERR_HAS_EXCLUSIVE;
        } break;
        case RwLock::AcquireErr::HasReaders: {
            return SY_ACQUIRE_ERR_HAS_READERS;
        } break;
        case RwLock::AcquireErr::Deadlock: {
            return SY_ACQUIRE_ERR_DEADLOCK;
        } break;
        default:
            sync_unreachable();
        }
    }
}

SY_API void sy_rwlock_unlock_exclusive(SyRwLock* self) {
    RwLock* impl = reinterpret_cast<RwLock*>(self);
    impl->unlockExclusive();
}

SY_API void sy_rwlock_lock_exclusive_unchecked(SyRwLock* self) {
    RwLock* impl = reinterpret_cast<RwLock*>(self);
    impl->lockExclusiveUnchecked();
}
} // extern "C"
