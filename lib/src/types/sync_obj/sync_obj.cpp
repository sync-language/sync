#include "sync_obj.h"
#include "../../threading/sync_obj_val.hpp"
#include "../../util/assert.hpp"
#include "../type_info.h"
#include "../type_info.hpp"
#include "sync_obj.hpp"
#include <cstring>

using sy::SyncObjVal;

static const SyncObjVal* asObj(const void* inner) { return reinterpret_cast<const SyncObjVal*>(inner); }

static SyncObjVal* asObjMut(void* inner) { return reinterpret_cast<SyncObjVal*>(inner); }

static void syncObjLockExclusive(void* inner) { asObjMut(inner)->lockExclusive(); }

static bool syncObjTryLockExclusive(void* inner) { return asObjMut(inner)->tryLockExclusive(); }

static void syncObjUnlockExclusive(void* inner) { asObjMut(inner)->unlockExclusive(); }

static void syncObjLockShared(const void* inner) { asObj(inner)->lockShared(); }

static bool syncObjTryLockShared(const void* inner) { return asObj(inner)->tryLockShared(); }

static void syncObjUnlockShared(const void* inner) { asObj(inner)->unlockShared(); }

static const sy::sync_queue::SyncObject::VTable queueVTable = {&syncObjLockExclusive,   &syncObjTryLockExclusive,
                                                               &syncObjUnlockExclusive, &syncObjLockShared,
                                                               &syncObjTryLockShared,   &syncObjUnlockShared};

void sy::detail::syncObjDestroyAndFreeOwned(void* inner, void (*destruct)(void* ptr)) {
    bool shouldFree = false;

    syncObjLockExclusive(inner);
    {
        detail::syncObjDestroyHeldObjectCFunction(inner, destruct);
        shouldFree = detail::syncObjNoWeakRefs(inner);
    }
    syncObjUnlockExclusive(inner);

    if (shouldFree) {
        detail::syncObjDestroy(inner);
    }
}

static void syncObjDestroyAndFreeOwnedScript(void* inner, const sy::Type* typeInfo) {
    bool shouldFree = false;

    syncObjLockExclusive(inner);
    {
        asObjMut(inner)->destroyHeldObjectScriptFunction(typeInfo);
        shouldFree = sy::detail::syncObjNoWeakRefs(inner);
    }
    syncObjUnlockExclusive(inner);

    if (shouldFree) {
        sy::detail::syncObjDestroy(inner);
    }
}

void sy::detail::syncObjDestroyAndFreeShared(void* inner, void (*destruct)(void* ptr)) {
    const bool lastRef = detail::syncObjRemoveSharedCount(inner);
    if (!lastRef) {
        return;
    }

    bool shouldFree = false;

    syncObjLockExclusive(inner);
    {
        detail::syncObjDestroyHeldObjectCFunction(inner, destruct);
        shouldFree = detail::syncObjNoWeakRefs(inner);
    }
    syncObjUnlockExclusive(inner);

    if (shouldFree) {
        detail::syncObjDestroy(inner);
    }
}

static void syncObjDestroyAndFreeSharedScript(void* inner, const sy::Type* typeInfo) {
    const bool lastRef = sy::detail::syncObjRemoveSharedCount(inner);
    if (!lastRef) {
        return;
    }

    bool shouldFree = false;

    syncObjLockExclusive(inner);
    {
        asObjMut(inner)->destroyHeldObjectScriptFunction(typeInfo);
        shouldFree = sy::detail::syncObjNoWeakRefs(inner);
    }
    syncObjUnlockExclusive(inner);

    if (shouldFree) {
        sy::detail::syncObjDestroy(inner);
    }
}

void sy::detail::syncObjDestroyAndFreeWeak(void* inner) {
    const bool isExpired = syncObjExpired(inner);
    const bool isLastWeakRef = syncObjRemoveWeakCount(inner);

    if (isExpired && isLastWeakRef) {
        detail::syncObjDestroy(inner);
    }
}

