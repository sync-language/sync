#include "sync_obj_val.hpp"
#include "../mem/allocator.hpp"
#include "../program/program.hpp"
#include "../types/function/function.hpp"
#include "../types/type_info.hpp"
#include "../util/assert.hpp"
#include <cstring>
#include <new>

using sy::SyncObjVal;

static_assert(alignof(SyncObjVal) == ALLOC_CACHE_ALIGN);

static size_t paddingForType(const size_t alignType) {
    if (alignType <= ALLOC_CACHE_ALIGN) {
        return 0;
    }

    const size_t remainder = sizeof(SyncObjVal) % alignType;
    return alignType - remainder;
}

sy::Result<SyncObjVal*, sy::AllocErr> SyncObjVal::create(Allocator alloc, const size_t inSizeType,
                                                         const uint16_t inAlignType) {
    sy_assert(inAlignType <= UINT16_MAX, "Type alignment too big");

    const size_t allocAlign = inAlignType < ALLOC_CACHE_ALIGN ? ALLOC_CACHE_ALIGN : inAlignType;
    const size_t fullAllocSize = sizeof(SyncObjVal) + paddingForType(inAlignType) + inSizeType;

    auto res = alloc.allocAlignedArray<uint8_t>(fullAllocSize, allocAlign);
    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    uint8_t* mem = res.value();
    SyncObjVal* self = reinterpret_cast<SyncObjVal*>(mem);
    (void)new (self) SyncObjVal(alloc, inSizeType, inAlignType);
    memset(self->valueMemMut(), 0, inSizeType);
    return self;
}

void SyncObjVal::destroy() {
    // assumes the held object's destructor has already been called

    const size_t allocAlign = alignType < ALLOC_CACHE_ALIGN ? ALLOC_CACHE_ALIGN : alignType;
    const size_t fullAllocSize = sizeof(SyncObjVal) + paddingForType(alignType) + this->sizeType;

    sy::Allocator alloc = this->allocator;
    uint8_t* mem = reinterpret_cast<uint8_t*>(this);

    this->~SyncObjVal();
    alloc.freeAlignedArray(mem, fullAllocSize, allocAlign);
}

uintptr_t SyncObjVal::valueMemLocation() const {
    const size_t memOffset = sizeof(SyncObjVal) + paddingForType(alignType);
    const uint8_t* asBytes = reinterpret_cast<const uint8_t*>(this);
    return reinterpret_cast<uintptr_t>(&asBytes[memOffset]);
}

SyncObjVal::SyncObjVal(Allocator alloc, size_t inSizeType, uint16_t inAlignType)
    : allocator(alloc), sizeType(inSizeType), sharedCount(0), weakCount(0), isExpired(false), alignType(inAlignType) {}

void SyncObjVal::addWeakCount() { (void)this->weakCount.fetch_add(1); }

bool SyncObjVal::removeWeakCount() { return this->weakCount.fetch_sub(1) == 1; }

void SyncObjVal::addSharedCount() { (void)this->sharedCount.fetch_add(1); }

bool SyncObjVal::removeSharedCount() { return this->sharedCount.fetch_sub(1) == 1; }

void SyncObjVal::destroyHeldObjectCFunction(void (*destruct)(void* ptr)) {
    void* value = this->valueMemMut();
    this->isExpired.store(true);
    destruct(value);
}

void SyncObjVal::destroyHeldObjectScriptFunction(const sy::Type* typeInfo) {
    sy_assert(typeInfo->alignType == this->alignType, "Type mismatch");
    if (typeInfo->destructor.hasValue() == false)
        return;
    this->isExpired.store(true);
    sy::RawFunction::CallArgs callArgs = typeInfo->destructor.value()->startCall();
    callArgs.push(this->valueMemMut(), typeInfo->mutRef);
    auto err = callArgs.call(nullptr);
    sy_assert(err.hasValue(), "Destructors should not fail");
}

const void* SyncObjVal::valueMem() const {
    sy_assert(this->isExpired.load() == false, "The weak referenced value is expired");
    return reinterpret_cast<const void*>(this->valueMemLocation());
}

void* SyncObjVal::valueMemMut() {
    sy_assert(this->isExpired.load() == false, "The weak referenced value is expired");
    return reinterpret_cast<void*>(this->valueMemLocation());
}
