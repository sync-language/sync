#include "gen_pool.h"
#include "../../core/core_internal.h"
#include "../../types/type_info.hpp"
#include "../alloc_cache_align.hpp"
#include "gen_pool.hpp"
#include "gen_pool_internal.hpp"
#include <cstring>
#include <new>

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
    std::lock_guard lock(impl->mutex);

    internal::GenTypedPool* typedPool = nullptr;
    if (auto found = impl->typedPools.find(actualObjType); !found.hasValue()) {
        auto poolRes = internal::GenTypedPool::init(actualObjType, impl->allocator);
        if (poolRes.hasErr()) {
            return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
        }
        typedPool = poolRes.value();

        auto mapRes = impl->typedPools.insert(impl->allocator, actualObjType, typedPool);
        if (mapRes.hasErr()) {
            typedPool->~GenTypedPool();
            impl->allocator.freeObject(typedPool);
            return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
        }
    } else {
        typedPool = found.value();
    }

    auto addRes = typedPool->addObj(obj);
    if (addRes.hasErr()) {
        return static_cast<SyAllocErr>(addRes.takeErr());
    }

    *outGenOwner = addRes.takeValue();
    return SyAllocErr::SY_ALLOC_ERR_NONE;
}

SY_API SyProgramError sy_gen_owner_destroy(SyGenOwner* self) {
    auto* chunk = reinterpret_cast<internal::GenTypedPool::Chunk*>(self->chunk_);
    sy_assert(self->objectIndex_ < chunk->capacity, "Invalid object index");

    chunk->typedPoolParent->lock.lockExclusive();

    // this is genuinely the first time ive used compare exchange strong.
    if (chunk->generations[self->objectIndex_].compare_exchange_strong(self->gen_, self->gen_ + 1,
                                                                       std::memory_order_acq_rel)) {
        chunk->typedPoolParent->lock.unlockExclusive();
        return SY_PROGRAM_ERROR_NONE;
    }

    auto res = chunk->typedPoolParent->type->destroyObject(chunk->objAt(self->objectIndex_));
    chunk->typedPoolParent->lock.unlockExclusive();
    if (res.hasErr()) {
        // TODO unconditionally invalidate self if destructor fails?
        return static_cast<SyProgramError>(res.err());
    }

    self->gen_ = 0;
    self->chunk_ = nullptr;
    self->objectIndex_ = 0;
    return SY_PROGRAM_ERROR_NONE;
}

SY_API SyGenRef sy_gen_owner_ref(const SyGenOwner* self) {
    SyGenRef ref;
    ref.gen_ = self->gen_;
    ref.chunk_ = self->chunk_;
    ref.objectIndex_ = self->objectIndex_;
    return ref;
}
}

SY_API void sy::internal::ensureNoProgramError(int err) {
    sy_assert(err == 0, "Destructor should't have failed especially with C++ templates");
    (void)err;
}

SY_API int sy_gen_pool_add_impl(GenPool* self, void* obj, const sy::Type* objType,
                                void* outGenRef) {
    return static_cast<int>(sy_gen_pool_add(reinterpret_cast<SyGenPool*>(self), obj,
                                            reinterpret_cast<const SyType*>(objType),
                                            reinterpret_cast<SyGenOwner*>(outGenRef)));
}

SY_API int sy_gen_owner_destroy_impl(void* self) {
    return static_cast<int>(sy_gen_owner_destroy(reinterpret_cast<SyGenOwner*>(self)));
}
