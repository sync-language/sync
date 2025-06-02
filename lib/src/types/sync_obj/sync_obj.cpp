#include "sync_obj.h"
#include "sync_obj.hpp"
#include "../type_info.h"
#include "../type_info.hpp"
#include "../../threading/sync_obj_val.hpp"
#include "../../util/assert.hpp"
#include <cstring>

static const SyncObjVal* asObj(const void* inner) {
    return reinterpret_cast<const SyncObjVal*>(inner);
}

static SyncObjVal* asObjMut(void* inner) {
    return reinterpret_cast<SyncObjVal*>(inner);
}

static void syncObjLockExclusive(void* inner) {
    asObjMut(inner)->lockExclusive();
}

static bool syncObjTryLockExclusive(void* inner) {
    return asObjMut(inner)->tryLockExclusive();
}

static void syncObjUnlockExclusive(void* inner) {
    asObjMut(inner)->unlockExclusive();
}

static void syncObjLockShared(const void* inner) {
    asObj(inner)->lockShared();
}

static bool syncObjTryLockShared(const void* inner) {
    return asObj(inner)->tryLockShared();
}

static void syncObjUnlockShared(const void* inner) {
    asObj(inner)->unlockShared();
}

static const sy::sync_queue::SyncObject::VTable queueVTable = {
    &syncObjLockExclusive,
    &syncObjTryLockExclusive,
    &syncObjUnlockExclusive,
    &syncObjLockShared,
    &syncObjTryLockShared,
    &syncObjUnlockShared
};

extern "C" {
    SyOwned sy_owned_init(void* value, const size_t sizeType, const size_t alignType) 
    {
        SyOwned self = {sy::detail::syncObjCreate(sizeType, alignType)};
        memcpy(sy::detail::syncObjValueMemMut(self.inner), value, sizeType);
        return self;
    }

    SyOwned sy_owned_init_script_typed(void* value, const struct SyType* typeInfo) 
    {
        return sy_owned_init(value, typeInfo->sizeType, typeInfo->alignType);
    }

    void sy_owned_destroy(SyOwned* self, void (*destruct)(void* ptr), const size_t sizeType) 
    {
        if(self->inner == nullptr) {
            return;
        }

        bool shouldFree = false;

        sy_owned_lock_exclusive(self);
        {
            sy::detail::syncObjDestroyHeldObjectCFunction(self->inner, destruct);

            shouldFree = sy::detail::syncObjNoWeakRefs(self->inner);
        }
        sy_owned_unlock_exclusive(self);

        if(shouldFree) {
            sy::detail::syncObjDestroy(self->inner, sizeType);
        }

        self->inner = nullptr; 
    }

    void sy_owned_destroy_script_typed(SyOwned* self, const struct SyType* typeInfo) 
    {
        if(self->inner == nullptr) {
            return;
        }

        bool shouldFree = false;

        sy_owned_lock_exclusive(self);
        {
            if(typeInfo->optionalDestructor != nullptr) {
                asObjMut(self->inner)->destroyHeldObjectScriptFunction(reinterpret_cast<const sy::Type*>(typeInfo));
            }
            shouldFree = sy::detail::syncObjNoWeakRefs(self->inner);
        }
        sy_owned_unlock_exclusive(self);

        if(shouldFree) {
            sy::detail::syncObjDestroy(self->inner, typeInfo->sizeType);
        }

        self->inner = nullptr; 
    }

    void sy_owned_lock_exclusive(SyOwned* self) 
    {
        syncObjLockExclusive(self->inner);
    }

    bool sy_owned_try_lock_exclusive(SyOwned* self) 
    {
        return syncObjTryLockExclusive(self->inner);
    }

    void sy_owned_unlock_exclusive(SyOwned* self) 
    {
        syncObjUnlockExclusive(self->inner);
    }

    void sy_owned_lock_shared(const SyOwned* self) 
    {
        syncObjLockShared(self->inner);
    }

    bool sy_owned_try_lock_shared(const SyOwned* self) 
    {
        return syncObjTryLockShared(self->inner);
    }

    void sy_owned_unlock_shared(const SyOwned* self) 
    {
        syncObjUnlockShared(self->inner);
    }

    const void* sy_owned_get(const SyOwned* self) 
    {
        return asObj(self->inner)->valueMem();
    }

    void* sy_owned_get_mut(SyOwned* self) 
    {
        return asObjMut(self->inner)->valueMemMut();
    }

    SySyncObject sy_owned_to_queue_obj(const SyOwned* self) 
    {
        const SySyncObject obj{const_cast<void*>(self->inner), reinterpret_cast<const SySyncObjectVTable*>(&queueVTable)};
        return obj;
    }
} // extern "C"

