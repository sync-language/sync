#include "locks_internal.hpp"
#include "../../core/core_internal.h"
#include <atomic>
#include <thread>

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

void sy::internal::acquireAtomicFence(std::atomic<bool>& fence) {
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_pre_lock(&fence, 0);
#endif
    bool expected = false;
    // on success, acquire is fine cause we want to see writes from the previous fence owner that
    // used release.
    // on failure, relaxed is fine since no other shared data is being changed / access.
    while (!(fence.compare_exchange_weak(expected, true, std::memory_order_acquire,
                                         std::memory_order_relaxed))) {
        expected = false;
        std::this_thread::yield();
    }
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_post_lock(&fence, 0);
#endif
}

void sy::internal::releaseAtomicFence(std::atomic<bool>& fence) {
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_pre_unlock(&fence, 0);
#endif
    fence.store(false, std::memory_order_release);
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_post_unlock(&fence, 0);
#endif
}

void sy::internal::tsan_mutex_destroy(std::atomic<bool>& fence) {
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_destroy(&fence, 0);
#else
    (void)fence;
#endif
}
