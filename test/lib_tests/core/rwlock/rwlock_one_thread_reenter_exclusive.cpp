#include <assert.h>
#include <core_internal.h>

int main() {
    SyRawRwLock lock{};
    const bool _r1 = (sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(_r1);
    (void)_r1;
    // re-enter
    assert(lock.exclusiveCount.value == 1);
    const bool _r2 = (sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(_r2);
    (void)_r2;
    assert(lock.exclusiveCount.value == 2);
    sy_raw_rwlock_release_exclusive(&lock);
    assert(lock.exclusiveCount.value == 1);
    sy_raw_rwlock_release_exclusive(&lock);
    assert(lock.exclusiveCount.value == 0);

    const bool _r3 = (sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(_r3);
    (void)_r3;
    assert(lock.exclusiveCount.value == 1);
    sy_raw_rwlock_release_exclusive(&lock);
    assert(lock.exclusiveCount.value == 0);

    sy_raw_rwlock_destroy(&lock);
    return 0;
}
