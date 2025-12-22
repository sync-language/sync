//! API
#pragma once
#ifndef SY_TYPES_SYNC_OBJ_SYNC_OBJ_HPP_
#define SY_TYPES_SYNC_OBJ_SYNC_OBJ_HPP_

#include "../../core.h"
#include "../../mem/allocator.hpp"
#include "../../threading/sync_queue.hpp"
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace sy {
template <typename T> class Weak;

namespace detail {
class SY_API BaseSyncObj {
  public:
    void lockExclusive();

    bool tryLockExclusive();

    void unlockExclusive();

    void lockShared() const;

    bool tryLockShared() const;

    void unlockShared() const;

    operator sync_queue::SyncObject() const;

  protected:
    BaseSyncObj(void* inInner) noexcept : inner(inInner) {}

    void checkNotExpired() const;

    void* inner;
};
} // namespace detail

/// Synchronized thread-safe RAII object, supporting weak references and single ownership.
/// @tparam T Type of the held object
///
/// # Sync Queue Usage
///
/// ``` .cpp
/// sy::Owned<int> owned = 5;
/// sy::sync_queue::addExclusive(owned); // conversion operator
/// sy::sync_queue::lock();
///
/// *owned += 5;
/// assert(*owned == 10);
///
/// sy::sync_queue::unlock();
/// ```
///
/// # Outside of Sync Queue Usage
///
/// ``` .cpp
///
/// ```
template <typename T> class SY_API Unique final : public detail::BaseSyncObj {
  public:
    Unique(T value);

    Result<Unique, AllocErr> init(Allocator alloc, T value);

    Unique(const Unique&) = delete;

    Unique& operator=(const Unique&) = delete;

    Unique(Unique&& other);

    Unique& operator=(Unique&& other);

    ~Unique();

    const T* operator->() const { return this->get(); }

    T* operator->() { return this->get(); }

    const T& operator*() const { return *this->get(); }

    T& operator*() { return *this->get(); }

    const T* get() const;

    T* get();

    Weak<T> makeWeak() const;

    // TODO maybe promote to shared?

  private:
    Unique(void* p) : BaseSyncObj(p) {}
};

template <typename T> class SY_API Shared final : public detail::BaseSyncObj {
  public:
    Shared(T value);

    Result<Shared, AllocErr> init(Allocator alloc, T value);

    Shared(const Shared& other);

    Shared& operator=(const Shared& other);

    Shared(Shared&& other);

    Shared& operator=(Shared&& other);

    ~Shared();

    const T* operator->() const { return this->get(); }

    T* operator->() { return this->get(); }

    const T& operator*() const { return *this->get(); }

    T& operator*() { return *this->get(); }

    const T* get() const;

    T* get();

    Weak<T> makeWeak() const;

  private:
    Shared(void* p) : BaseSyncObj(p) {}
};

template <typename T> class SY_API Weak final : public detail::BaseSyncObj {
  public:
    Weak(const Weak& other);

    Weak& operator=(const Weak& other);

    Weak(Weak&& other);

    Weak& operator=(Weak&& other);

    ~Weak();

    /// After acquiring a lock, it's still possible that the held object itself has been destroyed.
    /// ``` .cpp
    /// sy::Weak<int> weak = owned->makeWeak();
    /// // ... Stuff happens
    /// weak.lockExclusive();
    /// if(!weak.expired()) {
    ///     *weak += 5;
    /// }
    /// weak.unlockExclusive();
    /// ```
    bool expired() const;

    /// # Debug Asserts
    /// `this->expired() == false`
    const T* operator->() const { return this->get(); }

    /// # Debug Asserts
    /// `this->expired() == false`
    T* operator->() { return this->get(); }

    /// # Debug Asserts
    /// `this->expired() == false`
    const T& operator*() const { return *this->get(); }

    /// # Debug Asserts
    /// `this->expired() == false`
    T& operator*() { return *this->get(); }

    /// # Debug Asserts
    /// `this->expired() == false`
    const T* get() const;

    /// # Debug Asserts
    /// `this->expired() == false`
    T* get();

  private:
    friend class Unique<T>;
    friend class Shared<T>;

    Weak(const void* inInner);
};

namespace detail {
Result<void*, AllocErr> syncObjCreate(Allocator alloc, const size_t sizeType, const size_t alignType);
void syncObjDestroy(void* inner);
bool syncObjExpired(const void* inner);
void syncObjAddWeakCount(void* inner);
bool syncObjRemoveWeakCount(void* inner);
void syncObjAddSharedCount(void* inner);
bool syncObjRemoveSharedCount(void* inner);
void syncObjDestroyHeldObjectCFunction(void* inner, void (*destruct)(void* ptr));
const void* syncObjValueMem(const void* inner);
void* syncObjValueMemMut(void* inner);
bool syncObjNoWeakRefs(const void* inner);
sync_queue::SyncObject syncObjToQueueObj(const void* inner);
void syncObjDestroyAndFreeOwned(void* inner, void (*destruct)(void* ptr));
void syncObjDestroyAndFreeShared(void* inner, void (*destruct)(void* ptr));
void syncObjDestroyAndFreeWeak(void* inner);
} // namespace detail

template <typename T> inline Unique<T>::Unique(Unique&& other) : BaseSyncObj(other.inner) { other.inner = nullptr; }

template <typename T> inline Unique<T>& Unique<T>::operator=(Unique&& other) {
    if (this->inner != nullptr) {
        detail::syncObjDestroyAndFreeOwned(
            this->inner,
            [](void* obj) {
                T* asT = reinterpret_cast<T*>(obj);
                asT->~T();
            },
            sizeof(T));
    }

    this->inner = other.inner;
    other.inner = nullptr;
    return *this;
}

template <typename T> inline Unique<T>::~Unique() {
    if (this->inner == nullptr) {
        return;
    }

    detail::syncObjDestroyAndFreeOwned(this->inner, [](void* obj) {
        T* asT = reinterpret_cast<T*>(obj);
        asT->~T();
    });

    this->inner = nullptr;
}

template <typename T>
inline Unique<T>::Unique(T value) : BaseSyncObj(detail::syncObjCreate(Allocator(), sizeof(T), alignof(T)).value()) {
    new (detail::syncObjValueMemMut(this->inner)) T(std::move(value));
}

template <typename T> inline Result<Unique<T>, AllocErr> Unique<T>::init(Allocator alloc, T value) {
    auto res = detail::syncObjCreate(alloc, sizeof(T), alignof(T));
    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }
    new (detail::syncObjValueMemMut(res.value())) T(std::move(value));
    return Unique(res.value());
}

