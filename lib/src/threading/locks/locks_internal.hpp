#pragma once
#ifndef SY_THREADING_LOCKS_LOCKS_INTERNAL_HPP_
#define SY_THREADING_LOCKS_LOCKS_INTERNAL_HPP_

#include "../../core/core.h"
#include <atomic>

namespace sy {
namespace internal {
uint32_t getThisThreadId() noexcept;

void acquireAtomicFence(std::atomic<bool>& fence);

void releaseAtomicFence(std::atomic<bool>& fence);

/// Is a no-op if tsan is not used.
void tsan_mutex_destroy(std::atomic<bool>& fence);

struct SY_API ThreadIdStore {
    static constexpr uint32_t SMALL_SIZE = 5;

    uint32_t len{};
    /// If `len <= SMALL_SIZE`, `buf` contains the thread ids.
    /// If `len > SMALL_SIZE` `buf[0]` contains the capacity, and `buf[1]`'s memory address can be
    /// cast to a double pointer which is an allocated memory region containing the thread ids.
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
/// The memory layout of `RwLockLayout` is such that the offset of `readers_.buf[1]`,
/// which is the beginning of where a heap allocated pointer would be stored for
/// `ThreadIdStore`, is at a 16 byte offset. This means, any platform that has 16 byte pointers,
/// such as CHERI, would work.
///
/// Doing it this EXACT layout also means that the RwLock can have a relatively small footprint of
/// just 32 bytes, and by forcing pointer alignment, means it would work in 16 byte pointer aligned
/// systems.
///
/// As such, DO NOT MODIFY THIS LAYOUT.
struct RwLockLayout {
    std::atomic<bool> fence_{};
    uint16_t exclusiveReentrantCount_{};
    std::atomic<uint32_t> exclusiveId_{};
    internal::ThreadIdStore readers_{};
};

} // namespace internal
} // namespace sy

#endif // SY_THREADING_LOCKS_LOCKS_INTERNAL_HPP_
