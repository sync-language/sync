#include <assert.h>
#include <atomic>
#include <chrono>
#include <core_internal.h>
#include <thread>

std::atomic<bool> ready;

void thread1Fn(SyRawRwLock* lock) {
    sy_raw_rwlock_acquire_shared(lock);
    while (ready.load() == false) {
        std::this_thread::yield();
    }
    sy_raw_rwlock_release_shared(lock);
    ready = false; // now t2 can try the exclusive lock
}

void thread2Fn(SyRawRwLock* lock) {
    sy_raw_rwlock_acquire_shared(lock);
    ready = true; // this must be set for t1 to continue
    while (ready.load() == true) {
        std::this_thread::yield();
    }
    assert(sy_raw_rwlock_acquire_exclusive(lock) == SY_ACQUIRE_ERR_NONE);
    sy_raw_rwlock_release_exclusive(lock);
    sy_raw_rwlock_release_shared(lock);
}

int main() {
    ready = false;
    SyRawRwLock lock{};
    std::thread t1(thread1Fn, &lock);
    std::thread t2(thread2Fn, &lock);

    t1.join();
    t2.join();
    sy_raw_rwlock_destroy(&lock);
}