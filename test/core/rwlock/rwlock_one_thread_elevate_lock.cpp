#include <assert.h>
#include <core_internal.h>

int main() {
    { // acquire shared -> exclusive, release exclusive -> shared
        SyRawRwLock lock{};
        assert(sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
        // re-enter
        assert(lock.readerLen.value == 1);
        assert(lock.exclusiveCount.value == 0);
        assert(sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(lock.readerLen.value == 1);
        assert(lock.exclusiveCount.value == 1);
        sy_raw_rwlock_release_exclusive(&lock);

        assert(lock.readerLen.value == 1);
        assert(lock.exclusiveCount.value == 0);
        sy_raw_rwlock_release_shared(&lock);
        assert(lock.exclusiveCount.value == 0);
        assert(lock.readerLen.value == 0);

        sy_raw_rwlock_destroy(&lock);
    }
    { // acquire shared -> exclusive, release shared -> exclusive
        SyRawRwLock lock{};
        assert(sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
        // re-enter
        assert(lock.readerLen.value == 1);
        assert(lock.exclusiveCount.value == 0);
        assert(sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(lock.readerLen.value == 1);
        assert(lock.exclusiveCount.value == 1);

        sy_raw_rwlock_release_shared(&lock);
        assert(lock.readerLen.value == 0);
        assert(lock.exclusiveCount.value == 1);
        sy_raw_rwlock_release_exclusive(&lock);
        assert(lock.exclusiveCount.value == 0);
        assert(lock.readerLen.value == 0);

        sy_raw_rwlock_destroy(&lock);
    }
    { // acquire exclusive -> shared, release exclusive -> shared
        SyRawRwLock lock{};
        assert(sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
        // re-enter
        assert(lock.readerLen.value == 0);
        assert(lock.exclusiveCount.value == 1);
        assert(sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(lock.readerLen.value == 1);
        assert(lock.exclusiveCount.value == 1);

        sy_raw_rwlock_release_exclusive(&lock);
        assert(lock.readerLen.value == 1);
        assert(lock.exclusiveCount.value == 0);
        sy_raw_rwlock_release_shared(&lock);
        assert(lock.exclusiveCount.value == 0);
        assert(lock.readerLen.value == 0);

        sy_raw_rwlock_destroy(&lock);
    }
    { // acquire exclusive -> shared, release shared -> exclusive
        SyRawRwLock lock{};
        assert(sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
        // re-enter
        assert(lock.readerLen.value == 0);
        assert(lock.exclusiveCount.value == 1);
        assert(sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(lock.readerLen.value == 1);
        assert(lock.exclusiveCount.value == 1);

        sy_raw_rwlock_release_shared(&lock);
        assert(lock.readerLen.value == 0);
        assert(lock.exclusiveCount.value == 1);
        sy_raw_rwlock_release_exclusive(&lock);
        assert(lock.exclusiveCount.value == 0);
        assert(lock.readerLen.value == 0);

        sy_raw_rwlock_destroy(&lock);
    }
}