#include "gen_pool.h"
#include "../../core/core_internal.h"
#include "../../types/type_info.hpp"
#include "../alloc_cache_align.hpp"
#include "../locks/locks_internal.hpp"
#include "gen_pool.hpp"
#include "gen_pool_internal.hpp"
#include <cstring>
#include <new>

static_assert(sizeof(sy::GenPool) == sizeof(SyGenPool));
static_assert(alignof(sy::GenPool) == alignof(SyGenPool));

static_assert(sizeof(SyGenOwner) == sizeof(SyGenRef));
static_assert(alignof(SyGenOwner) == alignof(SyGenRef));
static_assert(offsetof(SyGenOwner, gen_) == offsetof(SyGenRef, gen_));
static_assert(offsetof(SyGenOwner, chunk_) == offsetof(SyGenRef, chunk_));
static_assert(offsetof(SyGenOwner, objectIndex_) == offsetof(SyGenRef, objectIndex_));

using namespace sy;

GenPool::GenPool(GenPool&& other) noexcept : impl_(other.impl_) { other.impl_ = nullptr; }

GenPool& GenPool::operator=(GenPool&& other) noexcept {
    if (this != &other) {
        Allocator alloc = this->impl_->allocator;
        {
            std::lock_guard lock(this->impl_->mutex);
            for (auto entry : this->impl_->typedPools) {
                entry.value->~GenTypedPool();
                this->impl_->allocator.freeObject(entry.value);
            }
        }
        this->impl_->~GenPoolImpl();
        alloc.freeObject(this->impl_);
        this->impl_ = other.impl_;
        other.impl_ = nullptr;
    }

    return *this;
}

sy::GenPool::~GenPool() noexcept {
    if (this->impl_ == nullptr) {
        return;
    }

    Allocator alloc = this->impl_->allocator;
    {
        std::lock_guard lock(this->impl_->mutex);
        for (auto entry : this->impl_->typedPools) {
            entry.value->~GenTypedPool();
            alloc.freeObject(entry.value);
        }
    }
    this->impl_->~GenPoolImpl();
    alloc.freeObject(this->impl_);

    this->impl_ = nullptr;
}

