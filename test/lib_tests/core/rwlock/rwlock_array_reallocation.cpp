#include <assert.h>
#include <core_internal.h>

int main() {
    SyRawRwLock lock{};

    int initialCapacity = lock.readerCapacity;
    assert(initialCapacity == 0);

    assert(sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(lock.readerCapacity >= 1);
    int capacity1 = lock.readerCapacity;

    for (int i = 1; i < capacity1; i++) {
        assert(sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
        assert(lock.readerCapacity == capacity1);
    }

    assert(sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
    int capacity2 = lock.readerCapacity;
    assert(capacity2 > capacity1);
    assert(capacity2 == capacity1 * 2);

    for (int i = 0; i < capacity1 + 1; i++) {
        sy_raw_rwlock_release_shared(&lock);
    }

    assert(lock.readerLen == 0);

    sy_raw_rwlock_destroy(&lock);
    return 0;
}
