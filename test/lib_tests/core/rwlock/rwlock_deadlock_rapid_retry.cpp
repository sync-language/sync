#include <assert.h>
#include <atomic>
#include <core_internal.h>
#include <thread>

// They should be able to re-try

std::atomic<int> iteration{0};
std::atomic<int> deadlockCountIteration1{0};
std::atomic<int> deadlockCountIteration2{0};

void thread1Fn(SyRawRwLock* lock) {
    const bool _r1 = (sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);
    assert(_r1);
    (void)_r1;

    iteration.fetch_add(1, std::memory_order_seq_cst);
    while (iteration.load(std::memory_order_seq_cst) < 2) {
        std::this_thread::yield();
    }

    const bool _r2 = (sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_DEADLOCK);
    assert(_r2);
    (void)_r2;
    deadlockCountIteration1.fetch_add(1, std::memory_order_seq_cst);
    sy_raw_rwlock_release_shared(lock);

    // wait for both
    iteration.fetch_add(1, std::memory_order_seq_cst);
    while (iteration.load(std::memory_order_seq_cst) < 4) {
        std::this_thread::yield();
    }

    // again
    const bool _r3 = (sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);
    assert(_r3);
    (void)_r3;

    iteration.fetch_add(1, std::memory_order_seq_cst);
    while (iteration.load(std::memory_order_seq_cst) < 6) {
        std::this_thread::yield();
    }

    const bool _r4 = (sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_DEADLOCK);
    assert(_r4);
    (void)_r4;
    deadlockCountIteration2.fetch_add(1, std::memory_order_seq_cst);
    sy_raw_rwlock_release_shared(lock);
}

void thread2Fn(SyRawRwLock* lock) {
    const bool _r1 = (sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);
    assert(_r1);
    (void)_r1;

    iteration.fetch_add(1, std::memory_order_seq_cst);
    while (iteration.load(std::memory_order_seq_cst) < 2) {
        std::this_thread::yield();
    }

    const bool _r2 = (sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_DEADLOCK);
    assert(_r2);
    (void)_r2;
    deadlockCountIteration1.fetch_add(1, std::memory_order_seq_cst);
    sy_raw_rwlock_release_shared(lock);

    // wait for both
    iteration.fetch_add(1, std::memory_order_seq_cst);
    while (iteration.load(std::memory_order_seq_cst) < 4) {
        std::this_thread::yield();
    }

    // repeat
    const bool _r3 = (sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);
    assert(_r3);
    (void)_r3;

    iteration.fetch_add(1, std::memory_order_seq_cst);
    while (iteration.load(std::memory_order_seq_cst) < 6) {
        std::this_thread::yield();
    }

    const bool _r4 = (sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_DEADLOCK);
    assert(_r4);
    (void)_r4;
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
