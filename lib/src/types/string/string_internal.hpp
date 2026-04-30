#pragma once
#ifndef SY_TYPES_STRING_STRING_INTERNAL_HPP_
#define SY_TYPES_STRING_STRING_INTERNAL_HPP_

#include "../../mem/allocator.hpp"
#include "../../threading/alloc_cache_align.hpp"
#include "../result/result.hpp"
#include "string_slice.hpp"
#include <atomic>

namespace sy {
class String;

namespace internal {
struct AtomicStringHeader {
    /// Amount of bytes used for the data within the header excluding the inline string storage.
    constexpr static size_t HEADER_NON_STRING_BYTES_USED =
        sizeof(size_t) + sizeof(std::atomic<size_t>) + sizeof(Allocator) +
        sizeof(std::atomic<size_t>) + sizeof(std::atomic<bool>);
    /// Size of the inline string buffer.
    constexpr static size_t HEADER_INLINE_STRING_SIZE =
        ALLOC_CACHE_ALIGN - HEADER_NON_STRING_BYTES_USED;
    /// At which byte offset does the inline string data reach 8 byte alignment to leverage those
    /// kinds of optimizations.
    constexpr static size_t HEADER_INLINE_STRING_8_BYTE_ALIGN_OFFSET =
        HEADER_INLINE_STRING_SIZE & (alignof(uint64_t) - 1);

    /// String is immutable so the allocation capacity can be calculated from this.
    /// Does not include null terminator byte.
    size_t len = 0;
    mutable std::atomic<size_t> refCount = 1;
    /// Only valid if `hasHashedStore.load(...) == true`
    mutable Allocator allocator;
    std::atomic<size_t> hashCode = 0;
    std::atomic<bool> hasHashStored = false;
    char inlineString[HEADER_INLINE_STRING_SIZE]{};

    AtomicStringHeader(Allocator inAllocator) : allocator(inAllocator) {}

    static size_t allocationSizeFor(size_t stringLength) noexcept;

    ///
    /// @param self
    static void atomicStringDestroy(String* self) noexcept;

    ///
    /// @param out Assumes empty / stale, to write to the memory
    /// @param src The string to clone into `out` atomically.
    static void atomicStringClone(String* out, const String* src) noexcept;

    ///
    /// @param overwrite The string to overwrite. May contain data.
    /// @param src The string to write into `out` atomically.
    static void atomicStringSet(String* overwrite, const String* src) noexcept;
};
} // namespace internal
} // namespace sy

#endif // SY_TYPES_STRING_STRING_INTERNAL_HPP_
