#include <assert.h>
#include <core_internal.h>

int main() {
    { // acquire shared -> exclusive, release exclusive -> shared
        SyRawRwLock lock{};
        const bool _r1 = (sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(_r1);
        (void)_r1;
        // re-enter
        assert(lock.readerLen == 1);
        assert(lock.exclusiveCount.value == 0);
        const bool _r2 = (sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(_r2);
        (void)_r2;
        assert(lock.readerLen == 1);
        assert(lock.exclusiveCount.value == 1);
        sy_raw_rwlock_release_exclusive(&lock);

        assert(lock.readerLen == 1);
        assert(lock.exclusiveCount.value == 0);
        sy_raw_rwlock_release_shared(&lock);
        assert(lock.exclusiveCount.value == 0);
        assert(lock.readerLen == 0);

        sy_raw_rwlock_destroy(&lock);
    }
    { // acquire shared -> exclusive, release shared -> exclusive
        SyRawRwLock lock{};
        const bool _r3 = (sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(_r3);
        (void)_r3;
        // re-enter
        assert(lock.readerLen == 1);
        assert(lock.exclusiveCount.value == 0);
        const bool _r4 = (sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(_r4);
        (void)_r4;
        assert(lock.readerLen == 1);
        assert(lock.exclusiveCount.value == 1);

        sy_raw_rwlock_release_shared(&lock);
        assert(lock.readerLen == 0);
        assert(lock.exclusiveCount.value == 1);
        sy_raw_rwlock_release_exclusive(&lock);
        assert(lock.exclusiveCount.value == 0);
        assert(lock.readerLen == 0);

        sy_raw_rwlock_destroy(&lock);
    }
    { // acquire exclusive -> shared, release exclusive -> shared
        SyRawRwLock lock{};
        const bool _r5 = (sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(_r5);
        (void)_r5;
        // re-enter
        assert(lock.readerLen == 0);
        assert(lock.exclusiveCount.value == 1);
        const bool _r6 = (sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(_r6);
        (void)_r6;
        assert(lock.readerLen == 1);
        assert(lock.exclusiveCount.value == 1);

        sy_raw_rwlock_release_exclusive(&lock);
        assert(lock.readerLen == 1);
        assert(lock.exclusiveCount.value == 0);
        sy_raw_rwlock_release_shared(&lock);
        assert(lock.exclusiveCount.value == 0);
        assert(lock.readerLen == 0);

        sy_raw_rwlock_destroy(&lock);
    }
    { // acquire exclusive -> shared, release shared -> exclusive
        SyRawRwLock lock{};
        const bool _r7 = (sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(_r7);
        (void)_r7;
        // re-enter
        assert(lock.readerLen == 0);
        assert(lock.exclusiveCount.value == 1);
        const bool _r8 = (sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(_r8);
        (void)_r8;
        assert(lock.readerLen == 1);
        assert(lock.exclusiveCount.value == 1);

        sy_raw_rwlock_release_shared(&lock);
        assert(lock.readerLen == 0);
        assert(lock.exclusiveCount.value == 1);
        sy_raw_rwlock_release_exclusive(&lock);
        assert(lock.exclusiveCount.value == 0);
        assert(lock.readerLen == 0);

        sy_raw_rwlock_destroy(&lock);
    }
    return 0;
}
