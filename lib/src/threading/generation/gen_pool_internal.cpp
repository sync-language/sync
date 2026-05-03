#include "gen_pool_internal.hpp"
#include "../../core/core_internal.h"
#include "../../types/type_info.hpp"
#include "../alloc_cache_align.hpp"
#include <cstring>
#include <new>

using namespace sy;
using sy::internal::GenTypedPool;

Result<GenTypedPool*, AllocErr> sy::internal::GenTypedPool::init(const Type* type,
                                                                 Allocator alloc) noexcept {
    auto poolAllocRes = alloc.allocObject<GenTypedPool>();
    if (poolAllocRes.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }
    GenTypedPool* self = poolAllocRes.value();
    new (self) GenTypedPool();
    self->allocator = alloc;
    self->type = type;

    auto chunkRes = alloc.allocObject<Chunk>();
    if (chunkRes.hasErr()) {
        alloc.freeObject(self);
        return Error(AllocErr::OutOfMemory);
    }
    Chunk* chunk = chunkRes.value();
    new (chunk) Chunk(self);
    auto chunkDataRes =
        chunk->allocateCapacity(alloc, CHUNK_BASE_CAPACITY, type->sizeType, type->alignType);
    if (chunkDataRes.hasErr()) {
        alloc.freeObject(self);
        alloc.freeObject(chunk);
        return Error(AllocErr::OutOfMemory);
    }

    self->chunks[0] = chunk;
    self->chunksLen = 1;

    return self;
}

internal::GenTypedPool::~GenTypedPool() noexcept {
    this->lock.lockExclusive();
    for (size_t i = 0; i < this->chunksLen; i++) {
        Chunk* chunk = this->chunks[i];
        for (size_t j = 0; j < chunk->capacity; j++) {
            if (chunk->hasData[j]) {
                uint8_t* dataStart = &chunk->data[this->type->sizeType * j];
                this->type->destroyObject(reinterpret_cast<void*>(dataStart));
            }
        }

        allocator.freeAlignedArray(chunk->data, chunk->capacity * this->type->sizeType,
                                   this->dataAllocationAlign());
        allocator.freeAlignedArray(chunk->generations, chunk->capacity, ALLOC_CACHE_ALIGN);
        allocator.freeAlignedArray(chunk->seqlocks, chunk->capacity, ALLOC_CACHE_ALIGN);
        allocator.freeAlignedArray(chunk->hasData, chunk->capacity, ALLOC_CACHE_ALIGN);
        chunk->freedList.destroy(this->allocator);
        allocator.freeObject(chunk);
    }
    this->lock.unlockExclusive();
}

size_t sy::internal::GenTypedPool::dataAllocationAlign() const noexcept {
    // Doesn't need to lock here, as `GenTypedPool::type` never changes after construction, and is
    // the only member accessed here.
    if (this->type->alignType > ALLOC_CACHE_ALIGN) {
        return this->type->alignType;
    }
    return ALLOC_CACHE_ALIGN;
}

