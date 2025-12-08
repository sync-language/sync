#include <assert.h>
#include <atomic>
#include <core_internal.h>
#include <thread>
#include <vector>

std::atomic<int> readyCount{0};
std::atomic<int> maxConcurrentReaders{0};
std::atomic<int> currentReaders{0};

void readerFn(SyRawRwLock* lock) {
    for (int i = 0; i < 100; i++) {
        assert(sy_raw_rwlock_acquire_shared(lock) == SY_ACQUIRE_ERR_NONE);

        int current = currentReaders.fetch_add(1, std::memory_order_seq_cst) + 1;
        int expected = maxConcurrentReaders.load(std::memory_order_seq_cst);
        while (current > expected && !maxConcurrentReaders.compare_exchange_weak(expected, current)) {
            expected = maxConcurrentReaders.load(std::memory_order_seq_cst);
        }

        std::this_thread::yield();

        currentReaders.fetch_sub(1, std::memory_order_seq_cst);
        sy_raw_rwlock_release_shared(lock);
    }
}

int main() {
    SyRawRwLock lock{};
    const int numThreads = 16;
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back(readerFn, &lock);
    }

    for (auto& t : threads) {
        t.join();
    }

    assert(maxConcurrentReaders.load() >= 2);
    assert(lock.readerLen == 0);

    sy_raw_rwlock_destroy(&lock);
    return 0;
}
