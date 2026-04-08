#pragma once
#ifndef SY_THREADING_LOCKS_LOCKS_INTERNAL_HPP_
#define SY_THREADING_LOCKS_LOCKS_INTERNAL_HPP_

#include "../../core/core.h"
#include <atomic>

#if defined(__x86_64__) || defined(_M_AMD64)
#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(__rdtsc)
#elif defined(__GNUC__) || defined(__clang__)
#include <x86intrin.h>
#endif
#endif // defined(__x86_64__) || defined(_M_AMD64)

#if defined(_MSC_VER)
#pragma intrinsic(__yield)
#elif defined(__GNUC__) || defined(__clang__)
#include <arm_acle.h>
#endif

namespace sy {
namespace internal {
uint32_t getThisThreadId() noexcept;

// https://www.siliceum.com/en/blog/post/spinning-around/?s=r
struct SpinYielder {
    static constexpr uint64_t MAX_TSC_CYCLES = 500; // 100ns at 5.0ghz
    static constexpr int MAX_BACKOFF = 64;          // 140 cycles (worse case)
    static constexpr int YIELD_THRESHOLD = 100; // takes 1us minimum before yields can even kick in
    static constexpr int SLEEP_THRESHOLD = 500;

    int counter = 0;
    int backoffMultiplier = 1;

    void yield(volatile void* address, uint32_t comparisonValue) noexcept;
};

static inline void pause() {
#if defined(__EMSCRIPTEN__)
    __asm__ __volatile__("" ::: "memory");
#elif defined(__x86_64__) || defined(_M_AMD64)
    _mm_pause();
#elif defined(__aarch64__)
    __yield();
#elif defined(__riscv) && (__riscv_xlen == 64)
    __builtin_riscv_pause();
#endif
    // any other targets i'm not sure. maybe a contributor has some good idea?
}

static inline uint64_t rdtsc() {
#if defined(__EMSCRIPTEN__)
    return 0;
#elif defined(__x86_64__) || defined(_M_AMD64)
    return __rdtsc();
#elif defined(__aarch64__)
    // https://stackoverflow.com/a/67968296
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#elif defined(__riscv) && (__riscv_xlen == 64)
    uint64_t val;
    asm volatile("rdcycle %0" : "=r"(val));
    return val;
#else
    return 0;
#endif
}

void acquireAtomicFence(std::atomic<uint32_t>& fence);

void releaseAtomicFence(std::atomic<uint32_t>& fence);

/// Is a no-op if tsan is not used.
void tsan_mutex_destroy(std::atomic<uint32_t>& fence);

struct ThreadIdStore {
    static constexpr uint32_t SMALL_SIZE = 8;
    static constexpr uint32_t HEAP_PTR_INDEX = 0;
    static constexpr uint32_t CAPACITY_INDEX = SMALL_SIZE - 1;

    uint32_t len{};
    /// If `len <= SMALL_SIZE`, `buf` contains the thread ids.
    /// If `len > SMALL_SIZE` `buf[CAPACITY_INDEX]` contains the capacity, and
    /// `buf[HEAP_PTR_INDEX]`'s memory address can be cast to a double pointer which is an allocated
    /// memory region containing the thread ids.
    uint32_t buf[SMALL_SIZE]{};

    /// Adds a thread id entry to the storage. This function allocates memory, and thus is fallible.
    /// @param threadId The id to add.
    /// @return `false` if it failed to allocate memory, `true` on success.
    bool add(uint32_t threadId) noexcept;

    /// Removes the first occurrence of `threadId` within the storage.
    /// @param threadId The id to remove.
    void removeFirstInstance(uint32_t threadId) noexcept;

    /// Checks if `threadId` is within the storage. `threadId` can appear multiple times.
    /// @param threadId The id to check.
    /// @return `true` if `threadId` is in the storage, otherwise `false`.
    bool contains(uint32_t threadId) const noexcept;

    /// Checks if `threadId` is the only entry in the storage. `threadId` can appear multiple times.
    /// @param threadId The id to check.
    /// @return `true` if `threadId` is the only entry, otherwise `false`.
    bool isOnlyEntry(uint32_t threadId) const noexcept;

    ThreadIdStore() = default;

    ~ThreadIdStore() noexcept;

    ThreadIdStore(ThreadIdStore&& other) noexcept;
};

/// NOTE to the developer reading this internal implementation, this is some tomfoolery.
///
/// The memory layout of `RwLockLayout` is such that the offset of `readers_.buf[0]`,
/// which is the beginning of where a heap allocated pointer would be stored for
/// `ThreadIdStore`, is at a 16 byte offset. This means, any platform that has 16 byte pointers,
/// such as CHERI, would work.
///
/// Doing it this EXACT layout also means that the RwLock can have a relatively small footprint of
/// just 48 bytes, and by forcing pointer alignment, means it would work in 16 byte pointer aligned
/// systems.
///
/// As such, DO NOT MODIFY THIS LAYOUT.
struct RwLockLayout {
    std::atomic<uint32_t> fence_{};
    uint32_t exclusiveReentrantCount_{};
    std::atomic<uint32_t> exclusiveId_{};
    internal::ThreadIdStore readers_{};
};

} // namespace internal
} // namespace sy

#endif // SY_THREADING_LOCKS_LOCKS_INTERNAL_HPP_