extern "C" {
SY_API SyOwned sy_owned_init(void* value, const size_t sizeType, const size_t alignType) {
    SyOwned self = sy_owned_init_empty(sizeType, alignType);
    memcpy(sy::detail::syncObjValueMemMut(self.inner), value, sizeType);
    return self;
}

SY_API SyOwned sy_owned_init_empty(const size_t sizeType, const size_t alignType) {
    SyOwned self = {sy::detail::syncObjCreate(sy::Allocator(), sizeType, alignType).value()};
    return self;
}

SY_API SyOwned sy_owned_init_script_typed(void* value, const struct SyType* typeInfo) {
    return sy_owned_init(value, typeInfo->sizeType, typeInfo->alignType);
}

SY_API void sy_owned_destroy(SyOwned* self, void (*destruct)(void* ptr)) {
    if (self->inner == nullptr) {
        return;
    }

    sy::detail::syncObjDestroyAndFreeOwned(self->inner, destruct);

    self->inner = nullptr;
}

SY_API void sy_owned_destroy_script_typed(SyOwned* self, const struct SyType* typeInfo) {
    if (self->inner == nullptr) {
        return;
    }

    syncObjDestroyAndFreeOwnedScript(self->inner, reinterpret_cast<const sy::Type*>(typeInfo));

    self->inner = nullptr;
}

SY_API SyWeak sy_owned_make_weak(const SyOwned* self) {
    SyWeak weak = {const_cast<void*>(self->inner)};
    sy::detail::syncObjAddWeakCount(weak.inner);
    return weak;
}

SY_API void sy_owned_lock_exclusive(SyOwned* self) { syncObjLockExclusive(self->inner); }

SY_API bool sy_owned_try_lock_exclusive(SyOwned* self) { return syncObjTryLockExclusive(self->inner); }

SY_API void sy_owned_unlock_exclusive(SyOwned* self) { syncObjUnlockExclusive(self->inner); }

SY_API void sy_owned_lock_shared(const SyOwned* self) { syncObjLockShared(self->inner); }

SY_API bool sy_owned_try_lock_shared(const SyOwned* self) { return syncObjTryLockShared(self->inner); }

SY_API void sy_owned_unlock_shared(const SyOwned* self) { syncObjUnlockShared(self->inner); }

SY_API const void* sy_owned_get(const SyOwned* self) { return asObj(self->inner)->valueMem(); }

SY_API void* sy_owned_get_mut(SyOwned* self) { return asObjMut(self->inner)->valueMemMut(); }

SY_API SySyncObject sy_owned_to_queue_obj(const SyOwned* self) {
    const SySyncObject obj{const_cast<void*>(self->inner), reinterpret_cast<const SySyncObjectVTable*>(&queueVTable)};
    return obj;
}

SY_API SyShared sy_shared_init(void* value, const size_t sizeType, const size_t alignType) {
    SyShared self = sy_shared_init_empty(sizeType, alignType);
    memcpy(sy::detail::syncObjValueMemMut(self.inner), value, sizeType);
    return self;
}

SY_API SyShared sy_shared_init_empty(const size_t sizeType, const size_t alignType) {
    SyShared self = {sy::detail::syncObjCreate(sy::Allocator(), sizeType, alignType).value()};
    sy::detail::syncObjAddSharedCount(self.inner);
    return self;
}

SY_API SyShared sy_shared_init_script_typed(void* value, const struct SyType* typeInfo) {
    return sy_shared_init(value, typeInfo->sizeType, typeInfo->alignType);
}

SY_API SyShared sy_shared_clone(const SyShared* self) {
    SyShared newObj = {const_cast<void*>(self->inner)};
    sy::detail::syncObjAddSharedCount(newObj.inner);
    return newObj;
}

SY_API void sy_shared_destroy(SyShared* self, void (*destruct)(void* ptr)) {
    if (self->inner == nullptr) {
        return;
    }

    sy::detail::syncObjDestroyAndFreeShared(self->inner, destruct);

    self->inner = nullptr;
}

SY_API void sy_shared_destroy_script_typed(SyShared* self, const struct SyType* typeInfo) {
    if (self->inner == nullptr) {
        return;
    }

    syncObjDestroyAndFreeSharedScript(self->inner, reinterpret_cast<const sy::Type*>(typeInfo));

    self->inner = nullptr;
}

SY_API SyWeak sy_shared_make_weak(const SyShared* self) {
    SyWeak weak = {const_cast<void*>(self->inner)};
    sy::detail::syncObjAddWeakCount(weak.inner);
    return weak;
}

SY_API void sy_shared_lock_exclusive(SyShared* self) { syncObjLockExclusive(self->inner); }

SY_API bool sy_shared_try_lock_exclusive(SyShared* self) { return syncObjTryLockExclusive(self->inner); }

SY_API void sy_shared_unlock_exclusive(SyShared* self) { syncObjUnlockExclusive(self->inner); }

SY_API void sy_shared_lock_shared(const SyShared* self) { syncObjLockShared(self->inner); }

SY_API bool sy_shared_try_lock_shared(const SyShared* self) { return syncObjTryLockShared(self->inner); }

SY_API void sy_shared_unlock_shared(const SyShared* self) { syncObjUnlockShared(self->inner); }

SY_API const void* sy_shared_get(const SyShared* self) { return asObj(self->inner)->valueMem(); }

SY_API void* sy_shared_get_mut(SyShared* self) { return asObjMut(self->inner)->valueMemMut(); }

SY_API SySyncObject sy_shared_to_queue_obj(const SyShared* self) {
    const SySyncObject obj{const_cast<void*>(self->inner), reinterpret_cast<const SySyncObjectVTable*>(&queueVTable)};
    return obj;
}

SY_API SyWeak sy_weak_clone(const SyWeak* self) {
    SyWeak weak = *self; // make a copy
    sy::detail::syncObjAddWeakCount(weak.inner);
    return weak;
}

SY_API void sy_weak_lock_exclusive(SyWeak* self) { syncObjLockExclusive(self->inner); }

SY_API bool sy_weak_try_lock_exclusive(SyWeak* self) { return syncObjTryLockExclusive(self->inner); }

SY_API void sy_weak_unlock_exclusive(SyWeak* self) { syncObjUnlockExclusive(self->inner); }

SY_API void sy_weak_lock_shared(const SyWeak* self) { syncObjLockShared(self->inner); }

SY_API bool sy_weak_try_lock_shared(const SyWeak* self) { return syncObjTryLockShared(self->inner); }

SY_API void sy_weak_unlock_shared(const SyWeak* self) { syncObjUnlockShared(self->inner); }

SY_API bool sy_weak_expired(const SyWeak* self) { return asObj(self->inner)->expired(); }

SY_API const void* sy_weak_get(const SyWeak* self) { return asObj(self->inner)->valueMem(); }

SY_API void* sy_weak_get_mut(SyWeak* self) { return asObjMut(self->inner)->valueMemMut(); }

SY_API SySyncObject sy_weak_to_queue_obj(const SyWeak* self) {
    const SySyncObject obj{const_cast<void*>(self->inner), reinterpret_cast<const SySyncObjectVTable*>(&queueVTable)};
    return obj;
}
} // extern "C"

