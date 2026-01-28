#include <assert.h>
#include <atomic>
#include <core_internal.h>
#include <thread>
#include <vector>

std::atomic<int> sharedCounter{0};

void readerFn(SyRawRwLock* lock, std::atomic<int>* counter) {
    for (int i = 0; i < 500; i++) {
        assert(sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);
        int value = counter->load(std::memory_order_seq_cst);
        (void)value;
        sy_raw_rwlock_release_shared(lock);
    }
}

void writerFn(SyRawRwLock* lock, std::atomic<int>* counter) {
    for (int i = 0; i < 100; i++) {
        assert(sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_NONE);
        counter->fetch_add(1, std::memory_order_seq_cst);
        sy_raw_rwlock_release_exclusive(lock);
    }
}

int main() {
    SyRawRwLock lock{};
    std::vector<std::thread> threads;

    for (int i = 0; i < 5; i++) {
        threads.emplace_back(readerFn, &lock, &sharedCounter);
        std::this_thread::yield(); // TSan false positives
    }

    for (int i = 0; i < 2; i++) {
        threads.emplace_back(writerFn, &lock, &sharedCounter);
        std::this_thread::yield(); // TSan false positives
    }

    for (auto& t : threads) {
        t.join();
    }

    assert(sharedCounter.load() == 200);
    assert(lock.readerLen == 0);
    assert(lock.exclusiveId.value == 0);

    sy_raw_rwlock_destroy(&lock);
    return 0;
}
