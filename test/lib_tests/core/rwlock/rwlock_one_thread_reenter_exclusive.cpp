#include <assert.h>
#include <core_internal.h>

int main() {
    SyRawRwLock lock{};
    assert(sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
    // re-enter
    assert(lock.exclusiveCount.value == 1);
    assert(sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(lock.exclusiveCount.value == 2);
    sy_raw_rwlock_release_exclusive(&lock);
    assert(lock.exclusiveCount.value == 1);
    sy_raw_rwlock_release_exclusive(&lock);
    assert(lock.exclusiveCount.value == 0);

    assert(sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(lock.exclusiveCount.value == 1);
    sy_raw_rwlock_release_exclusive(&lock);
    assert(lock.exclusiveCount.value == 0);

    sy_raw_rwlock_destroy(&lock);
    return 0;
}