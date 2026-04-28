#pragma once
#ifndef SY_THREADING_GENERATION_GEN_POOL_INTERNAL_HPP_
#define SY_THREADING_GENERATION_GEN_POOL_INTERNAL_HPP_

#include "../../types/array/dynamic_array.hpp"
#include "../../types/hash/map.hpp"
#include "../../types/option/option.hpp"
#include "../locks/rwlock.hpp"
#include "gen_pool.h"
#include "gen_pool.hpp"
#include <atomic>
#include <mutex>

/*
Creating or destroying an entity in the pool requires acquiring the typed pool's lock.
Swap / clone does not.

`GenRef` instances only need to store a pointer to `Chunk`, and the 32 bit index within the chunk.
*/

namespace sy {
namespace internal {

struct GenPoolImpl {
    std::mutex mutex;
    Allocator allocator;
    /// The GenTypedPool* value type is allocated, guaranteeing pointer stablility through
    /// allocating each element.
    MapUnmanaged<const Type*, internal::GenTypedPool*> typedPools;
};

struct GenTypedPool {
    struct Chunk {
        GenTypedPool* typedPoolParent = nullptr;
        /// All of the object data.
        uint8_t* data = nullptr;
        /// Array of generation counts. Each index corresponds to a range of `data`.
        std::atomic<uint64_t>* generations = nullptr;
        /// Array of seqlock counters. Each index corresponds to a range of `data`.
        std::atomic<uint64_t>* seqlocks = nullptr;
        /// Array to track if a specific index actually has data. Only needs to be read/write when
        /// creating / destroying an entry or iterating. Is different than the generation count, as
        /// that will always increment on create / destroy.
        bool* hasData = nullptr;
        /// Stores entries that  entries.
        DynArrayUnmanaged<uint32_t> freedList{};
        /// Never changes after `Chunk` construction.
        uint32_t capacity = 0;

        Chunk(GenTypedPool* parent) : typedPoolParent(parent) {}

        Result<void, AllocErr> allocateCapacity(Allocator alloc, size_t inCapacity, size_t dataSize,
                                                size_t dataAlign) noexcept;

        Option<uint32_t> firstEmptySlot(bool* fromFreedList) const noexcept;

        void* objAt(uint32_t index);
    };

    static constexpr size_t MAX_CHUNK_COUNT = 24;
    static constexpr size_t CHUNK_BASE_CAPACITY = 256;

    GenPoolImpl* poolOwner;
    RwLock lock;
    Allocator allocator;
    /// Single pointer containing the type this typed pool holds.
    /// Never changes after `GenTypePool` construction.
    const Type* type = nullptr;
    /// Pointer stable chunks.
    /// Each Chunk::capacity:
    /// - 0 - 256 (CHUNK_BASE_CAPACITY)
    /// - 1 - 512 (CHUNK_BASE_CAPACITY * 2^1)
    /// - 2 - 1024 (CHUNK_BASE_CAPACITY * 2^2)
    /// ...
    /// - 23 - 2.14 billion (CHUNK_BASE_CAPACITY * 2^23)
    Chunk* chunks[MAX_CHUNK_COUNT]{};
    /// Max of `MAX_CHUNK_COUNT`.
    size_t chunksLen = 0;

    static Result<GenTypedPool*, AllocErr> init(const Type* type, Allocator alloc) noexcept;

    ~GenTypedPool() noexcept;

    size_t dataAllocationAlign() const noexcept;

    /// Takes ownership of the data at `obj`.
    Result<SyGenOwner, AllocErr> addObj(void* obj) noexcept;
};
} // namespace internal

} // namespace sy

#endif // SY_THREADING_GENERATION_GEN_POOL_INTERNAL_HPP_
