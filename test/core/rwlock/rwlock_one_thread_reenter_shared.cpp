#include <assert.h>
#include <core_internal.h>

int main() {
    SyRawRwLock lock{};
    assert(sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(lock.readerLen == 1);
    // re-enter
    assert(sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(lock.readerLen == 2);
    sy_raw_rwlock_release_shared(&lock);
    assert(lock.readerLen == 1);
    sy_raw_rwlock_release_shared(&lock);
    assert(lock.readerLen == 0);

    assert(sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(lock.readerLen == 1);
    sy_raw_rwlock_release_shared(&lock);
    assert(lock.readerLen == 0);

    sy_raw_rwlock_destroy(&lock);
    return 0;
}