Result<GenPool, AllocErr> GenPool::init(Allocator allocator) noexcept {
    auto implRes = allocator.allocAlignedObject<internal::GenPoolImpl>(ALLOC_CACHE_ALIGN);
    if (implRes.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    internal::GenPoolImpl* newImpl = implRes.value();
    new (newImpl) internal::GenPoolImpl();
    GenPool self;
    self.impl_ = newImpl;
    return self;
}

extern "C" {
SY_API SyAllocErr sy_gen_pool_init(SyAllocator allocator, SyGenPool* outGenPool) {
    auto res = sy::GenPool::init(*reinterpret_cast<sy::Allocator*>(&allocator));
    if (res.hasErr()) {
        return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
    }

    sy::GenPool* ascpp = reinterpret_cast<sy::GenPool*>(outGenPool);
    new (ascpp) sy::GenPool(std::move(res.takeValue()));
    return SyAllocErr::SY_ALLOC_ERR_NONE;
}

SY_API SyAllocErr sy_gen_pool_add(SyGenPool* self, void* obj, const SyType* objType,
                                  SyGenOwner* outGenOwner) {
    internal::GenPoolImpl* impl = reinterpret_cast<internal::GenPoolImpl*>(self->impl_);
    const sy::Type* actualObjType = reinterpret_cast<const sy::Type*>(objType);

    impl->mutex.lock();

    internal::GenTypedPool* typedPool = nullptr;
    if (auto found = impl->typedPools.find(actualObjType); !found.hasValue()) {
        auto poolRes = internal::GenTypedPool::init(actualObjType, impl->allocator);
        if (poolRes.hasErr()) {
            impl->mutex.unlock();
            return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
        }
        typedPool = poolRes.value();

        auto mapRes = impl->typedPools.insert(impl->allocator, actualObjType, typedPool);
        if (mapRes.hasErr()) {
            typedPool->~GenTypedPool();
            impl->allocator.freeObject(typedPool);
            impl->mutex.unlock();
            return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
        }
    } else {
        typedPool = found.value();
    }

    impl->mutex.unlock(); // not needed now

    auto addRes = typedPool->addObj(obj);
    if (addRes.hasErr()) {
        return static_cast<SyAllocErr>(addRes.takeErr());
    }

    *outGenOwner = addRes.takeValue();

    return SyAllocErr::SY_ALLOC_ERR_NONE;
}

SY_API void sy_gen_pool_destroy(SyGenPool* self) {
    sy::GenPool* ascpp = reinterpret_cast<sy::GenPool*>(self);
    ascpp->~GenPool();
}

SY_API SyProgramError sy_gen_owner_destroy(SyGenOwner* self) {
    if (self->gen_ == 0) {
        return SY_PROGRAM_ERROR_GEN_REF_STALE;
    }

    auto* chunk = reinterpret_cast<internal::GenTypedPool::Chunk*>(self->chunk_);
    sy_assert(self->objectIndex_ < chunk->capacity, "Invalid object index");

    chunk->typedPoolParent->lock.lockExclusive();

    // this is genuinely the first time ive used compare exchange strong.
    if (!chunk->generations[self->objectIndex_].compare_exchange_strong(
            self->gen_, self->gen_ + 1, std::memory_order_acq_rel)) {
        chunk->typedPoolParent->lock.unlockExclusive();
        return SY_PROGRAM_ERROR_GEN_REF_STALE;
    }

    auto res = chunk->typedPoolParent->type->destroyObject(chunk->objAt(self->objectIndex_));
    if (res.hasErr()) {
        // TODO unconditionally invalidate self if destructor fails?
        // probably to prevent calling again, and have the interpreter just indiscriminately free
        // memory.
        self->gen_ = 0;
        self->chunk_ = nullptr;
        self->objectIndex_ = 0;
        chunk->typedPoolParent->lock.unlockExclusive();
        return static_cast<SyProgramError>(res.err());
    }

    chunk->hasData[self->objectIndex_] = false;
    chunk->typedPoolParent->lock.unlockExclusive();

    self->gen_ = 0;
    self->chunk_ = nullptr;
    self->objectIndex_ = 0;
    return SY_PROGRAM_ERROR_NONE;
}

SY_API SyProgramError sy_gen_owner_load(const SyGenOwner* self, void* outObj) {
    return sy_gen_ref_load(reinterpret_cast<const SyGenRef*>(self), outObj);
}

SY_API SyProgramError sy_gen_owner_store(SyGenOwner* self, void* obj) {
    if (self->gen_ == 0) {
        return SY_PROGRAM_ERROR_GEN_REF_STALE;
    }

    auto* chunk = reinterpret_cast<internal::GenTypedPool::Chunk*>(self->chunk_);
    sy_assert(self->objectIndex_ < chunk->capacity, "Invalid object index");

    if (self->gen_ != chunk->generations[self->objectIndex_].load(std::memory_order_acquire)) {
        return SY_PROGRAM_ERROR_GEN_REF_STALE;
    }

    auto* typedPool = chunk->typedPoolParent;
    sy_assert(typedPool->type->builtinTraits->elementWiseAtomicMove.hasValue(),
              "Needs elementWiseAtomicMove()");

    auto& seq = chunk->seqlocks[self->objectIndex_];

    uint64_t current = seq.load(std::memory_order_relaxed);
    while (true) {
        // only one thread can make the seqlock odd at a time
        if (current & 1) {
            current = seq.load(std::memory_order_relaxed);
            sy::internal::pause();
            continue;
        }
        if (seq.compare_exchange_weak(current, current + 1, std::memory_order_acq_rel,
                                      std::memory_order_relaxed)) {
            break;
        }
        // cas failed
        sy::internal::pause();
    }

    void* slot = chunk->objAt(self->objectIndex_);
    auto elementWiseAtomicMoveRes =
        typedPool->type->builtinTraits->elementWiseAtomicMove.value()->call(slot, obj);

    // make even to mark that done writing
    seq.fetch_add(1, std::memory_order_release);

    if (self->gen_ != chunk->generations[self->objectIndex_].load(std::memory_order_acquire)) {
        return SY_PROGRAM_ERROR_GEN_REF_STALE;
    }

    if (elementWiseAtomicMoveRes.hasErr()) {
        return static_cast<SyProgramError>(elementWiseAtomicMoveRes.err());
    }

    return SY_PROGRAM_ERROR_NONE;
}

SY_API SyGenRef sy_gen_owner_ref(SyGenOwner* self) {
    SyGenRef ref;
    ref.gen_ = self->gen_;
    ref.chunk_ = self->chunk_;
    ref.objectIndex_ = self->objectIndex_;
    return ref;
}

SY_API SyProgramError sy_gen_ref_load(const SyGenRef* self, void* outObj) {
    if (self->gen_ == 0) {
        return SY_PROGRAM_ERROR_GEN_REF_STALE;
    }

    auto* chunk = reinterpret_cast<internal::GenTypedPool::Chunk*>(self->chunk_);
    sy_assert(self->objectIndex_ < chunk->capacity, "Invalid object index");

    auto* typedPool = chunk->typedPoolParent;
    sy_assert(typedPool->type->builtinTraits->elementWiseAtomicClone.hasValue(),
              "Needs elementWiseAtomicClone()");

    auto& seq = chunk->seqlocks[self->objectIndex_];
    auto& gen = chunk->generations[self->objectIndex_];
    const void* slot = chunk->objAt(self->objectIndex_);

    while (true) {
        // odd means someone is writing
        const uint64_t seqBefore = seq.load(std::memory_order_acquire);
        if (seqBefore & 1) {
            sy::internal::pause();
            continue;
        }

        auto cloneRes =
            typedPool->type->builtinTraits->elementWiseAtomicClone.value()->call(outObj, slot);

        // if it changed someone wrote, so re-clone
        if (seq.load(std::memory_order_acquire) != seqBefore) {
            // clone may set something like atomic string ref count, call destructor, also acts as
            // CPU pause
            // don't need to pause cause this does work
            typedPool->type->destroyObject(outObj);
            continue;
        }

        if (gen.load(std::memory_order_acquire) != self->gen_) {
            return SY_PROGRAM_ERROR_GEN_REF_STALE;
        }

        if (cloneRes.hasErr()) {
            return static_cast<SyProgramError>(cloneRes.err());
        }
        return SY_PROGRAM_ERROR_NONE;
    }
}

SY_API SyProgramError sy_gen_ref_store(SyGenRef* self, void* obj) {
    auto* chunk = reinterpret_cast<internal::GenTypedPool::Chunk*>(self->chunk_);
    // shared lock to prevent destruction
    chunk->typedPoolParent->lock.lockShared();
    const SyProgramError err = sy_gen_owner_store(reinterpret_cast<SyGenOwner*>(self), obj);
    chunk->typedPoolParent->lock.unlockShared();
    return err;
}
}

SY_API void sy::internal::ensureNoProgramError(int err) {
    sy_assert(err == 0, "Destructor should't have failed especially with C++ templates");
    (void)err;
}

SY_API int sy::internal::sy_gen_pool_add_impl(GenPool* self, void* obj, const sy::Type* objType,
                                              void* outGenRef) {
    return static_cast<int>(sy_gen_pool_add(reinterpret_cast<SyGenPool*>(self), obj,
                                            reinterpret_cast<const SyType*>(objType),
                                            reinterpret_cast<SyGenOwner*>(outGenRef)));
}

SY_API int sy::internal::sy_gen_owner_destroy_impl(void* self) {
    return static_cast<int>(sy_gen_owner_destroy(reinterpret_cast<SyGenOwner*>(self)));
}

SY_API int sy::internal::sy_gen_owner_load_impl(void* self, void* outObj) {
    return static_cast<int>(sy_gen_ref_load(reinterpret_cast<const SyGenRef*>(self), outObj));
}

SY_API int sy::internal::sy_gen_ref_load_impl(void* self, void* outObj) {
    return static_cast<int>(sy_gen_ref_load(reinterpret_cast<const SyGenRef*>(self), outObj));
}

SY_API int sy::internal::sy_gen_owner_store_impl(void* self, void* obj) {
    return static_cast<int>(sy_gen_owner_store(reinterpret_cast<SyGenOwner*>(self), obj));
}

SY_API int sy::internal::sy_gen_ref_store_impl(void* self, void* obj) {
    return static_cast<int>(sy_gen_ref_store(reinterpret_cast<SyGenRef*>(self), obj));
}
