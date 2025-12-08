#include <assert.h>
#include <atomic>
#include <core_internal.h>
#include <thread>

// They should be able to re-try

std::atomic<int> iteration{0};
std::atomic<int> deadlockCountIteration1{0};
std::atomic<int> deadlockCountIteration2{0};

void thread1Fn(SyRawRwLock* lock) {
    assert(sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);

    iteration.fetch_add(1, std::memory_order_seq_cst);
    while (iteration.load(std::memory_order_seq_cst) < 2) {
        std::this_thread::yield();
    }

    assert(sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_DEADLOCK);
    deadlockCountIteration1.fetch_add(1, std::memory_order_seq_cst);
    sy_raw_rwlock_release_shared(lock);

    // wait for both
    iteration.fetch_add(1, std::memory_order_seq_cst);
    while (iteration.load(std::memory_order_seq_cst) < 4) {
        std::this_thread::yield();
    }

    // again
    assert(sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);

    iteration.fetch_add(1, std::memory_order_seq_cst);
    while (iteration.load(std::memory_order_seq_cst) < 6) {
        std::this_thread::yield();
    }

    assert(sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_DEADLOCK);
    deadlockCountIteration2.fetch_add(1, std::memory_order_seq_cst);
    sy_raw_rwlock_release_shared(lock);
}

void thread2Fn(SyRawRwLock* lock) {
    assert(sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);

    iteration.fetch_add(1, std::memory_order_seq_cst);
    while (iteration.load(std::memory_order_seq_cst) < 2) {
        std::this_thread::yield();
    }

    assert(sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_DEADLOCK);
    deadlockCountIteration1.fetch_add(1, std::memory_order_seq_cst);
    sy_raw_rwlock_release_shared(lock);

    // wait for both
    iteration.fetch_add(1, std::memory_order_seq_cst);
    while (iteration.load(std::memory_order_seq_cst) < 4) {
        std::this_thread::yield();
    }

    // repeat
    assert(sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);

    iteration.fetch_add(1, std::memory_order_seq_cst);
    while (iteration.load(std::memory_order_seq_cst) < 6) {
        std::this_thread::yield();
    }

    assert(sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_DEADLOCK);
    deadlockCountIteration2.fetch_add(1, std::memory_order_seq_cst);
    sy_raw_rwlock_release_shared(lock);
}

int main() {
    SyRawRwLock lock{};

    std::thread t1(thread1Fn, &lock);
    std::thread t2(thread2Fn, &lock);

    t1.join();
    t2.join();

    assert(deadlockCountIteration1.load() == 2);
    assert(deadlockCountIteration2.load() == 2);

    // at least two deadlocks detected
    assert(lock.deadlockGeneration.value >= 2);

    // clean state
    assert(lock.readerLen == 0);
    assert(lock.threadsWantElevateLen == 0);
    assert(lock.exclusiveId.value == 0);
    assert(lock.exclusiveCount.value == 0);

    sy_raw_rwlock_destroy(&lock);
    return 0;
}
