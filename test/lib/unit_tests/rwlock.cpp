#include <doctest.h>
#include <thread>
#include <threading/locks/locks_internal.hpp>
#include <threading/locks/rwlock.h>
#include <threading/locks/rwlock.hpp>
#include <vector>

using namespace sy;

TEST_CASE("[sy::RwLock] default construct") {
    {
        RwLock lock{};
        (void)lock;
    }
    {
        SyRwLock lock{};
        sy_rwlock_destroy(&lock);
    }
}

TEST_CASE("[sy::RwLock] one thread shared") {
    {
        RwLock lock{};
        REQUIRE(lock.lockShared());
        lock.unlockShared();
    }
    {
        SyRwLock lock{};
        REQUIRE_EQ(sy_rwlock_lock_shared(&lock), SY_ACQUIRE_ERR_NONE);
        sy_rwlock_unlock_shared(&lock);
        sy_rwlock_destroy(&lock);
    }
}

TEST_CASE("[sy::RwLock] one thread exclusive") {
    {
        RwLock lock{};
        REQUIRE(lock.lockExclusive());
        lock.unlockExclusive();
    }
    {
        SyRwLock lock{};
        REQUIRE_EQ(sy_rwlock_lock_exclusive(&lock), SY_ACQUIRE_ERR_NONE);
        sy_rwlock_unlock_exclusive(&lock);
        sy_rwlock_destroy(&lock);
    }
}

TEST_CASE("[sy::RwLock] two thread shared") {
    auto threadFn = [](RwLock* lock) {
        for (int i = 0; i < 10000; i++) {
            REQUIRE(lock->lockShared());
            lock->unlockShared();
        }
    };

    RwLock lock{};
    std::thread t1(threadFn, &lock);
    std::thread t2(threadFn, &lock);

    t1.join();
    t2.join();
}

TEST_CASE("[sy::RwLock] two thread exclusive") {
    auto threadFn = [](RwLock* lock, int* counter) {
        for (int i = 0; i < 10000; i++) {
            REQUIRE(lock->lockExclusive());
            *counter += 1;
            lock->unlockExclusive();
        }
    };

    int counter = 0;
    RwLock lock{};
    std::thread t1(threadFn, &lock, &counter);
    std::thread t2(threadFn, &lock, &counter);

    t1.join();
    t2.join();
    REQUIRE_EQ(counter, 20000);
}

namespace sy {
struct RwLockTest {
    RwLock lock;

    std::atomic<uint32_t>& fence() { return lock.asLayout()->fence_; }
    uint32_t& exclusiveReentrantCount() { return lock.asLayout()->exclusiveReentrantCount_; }
    std::atomic<uint32_t>& exclusiveId() { return lock.asLayout()->exclusiveId_; }
    sy::internal::ThreadIdStore& readers() { return lock.asLayout()->readers_; }
};
} // namespace sy

TEST_CASE_FIXTURE(RwLockTest, "[sy::RwLock] one thread re-enter shared") {
    REQUIRE(this->lock.lockShared());
    REQUIRE_EQ(this->readers().len, 1);
    REQUIRE_EQ(this->exclusiveReentrantCount(), 0);
    REQUIRE_EQ(this->exclusiveId(), 0);

    REQUIRE(this->lock.lockShared());
    REQUIRE_EQ(this->readers().len, 2);
    REQUIRE_EQ(this->exclusiveReentrantCount(), 0);
    REQUIRE_EQ(this->exclusiveId(), 0);

    this->lock.unlockShared();
    REQUIRE_EQ(this->readers().len, 1);
    REQUIRE_EQ(this->exclusiveReentrantCount(), 0);
    REQUIRE_EQ(this->exclusiveId(), 0);

    this->lock.unlockShared();
    REQUIRE_EQ(this->readers().len, 0);
    REQUIRE_EQ(this->exclusiveReentrantCount(), 0);
    REQUIRE_EQ(this->exclusiveId(), 0);

    REQUIRE(this->lock.lockShared());
    REQUIRE_EQ(this->readers().len, 1);
    REQUIRE_EQ(this->exclusiveReentrantCount(), 0);
    REQUIRE_EQ(this->exclusiveId(), 0);

    this->lock.unlockShared();
}