Result<SyGenOwner, AllocErr> sy::internal::GenTypedPool::addObj(void* obj) noexcept {
    this->lock.lockExclusive();

    SyGenOwner owner;

    for (size_t i = 0; i < chunksLen; i++) {
        Chunk* chunk = this->chunks[i];
        bool fromFreedList{};
        auto slot = this->chunks[i]->firstEmptySlot(&fromFreedList);
        if (slot.hasValue()) {
            const uint32_t index = slot.value();
            uint8_t* data = &chunk->data[static_cast<size_t>(index) * this->type->sizeType];
            memcpy(reinterpret_cast<void*>(data), obj, this->type->sizeType);

            const uint64_t genCount = chunk->generations[index].fetch_add(1);
            sy_assert(genCount < (UINT64_MAX - 1),
                      "Generation count exceeded 64 bit integer limit");

            chunk->hasData[index] = true;
            chunk->seqlocks[index].store(0, std::memory_order_relaxed);

            owner.gen_ = genCount + 1;
            owner.chunk_ = reinterpret_cast<void*>(chunk);
            owner.objectIndex_ = index;
            this->lock.unlockExclusive();
            return owner;
        }
    }

    sy_assert((this->chunksLen + 1) < MAX_CHUNK_COUNT, "Too many chunks");

    // TODO deallocate the new pool if any allocations fails?

    {
        auto chunkRes = this->allocator.allocObject<Chunk>();
        if (chunkRes.hasErr()) {
            this->lock.unlockExclusive();
            return Error(AllocErr::OutOfMemory);
        }
        Chunk* newChunk = chunkRes.value();
        // if there is 1 chunk already, `CHUNK_BASE_CAPACITY * 2^1` which is the same as
        // `CHUNK_BASE_CAPACITY * (1 << 1)`.
        const size_t newChunkCapacity =
            CHUNK_BASE_CAPACITY * (static_cast<size_t>(1) << this->chunksLen);
        auto chunkDataRes = newChunk->allocateCapacity(this->allocator, newChunkCapacity,
                                                       type->sizeType, type->alignType);
        if (chunkDataRes.hasErr()) {
            this->allocator.freeObject(newChunk);
            this->lock.unlockExclusive();
            return Error(AllocErr::OutOfMemory);
        }

        this->chunks[this->chunksLen] = newChunk;
        this->chunksLen += 1;

        const uint32_t index = 0;
        uint8_t* data = &newChunk->data[static_cast<size_t>(index) * this->type->sizeType];
        memcpy(reinterpret_cast<void*>(data), obj, this->type->sizeType);

        const uint64_t genCount = newChunk->generations[index].fetch_add(1);

        newChunk->hasData[index] = true;

        owner.gen_ = genCount + 1;
        owner.chunk_ = reinterpret_cast<void*>(newChunk);
        owner.objectIndex_ = index;
    }

    this->lock.unlockExclusive();
    return owner;
}

Result<void, AllocErr>
sy::internal::GenTypedPool::Chunk::allocateCapacity(Allocator alloc, size_t inCapacity,
                                                    size_t dataSize, size_t dataAlign) noexcept {
    sy_assert(this->capacity == 0, "Can only allocate once");

    auto dataRes = alloc.allocAlignedArray<uint8_t>(dataSize * inCapacity, dataAlign);
    if (dataRes.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }
    this->data = dataRes.value();

    auto generationsRes = alloc.allocAlignedArray<std::atomic<uint64_t>>(inCapacity, dataAlign);
    if (generationsRes.hasErr()) {
        alloc.freeAlignedArray(this->data, dataSize * inCapacity, dataAlign);
        return Error(AllocErr::OutOfMemory);
    }
    this->generations = generationsRes.value();

    auto seqlocksRes = alloc.allocAlignedArray<std::atomic<uint64_t>>(inCapacity, dataAlign);
    if (seqlocksRes.hasErr()) {
        alloc.freeAlignedArray(this->data, dataSize * inCapacity, dataAlign);
        alloc.freeAlignedArray(this->generations, inCapacity, dataAlign);
        return Error(AllocErr::OutOfMemory);
    }
    this->seqlocks = seqlocksRes.value();

    auto hasDataRes = alloc.allocAlignedArray<bool>(inCapacity, dataAlign);
    if (hasDataRes.hasErr()) {
        alloc.freeAlignedArray(this->data, dataSize * inCapacity, dataAlign);
        alloc.freeAlignedArray(this->generations, inCapacity, dataAlign);
        alloc.freeAlignedArray(this->seqlocks, inCapacity, dataAlign);
        return Error(AllocErr::OutOfMemory);
    }
    this->hasData = hasDataRes.value();

    for (size_t i = 0; i < inCapacity; i++) {
        this->generations[i].store(0, std::memory_order_relaxed);
    }
    for (size_t i = 0; i < inCapacity; i++) {
        this->seqlocks[i].store(0, std::memory_order_relaxed);
    }
    for (size_t i = 0; i < inCapacity; i++) {
        this->hasData[i] = false;
    }

    this->capacity = static_cast<uint32_t>(inCapacity);

    return {};
}

Option<uint32_t>
sy::internal::GenTypedPool::Chunk::firstEmptySlot(bool* fromFreedList) const noexcept {
    // TODO OPTIMIZE THIS

    if (this->freedList.len() > 0) {
        *fromFreedList = true;
        return this->freedList.at(this->freedList.len() - 1);
    }

    for (uint32_t i = 0; i < this->capacity; i++) {
        if (this->hasData[i]) {
            continue;
        }
        return i;
    }

    return Option<uint32_t>();
}

void* sy::internal::GenTypedPool::Chunk::objAt(uint32_t index) {
    uint8_t* mem = &this->data[static_cast<size_t>(index) * this->typedPoolParent->type->sizeType];
    return static_cast<void*>(mem);
}
