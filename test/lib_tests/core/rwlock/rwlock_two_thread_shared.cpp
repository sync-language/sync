#include <assert.h>
#include <core_internal.h>
#include <thread>

void threadFn(SyRawRwLock* lock) {
    for (int i = 0; i < 10000; i++) {
        const bool _r1 = (sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);
        assert(_r1);
        (void)_r1;
        sy_raw_rwlock_release_shared(lock);
    }
}

int main() {
    SyRawRwLock lock{};
    std::thread t1(threadFn, &lock);
    std::thread t2(threadFn, &lock);

    t1.join();
    t2.join();
    sy_raw_rwlock_destroy(&lock);
    return 0;
}