#include <assert.h>
#include <core_internal.h>

int main() {
    SyRawRwLock lock{};

    assert(lock.readerCapacity == 0);

    const bool _r1 = (sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(_r1);
    (void)_r1;
    assert(lock.readerCapacity >= 1);
    int capacity1 = lock.readerCapacity;

    for (int i = 1; i < capacity1; i++) {
        const bool _r2 = (sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(_r2);
        (void)_r2;
        assert(lock.readerCapacity == capacity1);
    }

    const bool _r3 = (sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(_r3);
    (void)_r3;
    assert(lock.readerCapacity > capacity1);
    assert(lock.readerCapacity == capacity1 * 2);

    for (int i = 0; i < capacity1 + 1; i++) {
        sy_raw_rwlock_release_shared(&lock);
    }

    assert(lock.readerLen == 0);

    sy_raw_rwlock_destroy(&lock);
    return 0;
}
