#include <assert.h>
#include <atomic>
#include <core_internal.h>
#include <thread>

std::atomic<bool> thread1HasExclusive{false};
std::atomic<bool> thread2CanProceed{false};

void thread1Fn(SyRawRwLock* lock) {
    assert(sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_NONE);
    thread1HasExclusive.store(true, std::memory_order_seq_cst);

    while (!thread2CanProceed.load(std::memory_order_seq_cst)) {
        std::this_thread::yield();
    }

    sy_raw_rwlock_release_exclusive(lock);
}

void thread2Fn(SyRawRwLock* lock) {
    while (!thread1HasExclusive.load(std::memory_order_seq_cst)) {
        std::this_thread::yield();
    }

    assert(sy_raw_rwlock_try_acquire_shared(lock) == SY_ACQUIRE_ERR_SHARED_HAS_EXCLUSIVE);
    assert(sy_raw_rwlock_try_acquire_exclusive(lock) == SY_ACQUIRE_ERR_EXCLUSIVE_HAS_EXCLUSIVE);

    thread2CanProceed.store(true, std::memory_order_seq_cst);

    assert(sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);
    sy_raw_rwlock_release_shared(lock);
}

int main() {
    SyRawRwLock lock{};

    std::thread t1(thread1Fn, &lock);
    std::thread t2(thread2Fn, &lock);

    t1.join();
    t2.join();

    sy_raw_rwlock_destroy(&lock);
    return 0;
}
