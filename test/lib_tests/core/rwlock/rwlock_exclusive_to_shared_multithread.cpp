#include <assert.h>
#include <atomic>
#include <core_internal.h>
#include <thread>

std::atomic<bool> thread1Downgraded{false};

void thread1Fn(SyRawRwLock* lock) {
    assert(sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_NONE);
    assert(lock->exclusiveCount.value == 1);

    assert(sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);
    assert(lock->readerLen == 1);
    assert(lock->exclusiveCount.value == 1);

    sy_raw_rwlock_release_exclusive(lock);
    assert(lock->readerLen == 1);
    assert(lock->exclusiveCount.value == 0);
    assert(lock->exclusiveId.value == 0);

    thread1Downgraded.store(true, std::memory_order_seq_cst);

    std::this_thread::yield();

    sy_raw_rwlock_release_shared(lock);
}

void thread2Fn(SyRawRwLock* lock) {
    while (!thread1Downgraded.load(std::memory_order_seq_cst)) {
        std::this_thread::yield();
    }

    assert(sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);
    sy_raw_rwlock_release_shared(lock);
}

int main() {
    SyRawRwLock lock{};

    std::thread t1(thread1Fn, &lock);
    std::thread t2(thread2Fn, &lock);

    t1.join();
    t2.join();

    assert(lock.readerLen == 0);
    assert(lock.exclusiveId.value == 0);

    sy_raw_rwlock_destroy(&lock);
    return 0;
}
