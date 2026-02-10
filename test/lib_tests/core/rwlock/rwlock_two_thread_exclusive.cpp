#include <assert.h>
#include <core_internal.h>
#include <thread>

void threadFn(SyRawRwLock* lock, int* counter) {
    for (int i = 0; i < 10000; i++) {
        assert(sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_NONE);
        *counter += 1;
        sy_raw_rwlock_release_exclusive(lock);
    }
}

int main() {
    SyRawRwLock lock{};
    int counter = 0;
    std::thread t1(threadFn, &lock, &counter);
    std::thread t2(threadFn, &lock, &counter);

    t1.join();
    t2.join();
    assert(counter == 20000);
    sy_raw_rwlock_destroy(&lock);
    return 0;
}