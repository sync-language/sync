#include "string_internal.hpp"
#include "../../core/core_internal.h"
#include "../../threading/locks/locks_internal.hpp"
#include "string.hpp"

// no bytes wasted
static_assert(sizeof(sy::internal::AtomicStringHeader) == ALLOC_CACHE_ALIGN);

size_t sy::internal::AtomicStringHeader::allocationSizeFor(size_t stringLength) noexcept {
    const size_t lenWithNullTerm = stringLength + 1;
    if (lenWithNullTerm <= HEADER_INLINE_STRING_SIZE) {
        return sizeof(AtomicStringHeader);
    }

    size_t nonInlineStringSize = lenWithNullTerm - HEADER_INLINE_STRING_SIZE;
    const size_t remainder = nonInlineStringSize % ALLOC_CACHE_ALIGN;
    if (remainder != 0) {
        nonInlineStringSize += ALLOC_CACHE_ALIGN - remainder;
    }

    return sizeof(AtomicStringHeader) + nonInlineStringSize;
}

void sy::internal::AtomicStringHeader::atomicStringDestroy(AtomicString* self) noexcept {
    std::atomic<const internal::AtomicStringHeader*>* atomicImpl =
        const_cast<std::atomic<const internal::AtomicStringHeader*>*>(
            reinterpret_cast<const std::atomic<const internal::AtomicStringHeader*>*>(
                &self->impl_));

    // lock here is held super briefly, and is O(1) with atomicStringSet() and atomicStringClone().
    while (true) {
        const internal::AtomicStringHeader* current = atomicImpl->load(std::memory_order_seq_cst);
        const uintptr_t asUptr = reinterpret_cast<uintptr_t>(current);
        // header pointer is always ALLOC_CACHE_ALIGN aligned, so using the lowest bit as the lock
        // is fine
        if (asUptr & 1) {
            sy::internal::pause();
            continue;
        }

        if (!atomicImpl->compare_exchange_weak(
                current, reinterpret_cast<const internal::AtomicStringHeader*>(asUptr | 1),
                std::memory_order_acq_rel, std::memory_order_relaxed)) {
            sy::internal::pause();
            continue;
        }

        if (current != nullptr) {
            const size_t prevRefCount = current->refCount.fetch_sub(1);
            // unlock
            atomicImpl->store(nullptr, std::memory_order_seq_cst);
            if (prevRefCount == 1) {
                Allocator alloc = current->allocator;
                const size_t totalAllocationSize = current->allocationSizeFor(current->len);
                alloc.freeAlignedArray(
                    reinterpret_cast<uint8_t*>(const_cast<internal::AtomicStringHeader*>(current)),
                    totalAllocationSize, ALLOC_CACHE_ALIGN);
            }
        } else {
            // still must unlock
            atomicImpl->store(nullptr, std::memory_order_seq_cst);
        }

        return;
    }
}

void sy::internal::AtomicStringHeader::atomicStringClone(AtomicString* out,
                                                         const AtomicString* src) noexcept {
    std::atomic<const internal::AtomicStringHeader*>* atomicSrcImpl =
        const_cast<std::atomic<const internal::AtomicStringHeader*>*>(
            reinterpret_cast<const std::atomic<const internal::AtomicStringHeader*>*>(&src->impl_));

    // lock here is held super briefly, and is O(1) with atomicStringDestroy() and
    // atomicStringSet().
    while (true) {
        const internal::AtomicStringHeader* current =
            atomicSrcImpl->load(std::memory_order_seq_cst);
        const uintptr_t asUptr = reinterpret_cast<uintptr_t>(current);
        // header pointer is always ALLOC_CACHE_ALIGN aligned, so using the lowest bit as the lock
        // is fine
        if (asUptr & 1) {
            sy::internal::pause();
            continue;
        }

        if (!atomicSrcImpl->compare_exchange_weak(
                current, reinterpret_cast<const internal::AtomicStringHeader*>(asUptr | 1),
                std::memory_order_acq_rel, std::memory_order_relaxed)) {
            sy::internal::pause();
            continue;
        }

        if (current != nullptr) {
            const size_t prevRefCount = current->refCount.fetch_add(1);
            sy_assert(prevRefCount < (SIZE_MAX - 1), "ref count too big");
            (void)prevRefCount;
        }
        out->impl_ = current;
        // store with cleared bit to unlock
        atomicSrcImpl->store(current, std::memory_order_seq_cst);
        return;
    }
}

void sy::internal::AtomicStringHeader::atomicStringSet(AtomicString* overwrite,
                                                       const AtomicString* src) noexcept {
    std::atomic<const internal::AtomicStringHeader*>* atomicOverwriteImpl =
        const_cast<std::atomic<const internal::AtomicStringHeader*>*>(
            reinterpret_cast<const std::atomic<const internal::AtomicStringHeader*>*>(
                &overwrite->impl_));

    // lock here is held super briefly, and is O(1) with atomicStringDestroy() and
    // atomicStringSet().
    while (true) {
        const internal::AtomicStringHeader* current =
            atomicOverwriteImpl->load(std::memory_order_seq_cst);
        const uintptr_t asUptr = reinterpret_cast<uintptr_t>(current);
        // header pointer is always ALLOC_CACHE_ALIGN aligned, so using the lowest bit as the lock
        // is fine
        if (asUptr & 1) {
            sy::internal::pause();
            continue;
        }

        if (!atomicOverwriteImpl->compare_exchange_weak(
                current, reinterpret_cast<const internal::AtomicStringHeader*>(asUptr | 1),
                std::memory_order_acq_rel, std::memory_order_relaxed)) {
            sy::internal::pause();
            continue;
        }

        if (src->impl_ != nullptr) {
            const size_t prevSrcRefCount = src->impl_->refCount.fetch_add(1);
            sy_assert(prevSrcRefCount < (SIZE_MAX - 1), "ref count too big");
            (void)prevSrcRefCount;
        }

        // overwrite and unlock in one go
        atomicOverwriteImpl->store(src->impl_, std::memory_order_seq_cst);

        // now that it's unlocked, feel free to delete the old if necessary
        if (current != nullptr) {
            const size_t prevRefCount = current->refCount.fetch_sub(1);
            if (prevRefCount == 1) {
                Allocator alloc = current->allocator;
                const size_t totalAllocationSize = current->allocationSizeFor(current->len);
                alloc.freeAlignedArray(
                    reinterpret_cast<uint8_t*>(const_cast<internal::AtomicStringHeader*>(current)),
                    totalAllocationSize, ALLOC_CACHE_ALIGN);
            }
        }
        return;
    }
}
