#include <assert.h>
#include <core_internal.h>

int main() {
    SyRawRwLock lock{};

    for (int i = 1; i <= 10; i++) {
        const bool _r1 = (sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(_r1);
        (void)_r1;
        assert(lock.readerLen == i);
    }

    for (int i = 10; i >= 1; i--) {
        assert(lock.readerLen == i);
        sy_raw_rwlock_release_shared(&lock);
    }

    assert(lock.readerLen == 0);

    for (int i = 1; i <= 10; i++) {
        const bool _r2 = (sy_raw_rwlock_acquire_exclusive(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(_r2);
        (void)_r2;
        assert(lock.exclusiveCount.value == (size_t)i);
    }

    for (int i = 10; i >= 1; i--) {
        assert(lock.exclusiveCount.value == (size_t)i);
        sy_raw_rwlock_release_exclusive(&lock);
    }

    assert(lock.exclusiveCount.value == 0);
    assert(lock.exclusiveId.value == 0);

    sy_raw_rwlock_destroy(&lock);
    return 0;
}
