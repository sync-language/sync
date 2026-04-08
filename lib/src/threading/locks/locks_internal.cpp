#include "locks_internal.hpp"
#include "../../core/core_internal.h"
#include <atomic>
#include <thread>

#if defined(_MSC_VER) || defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
// clang-format off
#include <windows.h>
#if defined(_MSC_VER)
#pragma comment(lib, "Synchronization.lib")
#endif
// clang-format on
#endif // WIN32

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#if defined(__linux__)
#include <errno.h>
#include <linux/futex.h> // FUTEX_WAIT, FUTEX_WAKE
#include <sys/syscall.h>
#include <unistd.h>
#endif

static std::atomic<uint32_t> globalThreadIdGenerator{};
static thread_local uint32_t threadLocalThreadId = 0;

uint32_t sy::internal::getThisThreadId() noexcept {
    if (threadLocalThreadId != 0) {
        return threadLocalThreadId;
    }

    uint32_t fetched = globalThreadIdGenerator.fetch_add(1, std::memory_order_seq_cst);
    sy_assert_release(fetched < (UINT32_MAX - 1), "[getThisThreadId] reached max value for thread "
                                                  "id generator (how?)");

    threadLocalThreadId = fetched + 1;
    return threadLocalThreadId;
}

// WHAT
#if defined(__SANITIZE_THREAD__)
#define SYNC_TSAN_ENABLED 1
#elif defined(__clang__)
#if __has_feature(thread_sanitizer)
#define SYNC_TSAN_ENABLED 1
#endif
#endif

#ifdef SYNC_TSAN_ENABLED
extern "C" {
extern void __tsan_mutex_destroy(void* addr, unsigned flags);
extern void __tsan_mutex_pre_lock(void* addr, unsigned flags);
extern void __tsan_mutex_post_lock(void* addr, unsigned flags);
extern void __tsan_mutex_pre_unlock(void* addr, unsigned flags);
extern void __tsan_mutex_post_unlock(void* addr, unsigned flags);
}
#define __tsan_mutex_not_static 0x1
#endif

void sy::internal::SpinYielder::yield(volatile void* address, uint32_t comparisonValue) noexcept {
    if (this->counter < YIELD_THRESHOLD) {
#if defined(__EMSCRIPTEN__)
        for (uint64_t i = 0; i < MAX_TSC_CYCLES; i++) {
            pause();
        }
        this->counter += 1;
#elif defined(__x86_64__) || defined(_M_AMD64) || defined(__aarch64__) ||                          \
    (defined(__riscv) && (__riscv_xlen == 64))

        const uint64_t start = rdtsc();
        // jitter keeps different threads not perfectly in-sync
        const int jitter = static_cast<int>(rdtsc() & (this->backoffMultiplier - 1));
        // pause count will slowly increase from 1 pause to MAX_BACKOFF
        const int pauseCount = this->backoffMultiplier - jitter;

        for (int i = 0; i < pauseCount; i++) {
            pause();
            if ((rdtsc() - start) >= MAX_TSC_CYCLES) {
                break;
            }
        }

        this->backoffMultiplier *= 2;
        if (this->backoffMultiplier > MAX_BACKOFF) {
            this->backoffMultiplier = 1;
        }
#else
        this->counter += 1;
        std::this_thread::yield();
#endif
    } else if (this->counter < SLEEP_THRESHOLD) {
        this->counter += 1;
        std::this_thread::yield();
    } else {
#if defined(__EMSCRIPTEN__)
        (void)address;
        (void)comparisonValue;
        std::this_thread::yield();
#elif defined(_MSC_VER) || defined(_WIN32)
        WaitOnAddress(address, &comparisonValue, sizeof(uint32_t), 1);
#elif defined(__linux__)
        syscall(__NR_futex, address, FUTEX_WAIT, comparisonValue, nullptr, nullptr, 0);
#elif defined(__MACH__)
        (void)address;
        (void)comparisonValue;
        std::this_thread::yield();
#else
        (void)address;
        (void)comparisonValue;
        std::this_thread::yield();
#endif
        // reset the whole thing
        this->counter = 0;
    }
}

void sy::internal::acquireAtomicFence(std::atomic<uint32_t>& fence) {
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_pre_lock(&fence, 0);
#endif
    // bool expected = false;
    // on success, acquire is fine cause we want to see writes from the previous fence owner that
    // used release.
    // on failure, relaxed is fine since no other shared data is being changed / access.
    // while (!(fence.compare_exchange_weak(expected, true, std::memory_order_acquire,
    //                                      std::memory_order_relaxed))) {
    //     expected = false;
    //     std::this_thread::yield();
    // }

    SpinYielder yielder;
    while (fence.exchange(1u, std::memory_order_acquire) != 0u) {
        do {
            yielder.yield(&fence, 1u);
        } while (fence.load(std::memory_order_relaxed));
    }
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_post_lock(&fence, 0);
#endif
}

void sy::internal::releaseAtomicFence(std::atomic<uint32_t>& fence) {
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_pre_unlock(&fence, 0);
#endif
    fence.store(0u, std::memory_order_release);
#if defined(_MSC_VER) || defined(_WIN32)
    WakeByAddressSingle(&fence);
#elif defined(__linux__)
    syscall(__NR_futex, &fence, FUTEX_WAKE, 1, nullptr, nullptr, 0);
#else
    // emscripten uses wasm_sleep()
    // mac uses std::this_thread::yield()
    // everything else also uses std::this_thread::yield()
#endif
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_post_unlock(&fence, 0);
#endif
}

void sy::internal::tsan_mutex_destroy(std::atomic<uint32_t>& fence) {
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_destroy(&fence, 0);
#else
    (void)fence;
#endif
}
