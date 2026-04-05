#include <assert.h>
#include <core_internal.h>

int main() {
    SyRawRwLock lock{};
    const bool _r1 = (sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(_r1);
    (void)_r1;
    assert(lock.readerLen == 1);
    // re-enter
    const bool _r2 = (sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(_r2);
    (void)_r2;
    assert(lock.readerLen == 2);
    sy_raw_rwlock_release_shared(&lock);
    assert(lock.readerLen == 1);
    sy_raw_rwlock_release_shared(&lock);
    assert(lock.readerLen == 0);

    const bool _r3 = (sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(_r3);
    (void)_r3;
    assert(lock.readerLen == 1);
    sy_raw_rwlock_release_shared(&lock);
    assert(lock.readerLen == 0);

    sy_raw_rwlock_destroy(&lock);
    return 0;
}