TEST_CASE_FIXTURE(RwLockTest, "[sy::RwLock] one thread re-enter exclusive") {
    REQUIRE(this->lock.lockExclusive());
    REQUIRE_EQ(this->readers().len, 0);
    REQUIRE_EQ(this->exclusiveReentrantCount(), 1);
    REQUIRE_EQ(this->exclusiveId(), internal::getThisThreadId());

    REQUIRE(this->lock.lockExclusive());
    REQUIRE_EQ(this->readers().len, 0);
    REQUIRE_EQ(this->exclusiveReentrantCount(), 2);
    REQUIRE_EQ(this->exclusiveId(), internal::getThisThreadId());

    this->lock.unlockExclusive();
    REQUIRE_EQ(this->readers().len, 0);
    REQUIRE_EQ(this->exclusiveReentrantCount(), 1);
    REQUIRE_EQ(this->exclusiveId(), internal::getThisThreadId());

    this->lock.unlockExclusive();
    REQUIRE_EQ(this->readers().len, 0);
    REQUIRE_EQ(this->exclusiveReentrantCount(), 0);
    REQUIRE_EQ(this->exclusiveId(), 0);

    REQUIRE(this->lock.lockExclusive());
    REQUIRE_EQ(this->readers().len, 0);
    REQUIRE_EQ(this->exclusiveReentrantCount(), 1);
    REQUIRE_EQ(this->exclusiveId(), internal::getThisThreadId());

    this->lock.unlockExclusive();
}

TEST_CASE_FIXTURE(RwLockTest, "[sy::RwLock] one thread re-enter elevate lock") {
    { // acquire shared -> exclusive, release exclusive -> shared
        REQUIRE(this->lock.lockShared());
        REQUIRE(this->lock.lockExclusive());
        REQUIRE_EQ(this->readers().len, 1);
        REQUIRE_EQ(this->exclusiveReentrantCount(), 1);
        REQUIRE_EQ(this->exclusiveId(), internal::getThisThreadId());

        this->lock.unlockExclusive();
        REQUIRE_EQ(this->readers().len, 1);
        REQUIRE_EQ(this->exclusiveReentrantCount(), 0);
        REQUIRE_EQ(this->exclusiveId(), 0);

        this->lock.unlockShared();
        REQUIRE_EQ(this->readers().len, 0);
        REQUIRE_EQ(this->exclusiveReentrantCount(), 0);
        REQUIRE_EQ(this->exclusiveId(), 0);
    }
    { // acquire shared -> exclusive, release shared -> exclusive
        REQUIRE(this->lock.lockShared());
        REQUIRE(this->lock.lockExclusive());
        REQUIRE_EQ(this->readers().len, 1);
        REQUIRE_EQ(this->exclusiveReentrantCount(), 1);
        REQUIRE_EQ(this->exclusiveId(), internal::getThisThreadId());

        this->lock.unlockShared();
        REQUIRE_EQ(this->readers().len, 0);
        REQUIRE_EQ(this->exclusiveReentrantCount(), 1);
        REQUIRE_EQ(this->exclusiveId(), internal::getThisThreadId());

        this->lock.unlockExclusive();
        REQUIRE_EQ(this->readers().len, 0);
        REQUIRE_EQ(this->exclusiveReentrantCount(), 0);
        REQUIRE_EQ(this->exclusiveId(), 0);
    }
    { // acquire exclusive -> shared, release exclusive -> shared
        REQUIRE(this->lock.lockExclusive());
        REQUIRE(this->lock.lockShared());
        REQUIRE_EQ(this->readers().len, 1);
        REQUIRE_EQ(this->exclusiveReentrantCount(), 1);
        REQUIRE_EQ(this->exclusiveId(), internal::getThisThreadId());

        this->lock.unlockExclusive();
        REQUIRE_EQ(this->readers().len, 1);
        REQUIRE_EQ(this->exclusiveReentrantCount(), 0);
        REQUIRE_EQ(this->exclusiveId(), 0);

        this->lock.unlockShared();
        REQUIRE_EQ(this->readers().len, 0);
        REQUIRE_EQ(this->exclusiveReentrantCount(), 0);
        REQUIRE_EQ(this->exclusiveId(), 0);
    }
    { // acquire exclusive -> shared, release shared -> exclusive
        REQUIRE(this->lock.lockExclusive());
        REQUIRE(this->lock.lockShared());
        REQUIRE_EQ(this->readers().len, 1);
        REQUIRE_EQ(this->exclusiveReentrantCount(), 1);
        REQUIRE_EQ(this->exclusiveId(), internal::getThisThreadId());

        this->lock.unlockShared();
        REQUIRE_EQ(this->readers().len, 0);
        REQUIRE_EQ(this->exclusiveReentrantCount(), 1);
        REQUIRE_EQ(this->exclusiveId(), internal::getThisThreadId());

        this->lock.unlockExclusive();
        REQUIRE_EQ(this->readers().len, 0);
        REQUIRE_EQ(this->exclusiveReentrantCount(), 0);
        REQUIRE_EQ(this->exclusiveId(), 0);
    }
}

