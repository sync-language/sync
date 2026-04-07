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
} // namespace internal
} // namespace sy

#endif // SY_THREADING_LOCKS_LOCKS_INTERNAL_HPP_