sy::Result<void*, sy::AllocErr> sy::detail::syncObjCreate(Allocator alloc, const size_t sizeType,
                                                          const size_t alignType) {
    auto res = SyncObjVal::create(alloc, sizeType, static_cast<uint16_t>(alignType));
    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }
    return reinterpret_cast<void*>(res.value());
}

void sy::detail::syncObjDestroy(void* inner) { asObjMut(inner)->destroy(); }

bool sy::detail::syncObjExpired(const void* inner) { return asObj(inner)->expired(); }

void sy::detail::syncObjAddWeakCount(void* inner) { asObjMut(inner)->addWeakCount(); }

bool sy::detail::syncObjRemoveWeakCount(void* inner) { return asObjMut(inner)->removeWeakCount(); }

void sy::detail::syncObjAddSharedCount(void* inner) { asObjMut(inner)->addSharedCount(); }

bool sy::detail::syncObjRemoveSharedCount(void* inner) { return asObjMut(inner)->removeSharedCount(); }

void sy::detail::syncObjDestroyHeldObjectCFunction(void* inner, void (*destruct)(void* ptr)) {
    asObjMut(inner)->destroyHeldObjectCFunction(destruct);
}

const void* sy::detail::syncObjValueMem(const void* inner) { return asObj(inner)->valueMem(); }