TEST_CASE_FIXTURE(RwLockTest, "[sy::RwLock] two thread no elevate") {
    std::atomic<bool> ready = false;

    auto thread1Fn = [&ready, this]() {
        this->lock.lockShared();
        ready = true;
        while (ready.load() == true) {
            std::this_thread::yield();
        }
        this->lock.unlockShared();
    };

    auto thread2Fn = [&ready, this]() {
        this->lock.lockShared();
        while (ready.load() == false) {
            std::this_thread::yield();
        }
        CHECK_EQ(this->lock.lockExclusive().err(), RwLock::AcquireErr::Deadlock);
        ready = false;
        this->lock.unlockShared();
    };

    std::thread t1(thread1Fn);
    std::thread t2(thread2Fn);

    t1.join();
    t2.join();
}

TEST_CASE_FIXTURE(RwLockTest, "[sy::RwLock] two thread success elevate") {
    std::atomic<bool> ready = false;

    auto thread1Fn = [&ready, this]() {
        this->lock.lockShared();
        while (ready.load() == false) {
            std::this_thread::yield();
        }
        this->lock.unlockShared();
        ready = false;
    };

    auto thread2Fn = [&ready, this]() {
        this->lock.lockShared();
        ready = true;
        while (ready.load() == true) {
            std::this_thread::yield();
        }
        CHECK(this->lock.lockExclusive());
        this->lock.unlockExclusive();
        this->lock.unlockShared();
    };

    std::thread t1(thread1Fn);
    std::thread t2(thread2Fn);

    t1.join();
    t2.join();
}

TEST_CASE_FIXTURE(RwLockTest, "[sy::RwLock] two thread deadlock") {
    std::atomic<bool> ready = false;
    std::atomic<int> deadlockCount = 0;

    auto thread1Fn = [&ready, &deadlockCount, this]() {
        this->lock.lockShared();
        ready = true;
        while (ready.load() == true) {
            std::this_thread::yield();
        }
        CHECK_EQ(this->lock.lockExclusive().err(), RwLock::AcquireErr::Deadlock);

        deadlockCount.fetch_add(1, std::memory_order_seq_cst);
        while (deadlockCount.load(std::memory_order_seq_cst) < 2) {
            std::this_thread::yield();
        }

        this->lock.unlockShared();
    };

    auto thread2Fn = [&ready, &deadlockCount, this]() {
        while (ready.load() == false) {
            std::this_thread::yield();
        }
        this->lock.lockShared();
        ready = false;
        CHECK_EQ(this->lock.lockExclusive().err(), RwLock::AcquireErr::Deadlock);

        deadlockCount.fetch_add(1, std::memory_order_seq_cst);
        while (deadlockCount.load(std::memory_order_seq_cst) < 2) {
            std::this_thread::yield();
        }

        this->lock.unlockShared();
    };

    std::thread t1(thread1Fn);
    std::thread t2(thread2Fn);

    t1.join();
    t2.join();
}

TEST_CASE_FIXTURE(RwLockTest, "[sy::RwLock] three thread deadlock") {
    std::atomic<int> readyCount = 0;
    std::atomic<int> deadlockCount = 0;

    auto threadFn = [&readyCount, &deadlockCount, this]() {
        CHECK(this->lock.lockShared());

        readyCount.fetch_add(1, std::memory_order_seq_cst);
        while (readyCount.load(std::memory_order_seq_cst) < 3) {
            std::this_thread::yield();
        }
        std::this_thread::yield();

        auto res = this->lock.lockExclusive();
        if (res.hasValue() || res.err() != RwLock::AcquireErr::Deadlock) {
            this->lock.unlockExclusive();
        } else {
            deadlockCount.fetch_add(1, std::memory_order_seq_cst);
        }

        while (deadlockCount.load(std::memory_order_seq_cst) < 2) { // at least two
            std::this_thread::yield();
        }

        this->lock.unlockShared();
    };

    std::thread t1(threadFn);
    std::thread t2(threadFn);
    std::thread t3(threadFn);

    t1.join();
    t2.join();
    t3.join();

    REQUIRE_GE(deadlockCount, 2);
}

