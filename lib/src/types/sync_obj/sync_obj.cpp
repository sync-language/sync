#include "sync_obj.h"
#include "sync_obj.hpp"
#include "../../threading/sync_obj_val.hpp"

static const SyncObjVal* asObj(const void* inner) {
    return reinterpret_cast<const SyncObjVal*>(inner);
}

static SyncObjVal* asObjMut(void* inner) {
    return reinterpret_cast<SyncObjVal*>(inner);
}

void* sy::detail::syncObjCreate(const size_t sizeType, const size_t alignType) {
    return reinterpret_cast<void*>(SyncObjVal::create(sizeType, alignType));
}

void sy::detail::syncObjDestroy(void* inner, const size_t sizeType, const size_t alignType) {
    asObjMut(inner)->destroy(sizeType, alignType);
}

void sy::detail::syncObjLockExclusive(void* inner) {
    asObjMut(inner)->lockExclusive();
}

bool sy::detail::syncObjTryLockExclusive(void* inner) {
    return asObjMut(inner)->tryLockExclusive();
}

void sy::detail::syncObjUnlockExclusive(void* inner) {
    asObjMut(inner)->unlockExclusive();
}

void sy::detail::syncObjLockShared(const void* inner) {
    asObj(inner)->lockShared();
}

bool sy::detail::syncObjTryLockShared(const void* inner) {
    return asObj(inner)->tryLockShared();
}

void sy::detail::syncObjUnlockShared(const void* inner) {
    asObj(inner)->unlockShared();
}

bool sy::detail::syncObjExpired(const void* inner) {
    asObj(inner)->expired();
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

void sy::detail::syncObjDestroyHeldObjectCFunction(void* inner, void(*destruct)(void* ptr), const size_t alignType) {
    asObjMut(inner)->destroyHeldObjectCFunction(destruct, alignType);
}

const void* sy::detail::syncObjValueMem(const void* inner, const size_t alignType) {
    asObj(inner)->valueMem(alignType);
}

void* sy::detail::syncObjValueMemMut(void* inner, const size_t alignType) {
    asObjMut(inner)->valueMemMut(alignType);
}

bool sy::detail::syncObjNoWeakRefs(const void *inner)
{
    return asObj(inner)->noWeakRefs();
}
