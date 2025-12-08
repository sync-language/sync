#include <assert.h>
#include <atomic>
#include <core_internal.h>
#include <thread>

// All threads should deadlock simultaneously.

std::atomic<int> readyCount{0};
std::atomic<int> deadlockCount{0};

void threadFn(SyRawRwLock* lock) {
    assert(sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);

    // signal ready and wait for all threads to be ready
    readyCount.fetch_add(1, std::memory_order_seq_cst);
    while (readyCount.load(std::memory_order_seq_cst) < 3) {
        std::this_thread::yield();
    }

    // Small yield to ensure all threads have registered as readers
    // std::this_thread::yield();

    assert(sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_DEADLOCK);

    deadlockCount.fetch_add(1, std::memory_order_seq_cst);

    sy_raw_rwlock_release_shared(lock);
}

int main() {
    SyRawRwLock lock{};

    std::thread t1(threadFn, &lock);
    std::thread t2(threadFn, &lock);
    std::thread t3(threadFn, &lock);

    t1.join();
    t2.join();
    t3.join();

    assert(deadlockCount.load() == 3);

    // lock should be clean
    assert(lock.readerLen == 0);
    assert(lock.threadsWantElevateLen == 0);
    assert(lock.exclusiveId.value == 0);
    // also one thread should have incremented the deadlock generation
    assert(lock.deadlockGeneration.value == 1);

    sy_raw_rwlock_destroy(&lock);
    return 0;
}