template <typename T> const T* Unique<T>::get() const {
    const void* obj = detail::syncObjValueMem(this->inner);
    return reinterpret_cast<const T*>(obj);
}

template <typename T> T* Unique<T>::get() {
    void* obj = detail::syncObjValueMemMut(this->inner);
    return reinterpret_cast<T*>(obj);
}

template <typename T> inline Weak<T> Unique<T>::makeWeak() const { return Weak<T>(this->inner); }

template <typename T>
inline Shared<T>::Shared(T value) : BaseSyncObj(detail::syncObjCreate(Allocator(), sizeof(T), alignof(T)).value()) {
    detail::syncObjAddSharedCount(this->inner);
    new (detail::syncObjValueMemMut(this->inner)) T(std::move(value));
}

template <typename T> inline Result<Shared<T>, AllocErr> Shared<T>::init(Allocator alloc, T value) {
    auto res = detail::syncObjCreate(alloc, sizeof(T), alignof(T));
    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }
    new (detail::syncObjValueMemMut(res.value())) T(std::move(value));
    return Shared(res.value());
}

template <typename T> inline Shared<T>::Shared(const Shared& other) : BaseSyncObj(other.inner) {
    detail::syncObjAddSharedCount(this->inner);
}

template <typename T> inline Shared<T>& Shared<T>::operator=(const Shared& other) {
    if (this->inner != nullptr) {
        detail::syncObjDestroyAndFreeShared(
            this->inner,
            [](void* obj) {
                T* asT = reinterpret_cast<T*>(obj);
                asT->~T();
            },
            sizeof(T));
    }

    this->inner = other.inner;
    detail::syncObjAddSharedCount(this->inner);
    return *this;
}

template <typename T> inline Shared<T>::Shared(Shared&& other) : BaseSyncObj(other.inner) { other.inner = nullptr; }

template <typename T> inline Shared<T>& Shared<T>::operator=(Shared&& other) {
    if (this->inner != nullptr) {
        detail::syncObjDestroyAndFreeShared(
            this->inner,
            [](void* obj) {
                T* asT = reinterpret_cast<T*>(obj);
                asT->~T();
            },
            sizeof(T));
    }

    this->inner = other.inner;
    other.inner = nullptr;
    return *this;
}

template <typename T> inline Shared<T>::~Shared() {
    if (this->inner == nullptr) {
        return;
    }

    detail::syncObjDestroyAndFreeShared(this->inner, [](void* obj) {
        T* asT = reinterpret_cast<T*>(obj);
        asT->~T();
    });

    this->inner = nullptr;
}

template <typename T> const T* Shared<T>::get() const {
    const void* obj = detail::syncObjValueMem(this->inner);
    return reinterpret_cast<const T*>(obj);
}

template <typename T> T* Shared<T>::get() {
    void* obj = detail::syncObjValueMemMut(this->inner);
    return reinterpret_cast<T*>(obj);
}

template <typename T> inline Weak<T> Shared<T>::makeWeak() const { return Weak<T>(this->inner); }

template <typename T> inline Weak<T>::Weak(const Weak& other) : BaseSyncObj(other.inner) {
    detail::syncObjAddWeakCount(this->inner);
}

template <typename T> inline Weak<T>& Weak<T>::operator=(const Weak& other) {
    if (this->inner != nullptr) {
        detail::syncObjDestroyAndFreeWeak(this->inner);
    }

    this->inner = other.inner;
    detail::syncObjAddWeakCount(this->inner);
    return *this;
}

template <typename T> inline Weak<T>::Weak(Weak&& other) : BaseSyncObj(other.inner) { other.inner = nullptr; }

template <typename T> inline Weak<T>& Weak<T>::operator=(Weak&& other) {
    if (this->inner != nullptr) {
        detail::syncObjDestroyAndFreeWeak(this->inner);
    }

    this->inner = other.inner;
    other.inner = nullptr;
    return *this;
}

template <typename T> inline Weak<T>::~Weak() {
    if (this->inner == nullptr) {
        return;
    }

    detail::syncObjDestroyAndFreeWeak(this->inner);

    this->inner = nullptr;
}

template <typename T> bool Weak<T>::expired() const { return detail::syncObjExpired(this->inner); }

template <typename T> const T* Weak<T>::get() const {
    this->checkNotExpired();
    const void* obj = detail::syncObjValueMem(this->inner);
    return reinterpret_cast<const T*>(obj);
}

template <typename T> T* Weak<T>::get() {
    this->checkNotExpired();
    void* obj = detail::syncObjValueMemMut(this->inner);
    return reinterpret_cast<T*>(obj);
}

template <typename T> inline Weak<T>::Weak(const void* inInner) : BaseSyncObj(const_cast<void*>(inInner)) {
    detail::syncObjAddWeakCount(this->inner);
}
} // namespace sy

#endif // SY_TYPES_SYNC_OBJ_SYNC_OBJ_HPP_