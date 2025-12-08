#include <assert.h>
#include <atomic>
#include <core_internal.h>
#include <thread>

// Generation counter shouldn't impact future lock acquiring.

std::atomic<int> phase{0};
std::atomic<int> deadlockCount{0};

void earlyThread(SyRawRwLock* lock) {
    assert(sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);

    phase.fetch_add(1, std::memory_order_seq_cst);
    while (phase.load(std::memory_order_seq_cst) < 2) {
        std::this_thread::yield();
    }

    assert(sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_DEADLOCK);
    deadlockCount.fetch_add(1, std::memory_order_seq_cst);

    sy_raw_rwlock_release_shared(lock);

    phase.fetch_add(1, std::memory_order_seq_cst);
}

void lateThread(SyRawRwLock* lock) {
    // wait for the others to finish their deadlock
    while (phase.load(std::memory_order_seq_cst) < 4) {
        std::this_thread::yield();
    }

    // they both deadlocked
    assert(deadlockCount.load() == 2);

    assert(sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);
    assert(sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_NONE);

    assert(lock->exclusiveId.value != 0);
    assert(lock->exclusiveCount.value == 1);

    sy_raw_rwlock_release_exclusive(lock);
    sy_raw_rwlock_release_shared(lock);
}

int main() {
    SyRawRwLock lock{};

    std::thread t1(earlyThread, &lock);
    std::thread t2(earlyThread, &lock);
    std::thread t3(lateThread, &lock);

    t1.join();
    t2.join();
    t3.join();

    assert(lock.readerLen == 0);
    assert(lock.threadsWantElevateLen == 0);
    assert(lock.exclusiveId.value == 0);
    assert(lock.exclusiveCount.value == 0);
    // the early threads did deadlock
    assert(lock.deadlockGeneration.value == 1);

    sy_raw_rwlock_destroy(&lock);
    return 0;
}