TEST_CASE_FIXTURE(RwLockTest, "[sy::RwLock] late arrival not deadlock") {
    std::atomic<int> phase = 0;
    std::atomic<int> deadlockCount = 0;

    auto earlyThread = [&phase, &deadlockCount, this]() {
        CHECK(this->lock.lockShared());

        phase.fetch_add(1, std::memory_order_seq_cst);
        while (phase.load(std::memory_order_seq_cst) < 2) {
            std::this_thread::yield();
        }

        CHECK_EQ(this->lock.lockExclusive().err(), RwLock::AcquireErr::Deadlock);
        deadlockCount.fetch_add(1, std::memory_order_seq_cst);
        while (deadlockCount.load(std::memory_order_seq_cst) < 2) {
            std::this_thread::yield();
        }

        this->lock.unlockShared();

        phase.fetch_add(1, std::memory_order_seq_cst);
    };

    auto lateThread = [&phase, &deadlockCount, this]() {
        while (phase.load(std::memory_order_seq_cst) < 4) {
            std::this_thread::yield();
        }
        CHECK_EQ(deadlockCount, 2);

        CHECK(this->lock.lockShared());
        CHECK(this->lock.lockExclusive());

        this->lock.unlockExclusive();
        this->lock.unlockShared();
    };

    std::thread t1(earlyThread);
    std::thread t2(earlyThread);
    std::thread t3(lateThread);

    t1.join();
    t2.join();
    t3.join();

    REQUIRE_GE(deadlockCount, 2);
}

TEST_CASE_FIXTURE(RwLockTest, "[sy::RwLock] deadlock rapid retry") {
    std::atomic<int> iteration = 0;
    std::atomic<int> deadlockCountIteration1 = 0;
    std::atomic<int> deadlockCountIteration2 = 0;

    auto threadFn = [&iteration, &deadlockCountIteration1, &deadlockCountIteration2, this]() {
        CHECK(this->lock.lockShared());

        iteration.fetch_add(1, std::memory_order_seq_cst);
        while (iteration.load(std::memory_order_seq_cst) < 2) {
            std::this_thread::yield();
        }

        CHECK_EQ(this->lock.lockExclusive().err(), RwLock::AcquireErr::Deadlock);
        deadlockCountIteration1.fetch_add(1, std::memory_order_seq_cst);

        iteration.fetch_add(1, std::memory_order_seq_cst);
        while (iteration.load(std::memory_order_seq_cst) < 4) {
            std::this_thread::yield();
        }

        this->lock.unlockShared();

        CHECK(this->lock.lockShared());

        iteration.fetch_add(1, std::memory_order_seq_cst);
        while (iteration.load(std::memory_order_seq_cst) < 6) {
            std::this_thread::yield();
        }

        CHECK_EQ(this->lock.lockExclusive().err(), RwLock::AcquireErr::Deadlock);
        deadlockCountIteration2.fetch_add(1, std::memory_order_seq_cst);

        iteration.fetch_add(1, std::memory_order_seq_cst);
        while (iteration.load(std::memory_order_seq_cst) < 8) {
            std::this_thread::yield();
        }
        this->lock.unlockShared();
    };

    std::thread t1(threadFn);
    std::thread t2(threadFn);

    t1.join();
    t2.join();

    REQUIRE_EQ(deadlockCountIteration1.load(), 2);
    REQUIRE_EQ(deadlockCountIteration2.load(), 2);

    // clean state
    REQUIRE_EQ(this->readers().len, 0);
    REQUIRE_EQ(this->exclusiveId(), 0);
    REQUIRE_EQ(this->exclusiveReentrantCount(), 0);
}

TEST_CASE_FIXTURE(RwLockTest, "[sy::RwLock] try acquire behaviour") {
    std::atomic<bool> thread1HasExclusive = false;
    std::atomic<bool> thread2CanProceed = false;

    auto thread1Fn = [&thread1HasExclusive, &thread2CanProceed, this]() {
        CHECK(this->lock.lockExclusive());
        thread1HasExclusive.store(true);

        while (!thread2CanProceed.load(std::memory_order_seq_cst)) {
            std::this_thread::yield();
        }

        this->lock.unlockExclusive();
    };

    auto thread2Fn = [&thread1HasExclusive, &thread2CanProceed, this]() {
        while (!thread1HasExclusive.load(std::memory_order_seq_cst)) {
            std::this_thread::yield();
        }

        CHECK_EQ(this->lock.tryLockShared().err(), RwLock::AcquireErr::HasExclusive);
        CHECK_EQ(this->lock.tryLockExclusive().err(), RwLock::AcquireErr::HasExclusive);

        thread2CanProceed.store(true, std::memory_order_seq_cst);
        std::this_thread::yield();

        CHECK(this->lock.lockShared());
        this->lock.unlockShared();
    };

    std::thread t1(thread1Fn);
    std::thread t2(thread2Fn);

    t1.join();
    t2.join();
}

