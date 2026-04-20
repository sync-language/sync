#include "gen_pool.hpp"
#include "../alloc_cache_align.hpp"
#include "gen_pool_internal.hpp"
#include <new>

using namespace sy;

GenPool::GenPool(GenPool&& other) noexcept : impl_(other.impl_) { other.impl_ = nullptr; }

GenPool& GenPool::operator=(GenPool&& other) noexcept {
    if (this != &other) {
        std::lock_guard lock(this->impl_->mutex);
        for (auto entry : this->impl_->typedPools) {
            entry.value->~GenTypedPool();
            this->impl_->allocator.freeObject(entry.value);
        }
        this->impl_ = other.impl_;
        other.impl_ = nullptr;
    }

    return *this;
}

sy::GenPool::~GenPool() noexcept {
    if (this->impl_ == nullptr) {
        return;
    }

    std::lock_guard lock(this->impl_->mutex);
    for (auto entry : this->impl_->typedPools) {
        entry.value->~GenTypedPool();
        this->impl_->allocator.freeObject(entry.value);
    }
    this->impl_ = nullptr;
}

Result<GenPool, AllocErr> GenPool::init(Allocator allocator) noexcept {
    auto implRes = allocator.allocAlignedObject<GenPoolImpl>(ALLOC_CACHE_ALIGN);
    if (implRes.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    GenPoolImpl* newImpl = implRes.value();
    new (newImpl) GenPoolImpl();
    GenPool self;
    self.impl_ = newImpl;
    return self;
}

Result<RawGenRef, AllocErr> sy::GenPool::rawAdd(void* obj, const Type* objType) noexcept {
    std::lock_guard lock(this->impl_->mutex);

    internal::GenTypedPool* typedPool = nullptr;
    if (auto found = this->impl_->typedPools.find(objType); !found.hasValue()) {
        auto poolRes = internal::GenTypedPool::init(objType, this->impl_->allocator);
        if (poolRes.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        typedPool = poolRes.value();

        auto mapRes = this->impl_->typedPools.insert(this->impl_->allocator, objType, typedPool);
        if (mapRes.hasErr()) {
            typedPool->~GenTypedPool();
            this->impl_->allocator.freeObject(typedPool);
            return Error(AllocErr::OutOfMemory);
        }
    } else {
        typedPool = found.value();
    }

    return typedPool->addObj(obj);
}