void* sy::detail::syncObjCreate(const size_t sizeType, const size_t alignType) {
    return reinterpret_cast<void*>(SyncObjVal::create(sizeType, alignType));
}

void sy::detail::syncObjDestroy(void* inner, const size_t sizeType) {
    asObjMut(inner)->destroy(sizeType);
}

bool sy::detail::syncObjExpired(const void* inner) {
    return asObj(inner)->expired();
}

void sy::detail::syncObjAddWeakCount(void* inner) {
    asObjMut(inner)->addWeakCount();
}

bool sy::detail::syncObjRemoveWeakCount(void* inner) {
    return asObjMut(inner)->removeWeakCount();
}

void sy::detail::syncObjAddSharedCount(void* inner) {
    asObjMut(inner)->addSharedCount();
}

bool sy::detail::syncObjRemoveSharedCount(void* inner) {
    return asObjMut(inner)->removeSharedCount();
}

void sy::detail::syncObjDestroyHeldObjectCFunction(void* inner, void(*destruct)(void* ptr)) {
    asObjMut(inner)->destroyHeldObjectCFunction(destruct);
}

const void* sy::detail::syncObjValueMem(const void* inner) {
    return asObj(inner)->valueMem();
}

void* sy::detail::syncObjValueMemMut(void* inner) {
    return asObjMut(inner)->valueMemMut();
}

bool sy::detail::syncObjNoWeakRefs(const void *inner)
{
    return asObj(inner)->noWeakRefs();
}

sy::sync_queue::SyncObject sy::detail::syncObjToQueueObj(const void *inner)
{
    const sy::sync_queue::SyncObject obj{const_cast<void*>(inner), &queueVTable};
    return obj;
}

void sy::detail::BaseSyncObj::lockExclusive() 
{
    asObjMut(this->inner)->lockExclusive();
}

bool sy::detail::BaseSyncObj::tryLockExclusive() {
    return asObjMut(this->inner)->tryLockExclusive();
}

void sy::detail::BaseSyncObj::unlockExclusive() {
    asObjMut(this->inner)->unlockExclusive();
}

void sy::detail::BaseSyncObj::lockShared() {
    asObj(this->inner)->lockShared();
}

bool sy::detail::BaseSyncObj::tryLockShared() {
    return asObj(this->inner)->tryLockShared();
}

void sy::detail::BaseSyncObj::unlockShared() {
    asObj(this->inner)->unlockShared();
}

sy::detail::BaseSyncObj::operator sync_queue::SyncObject () const 
{
    const sy::sync_queue::SyncObject obj{const_cast<void*>(inner), &queueVTable};
    return obj;
}

void sy::detail::BaseSyncObj::checkNotExpired()
{
    sy_assert(!syncObjExpired(this->inner), "Held sync object is expired");
}

#if SYNC_LIB_TEST

#include "../../doctest.h"

using namespace sy;

TEST_CASE("Owned into sync queue") {
    Owned<int> owned = 5;
    sync_queue::addExclusive(owned);
    sync_queue::lock();
    sync_queue::unlock();
}

TEST_CASE("Owned lock normally") {
    Owned<int> owned = 5;
    owned.lockExclusive();
    owned.unlockExclusive();
}

#endif