void* sy::detail::syncObjValueMemMut(void* inner) { return asObjMut(inner)->valueMemMut(); }

bool sy::detail::syncObjNoWeakRefs(const void* inner) { return asObj(inner)->noWeakRefs(); }

sy::sync_queue::SyncObject sy::detail::syncObjToQueueObj(const void* inner) {
    const sy::sync_queue::SyncObject obj{const_cast<void*>(inner), &queueVTable};
    return obj;
}

void sy::detail::BaseSyncObj::lockExclusive() { asObjMut(this->inner)->lockExclusive(); }

bool sy::detail::BaseSyncObj::tryLockExclusive() { return asObjMut(this->inner)->tryLockExclusive(); }

void sy::detail::BaseSyncObj::unlockExclusive() { asObjMut(this->inner)->unlockExclusive(); }

void sy::detail::BaseSyncObj::lockShared() const { asObj(this->inner)->lockShared(); }

bool sy::detail::BaseSyncObj::tryLockShared() const { return asObj(this->inner)->tryLockShared(); }

void sy::detail::BaseSyncObj::unlockShared() const { asObj(this->inner)->unlockShared(); }

sy::detail::BaseSyncObj::operator sy::sync_queue::SyncObject() const {
    const sy::sync_queue::SyncObject obj{const_cast<void*>(inner), &queueVTable};
    return obj;
}

void sy::detail::BaseSyncObj::checkNotExpired() const {
    sy_assert(!syncObjExpired(this->inner), "Held sync object is expired");
}

#if SYNC_LIB_WITH_TESTS

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

TEST_CASE("Owned make weak") {
    Owned<int> owned = 5;
    Weak<int> weak = owned.makeWeak();

    weak.lockExclusive();
    CHECK_FALSE(weak.expired());
    CHECK_EQ(*weak, 5);
    CHECK_EQ(weak.get(), owned.get());
    weak.unlockExclusive();
}

TEST_CASE("Owned make weak, then expire") {
    Owned<int>* owned = new Owned<int>(5);
    Weak<int> weak = owned->makeWeak();
    delete owned;

    weak.lockExclusive();
    CHECK(weak.expired());
    weak.unlockExclusive();
}

TEST_CASE("Shared into sync queue") {
    Shared<int> shared = 5;
    sync_queue::addExclusive(shared);
    sync_queue::lock();
    sync_queue::unlock();
}

TEST_CASE("Shared lock normally") {
    Shared<int> shared = 5;
    shared.lockExclusive();
    shared.unlockExclusive();
}

TEST_CASE("Shared make weak") {
    Shared<int> shared = 5;
    Weak<int> weak = shared.makeWeak();

    weak.lockExclusive();
    CHECK_FALSE(weak.expired());
    CHECK_EQ(*weak, 5);
    CHECK_EQ(weak.get(), shared.get());
    weak.unlockExclusive();
}

TEST_CASE("Shared make weak, then expire") {
    Shared<int>* shared = new Shared<int>(5);
    Weak<int> weak = shared->makeWeak();
    delete shared;

    weak.lockExclusive();
    CHECK(weak.expired());
    weak.unlockExclusive();
}

TEST_CASE("Shared clone") {
    Shared<int> s1 = 9;
    Shared<int> s2 = s1;

    s2.lockExclusive();

    CHECK_EQ(*s2, 9);
    CHECK_EQ(s1.get(), s2.get());

    s2.unlockExclusive();
}

TEST_CASE("Weak into sync queue") {
    Owned<int> owned = 9;
    Weak<int> weak = owned.makeWeak();

    sync_queue::addExclusive(weak);
    sync_queue::lock();
    sync_queue::unlock();
}

TEST_CASE("Weak clone") {
    Owned<int> owned = 9;
    Weak<int> w1 = owned.makeWeak();
    Weak<int> w2 = w1;

    w2.lockExclusive();

    CHECK_EQ(*w2, 9);
    CHECK_EQ(w1.get(), w2.get());

    w2.unlockExclusive();
}

#endif // SYNC_LIB_NO_TESTS
