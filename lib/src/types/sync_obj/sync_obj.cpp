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

void sy::detail::syncObjDestroyHeldObjectCFunction(void* inner, void(*destruct)(void* ptr), const size_t alignType) {
    asObjMut(inner)->destroyHeldObjectCFunction(destruct, alignType);
}

const void* sy::detail::syncObjValueMem(const void* inner, const size_t alignType) {
    return asObj(inner)->valueMem(alignType);
}

void* sy::detail::syncObjValueMemMut(void* inner, const size_t alignType) {
    return asObjMut(inner)->valueMemMut(alignType);
}

bool sy::detail::syncObjNoWeakRefs(const void *inner)
{
    return asObj(inner)->noWeakRefs();
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
