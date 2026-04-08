//! API
#pragma once
#ifndef SY_THREADING_LOCKS_RWLOCK_HPP_
#define SY_THREADING_LOCKS_RWLOCK_HPP_

#include "../../types/result/result.hpp"

namespace sy {
struct RwLockTest;

namespace internal {
struct RwLockLayout;
} // namespace internal

/// Is zero initialized.
///
/// As sync code executes, it will inevitably call into external functions (C functions) due to
/// being embeddable. As such, some static analysis between external and sync function calls for the
/// compiler is not fully possible.
///
/// This presents a massive challenge when trying to acquire locks, as we cannot prevent, at sync
/// language code compile time, that acquiring an exclusive lock has not already been acquired by
/// that same thread as a shared lock.
///
/// How we can solve this is by combining two methods. Firstly, supporting re-entrant locks. This
/// will mean that a thread trying to re-acquire a lock in the same way as it did before is not a
/// problem. Secondly, allow to "elevate" a lock, meaning turn a shared lock into an exclusive lock
/// without giving up acquisition to another thread. If a thread has a shared lock, and so does
/// another, fail to "elevate" the lock into an exclusive lock. If two thread try to elevate at the
/// same time, also fail.
///
/// Works with ThreadSanitizer.
class SY_API RwLock {
  public:
    enum class AcquireErr : int {
        OutOfMemory = 1,
        HasExclusive = 2,
        HasReaders = 3,
        Deadlock = 4,
    };

    RwLock() = default;

    /// Destroys this RwLock, freeing any allocated memory.
    /// @warning Asserts:
    ///
    /// - No thread has an exclusive lock.
    ///
    /// - No thread has a shared lock.
    ///
    /// - No thread is trying to elevate from a shared lock to an exclusive lock.
    ~RwLock() noexcept;

    RwLock(const RwLock&) = delete;
    RwLock& operator=(const RwLock&) = delete;
    RwLock(RwLock&&) = delete;
    RwLock& operator=(RwLock&&) = delete;

    /// Acquires a shared (read-only) lock on this rwlock.
    /// @return Nothing on success, or `AcquireErr::OutOfMemory` if there was an allocation failure.
    Result<void, AcquireErr> lockShared() noexcept;

    /// Attempts to acquire a shared (read-only) lock on this rwlock.
    /// @return Nothing on success. On failure returns an error containing:
    ///
    /// - `AcquireErr::OutOfMemory` if there was an allocation failure.
    ///
    /// - `AcquireErr::HasExclusive` if there is an exclusive lock not owned by this thread.
    Result<void, AcquireErr> tryLockShared() noexcept;

    /// Unlocks a shared (read-only) lock held by this thread.
    /// @warning Debug asserts:
    ///
    /// - This rwlock is not exclusively locked by another thread
    ///
    /// - This rwlock was shared-locked by this thread
    void unlockShared() noexcept;

    /// Acquires a shared (read-only) lock on this rwlock.
    /// @warning Invokes the sync fatal error handler if memory allocation fails.
    void lockSharedUnchecked() noexcept;

    /// Acquires an exlusive (read-write) lock on this rwlock.
    /// @return Nothing on success. On failure, returns an error containing one of the following:
    ///
    /// - `AcquireErr::OutOfMemory` if there was an allocation failure.
    ///
    /// - `AcquireErr::Deadlock` if other threads have shared locks, or if this thread and one or
    /// more others are attempting to elevate theirs at the same time.
    Result<void, AcquireErr> lockExclusive() noexcept;

    /// Attempts to acquire an exlusive (read-write) lock on this rwlock.
    /// @return Nothing on success. On failure, returns an error containing one of the following:
    ///
    /// - `AcquireErr::OutOfMemory` if there was an allocation failure.
    ///
    /// - `AcquireErr::HasExclusive` if there is an exclusive lock not owned by this thread.
    ///
    /// - `AcquireErr::HasReaders` if one or more other threads are holding a shared lock.
    ///
    /// - `AcquireErr::Deadlock` if this thread and one or more others are attempting to elevate
    /// theirs at the same time.
    /// @warning Debug asserts:
    ///
    /// - This rwlock was re-entered through exclusive locking less than UINT16_MAX times.
    Result<void, AcquireErr> tryLockExclusive() noexcept;

    /// Unlocks an exclusive (read-write) lock held by this thread.
    /// @warning Debug asserts:
    ///
    /// - This rwlock is not exclusively locked by another thread
    ///
    /// - This rwlock was exclusively locked by this thread
    void unlockExclusive() noexcept;

    /// Acquires an exclusive (read-write) lock on this rwlock.
    /// @warning Invokes the sync fatal error handler if memory allocation fails or if a deadlock
    /// occurred.
    void lockExclusiveUnchecked() noexcept;

  private:
    friend struct RwLockTest;

    internal::RwLockLayout* asLayout() noexcept;

    union SY_API Inner {
        struct SY_API Padding {
            uint8_t _p[48]{};
            Padding() = default;
        } padding_;
        void* forceAlign_;

        Inner() : padding_(Padding()) {}
    } inner_;
};
} // namespace sy

#endif // SY_THREADING_RWLOCK_RWLOCK_HPP_