TEST_CASE_FIXTURE(RwLockTest, "[sy::RwLock] try acquire behaviour") {
    std::atomic<int> maxConcurrentReaders = 0;
    std::atomic<int> currentReaders = 0;

    auto readerFn = [&maxConcurrentReaders, &currentReaders, this]() {
        for (int i = 0; i < 500; i++) {
            CHECK(this->lock.lockShared());

            int current = currentReaders.fetch_add(1, std::memory_order_seq_cst) + 1;
            int expected = maxConcurrentReaders.load(std::memory_order_seq_cst);
            while (current > expected &&
                   !maxConcurrentReaders.compare_exchange_weak(expected, current)) {
                expected = maxConcurrentReaders.load(std::memory_order_seq_cst);
            }

            std::this_thread::yield();

            currentReaders.fetch_sub(1, std::memory_order_seq_cst);

            this->lock.unlockShared();
        }
    };

    const int numThreads = 16;
    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back(readerFn);
    }

    for (auto& t : threads) {
        t.join();
    }
}

TEST_CASE_FIXTURE(RwLockTest, "[sy::RwLock] deep re-entrance") {
    for (uint32_t i = 1; i <= 100; i++) {
        this->lock.lockShared();
        CHECK_EQ(this->readers().len, i);
    }
    for (uint32_t i = 100; i >= 1; i--) {
        CHECK_EQ(this->readers().len, i);
        this->lock.unlockShared();
    }
    CHECK_EQ(this->readers().len, 0);

    for (uint32_t i = 1; i <= 100; i++) {
        this->lock.lockExclusive();
        CHECK_EQ(this->exclusiveId(), internal::getThisThreadId());
        CHECK_EQ(this->exclusiveReentrantCount(), i);
    }
    for (uint32_t i = 100; i >= 1; i--) {
        CHECK_EQ(this->exclusiveId(), internal::getThisThreadId());
        CHECK_EQ(this->exclusiveReentrantCount(), i);
        this->lock.unlockExclusive();
    }

    CHECK_EQ(this->readers().len, 0);
    CHECK_EQ(this->exclusiveId(), 0);
    CHECK_EQ(this->exclusiveReentrantCount(), 0);
}

TEST_CASE_FIXTURE(RwLockTest, "[sy::RwLock] try acquire behaviour") {
    std::atomic<int> sharedCounter = 0;

    auto readerFn = [&sharedCounter, this]() {
        for (int i = 0; i < 500; i++) {
            this->lock.lockShared();
            int value = sharedCounter.load();
            (void)value;
            this->lock.unlockShared();
        }
    };

    auto writerFn = [&sharedCounter, this]() {
        for (int i = 0; i < 500; i++) {
            this->lock.lockExclusive();
            sharedCounter.fetch_add(1);
            this->lock.unlockExclusive();
        }
    };

    const int numThreads = 16;
    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    for (int i = 0; i < 8; i++) {
        threads.emplace_back(readerFn);
    }
    for (int i = 0; i < 8; i++) {
        threads.emplace_back(writerFn);
    }

    for (auto& t : threads) {
        t.join();
    }

    CHECK_EQ(sharedCounter, 4000);
}

TEST_CASE_FIXTURE(RwLockTest, "[sy::RwLock] multithread downgrade exclusive to shared") {
    std::atomic<bool> thread1Downgraded = false;

    auto thread1Fn = [&thread1Downgraded, this]() {
        CHECK(this->lock.lockExclusive());
        CHECK(this->lock.lockShared());
        this->lock.unlockExclusive();

        thread1Downgraded.store(true, std::memory_order_seq_cst);
        while (thread1Downgraded.load(std::memory_order_seq_cst) == true) {
            std::this_thread::yield();
        }

        this->lock.unlockShared();
    };

    auto thread2Fn = [&thread1Downgraded, this]() {
        while (!thread1Downgraded.load(std::memory_order_seq_cst)) {
            std::this_thread::yield();
        }

        CHECK(this->lock.lockShared());
        thread1Downgraded.store(false);
        this->lock.unlockShared();
    };

    std::thread t1(thread1Fn);
    std::thread t2(thread2Fn);

    t1.join();
    t2.join();
}
