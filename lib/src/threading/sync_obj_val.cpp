#include "sync_obj_val.hpp"
#include "../mem/allocator.hpp"
#include "../types/type_info.hpp"
#include "../types/function/function.hpp"
#include "../program/program.hpp"
#include "../util/assert.hpp"
#include <cstring>
#include <new>

static_assert(alignof(SyncObjVal) == ALLOC_CACHE_ALIGN);

static size_t paddingForType(const size_t alignType)
{
    if(alignType <= ALLOC_CACHE_ALIGN) {
        return 0;
    }

    const size_t remainder = sizeof(SyncObjVal) % alignType;
    return alignType - remainder;
}

SyncObjVal *SyncObjVal::create(const size_t sizeType, const size_t alignType)
{
    const size_t allocAlign = alignType < ALLOC_CACHE_ALIGN ? ALLOC_CACHE_ALIGN : alignType;
    const size_t fullAllocSize = sizeof(SyncObjVal) + paddingForType(alignType) + sizeType;

    sy::Allocator alloc{};
    uint8_t* mem = alloc.allocAlignedArray<uint8_t>(fullAllocSize, allocAlign).get();
    SyncObjVal* self = reinterpret_cast<SyncObjVal*>(mem);
    (void)new (self) SyncObjVal;
    memset(self->valueMemMut(alignType), 0, sizeType);
    return self;
}

void SyncObjVal::destroy(const size_t sizeType, const size_t alignType)
{
    // assumes the held object's destructor has already been called

    const size_t allocAlign = alignType < ALLOC_CACHE_ALIGN ? ALLOC_CACHE_ALIGN : alignType;
    const size_t fullAllocSize = sizeof(SyncObjVal) + paddingForType(alignType) + sizeType;

    sy::Allocator alloc{};
    uint8_t* mem = reinterpret_cast<uint8_t*>(this);

    this->~SyncObjVal();
    alloc.freeAlignedArray(mem, fullAllocSize, allocAlign);
}

uintptr_t SyncObjVal::valueMemLocation(const size_t alignType) const
{
    const size_t memOffset = sizeof(SyncObjVal) + paddingForType(alignType);
    const uint8_t* asBytes = reinterpret_cast<const uint8_t*>(this);
    return reinterpret_cast<uintptr_t>(&asBytes[memOffset]);
}

SyncObjVal::SyncObjVal()
    : sharedCount(0), weakCount(0), isExpired(false)
{}

void SyncObjVal::addWeakCount()
{
    (void)this->weakCount.fetch_add(1);
}

bool SyncObjVal::removeWeakCount()
{
    return this->weakCount.fetch_sub(1) == 1;
}

void SyncObjVal::addSharedCount()
{
    (void)this->sharedCount.fetch_add(1);
}

bool SyncObjVal::removeSharedCount()
{
    return this->sharedCount.fetch_sub(1) == 1;
}

void SyncObjVal::destroyHeldObjectCFunction(void (*destruct)(void *ptr), const size_t alignType)
{
    this->isExpired.store(true);
    destruct(this->valueMemMut(alignType));
}

void SyncObjVal::destroyHeldObjectScriptFunction(const sy::Function *func, const sy::Type* typeInfo)
{
    this->isExpired.store(true);
    sy::Function::CallArgs callArgs = func->startCall();
    callArgs.push(this->valueMemMut(typeInfo->alignType), typeInfo->mutRef);
    const sy::ProgramRuntimeError err = callArgs.call(nullptr);
    sy_assert(err.ok(), "Destructors should not fail");
}

const void *SyncObjVal::valueMem(const size_t alignType) const
{
    return reinterpret_cast<const void*>(this->valueMemLocation(alignType));
}

void *SyncObjVal::valueMemMut(const size_t alignType)
{
    return reinterpret_cast<void*>(this->valueMemLocation(alignType));
}
