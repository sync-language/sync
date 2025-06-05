//! API
#pragma once
#ifndef SY_TYPES_SYNC_OBJ_SYNC_OBJ_HPP_
#define SY_TYPES_SYNC_OBJ_SYNC_OBJ_HPP_

#include "../../core.h"
#include "../../threading/sync_queue.hpp"
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace sy {
    template<typename T>
    class Weak;

    namespace detail {
        class SY_API BaseSyncObj {
        public:
            void lockExclusive();

            bool tryLockExclusive();

            void unlockExclusive();

            void lockShared();

            bool tryLockShared();

            void unlockShared();

            operator sync_queue::SyncObject () const;
        protected:
            BaseSyncObj(void* inInner) : inner(inInner) {}

            void checkNotExpired();

            void* inner;
        };
    }

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
    template<typename T>
    class SY_API Owned final : public detail::BaseSyncObj {
    public:
        Owned(T value);

        Owned(const Owned&) = delete;
        
        Owned& operator=(const Owned&) = delete;

        Owned(Owned&& other);

        Owned& operator=(Owned&& other);

        ~Owned();

        const T* operator->() const { return this->get(); }

        T* operator->() { return this->get(); }

        const T& operator*() const { return *this->get(); }

        T& operator*() { return *this->get(); }

        const T* get() const;

        T* get();

        Weak<T> makeWeak() const;
    };

    template<typename T>
    class SY_API Shared final : public detail::BaseSyncObj {
    public:
        Shared(T value);

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
    };

    template<typename T>
    class SY_API Weak final : public detail::BaseSyncObj {
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
        /// if(weak.expired()) {
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

        friend class Owned<T>;
        friend class Shared<T>;

        Weak(const void* inInner);
    };

    namespace detail {
        void* syncObjCreate(const size_t sizeType, const size_t alignType);
        void syncObjDestroy(void* inner, const size_t sizeType);
        bool syncObjExpired(const void* inner);
        void syncObjAddWeakCount(void* inner);
        bool syncObjRemoveWeakCount(void* inner);
        void syncObjAddSharedCount(void* inner);
        bool syncObjRemoveSharedCount(void* inner);
        void syncObjDestroyHeldObjectCFunction(void* inner, void(*destruct)(void* ptr));
        const void* syncObjValueMem(const void* inner);
        void* syncObjValueMemMut(void* inner);
        bool syncObjNoWeakRefs(const void* inner);
        sync_queue::SyncObject syncObjToQueueObj(const void* inner);
        void syncObjDestroyAndFreeOwned(void* inner, void(*destruct)(void* ptr), const size_t sizeType);
        void syncObjDestroyAndFreeShared(void* inner, void(*destruct)(void* ptr), const size_t sizeType);
        void syncObjDestroyAndFreeWeak(void* inner, const size_t sizeType);
    }

    template <typename T>
    inline Owned<T>::Owned(Owned &&other)
        : BaseSyncObj(other.inner)
    {
        other.inner = nullptr;
    }

    template <typename T>
    inline Owned<T>& Owned<T>::operator=(Owned &&other)
    {
        if(this->inner != nullptr) {
            detail::syncObjDestroyAndFreeOwned(
            this->inner, 
            [](void* obj){
                T* asT = reinterpret_cast<T*>(obj);
                asT->~T();
            },
            sizeof(T));
        }

        this->inner = other.inner;
        other.inner = nullptr;
        return *this;
    }

    template <typename T>
    inline Owned<T>::~Owned()
    {
        if(this->inner == nullptr) {
            return;
        }

        detail::syncObjDestroyAndFreeOwned(
            this->inner, 
            [](void* obj){
                T* asT = reinterpret_cast<T*>(obj);
                asT->~T();
            },
            sizeof(T));

        this->inner = nullptr;
    }

    template <typename T>
    inline Owned<T>::Owned(T value)
        : BaseSyncObj(detail::syncObjCreate(sizeof(T), alignof(T)))
    {
        void* _ = new (detail::syncObjValueMemMut(this->inner)) T(std::move(value));
        (void)_; 
    }

    template<typename T>
    const T* Owned<T>::get() const 
    {
        const void* obj = detail::syncObjValueMem(this->inner);
        return reinterpret_cast<const T*>(obj);
    }

    template<typename T>
    T* Owned<T>::get()  
    {
        void* obj = detail::syncObjValueMemMut(this->inner);
        return reinterpret_cast<T*>(obj);
    }

    template <typename T>
    inline Weak<T> Owned<T>::makeWeak() const
    {
        return Weak<T>(this->inner);
    }

    template <typename T>
    inline Shared<T>::Shared(T value)
        : BaseSyncObj(detail::syncObjCreate(sizeof(T), alignof(T)))
    {
        detail::syncObjAddSharedCount(this->inner);
        void* _ = new (detail::syncObjValueMemMut(this->inner)) T(std::move(value));
        (void)_; 
    }

    template <typename T>
    inline Shared<T>::Shared(const Shared &other)
        : BaseSyncObj(other.inner)
    {
        detail::syncObjAddSharedCount(this->inner);
    }

    template <typename T>
    inline Shared<T>& Shared<T>::operator=(const Shared &other)
    {
        if(this->inner != nullptr) {   
            detail::syncObjDestroyAndFreeShared(
                this->inner, 
                [](void* obj){
                    T* asT = reinterpret_cast<T*>(obj);
                    asT->~T();
                },
                sizeof(T));
        }

        this->inner = other.inner;
        detail::syncObjAddSharedCount(this->inner);
        return *this;
    }

    template <typename T>
    inline Shared<T>::Shared(Shared &&other)
        : BaseSyncObj(other.inner)
    {
        other.inner = nullptr;
    }

    template <typename T>
    inline Shared<T>& Shared<T>::operator=(Shared &&other)
    {
        if(this->inner != nullptr) {    
            detail::syncObjDestroyAndFreeShared(
                this->inner, 
                [](void* obj){
                    T* asT = reinterpret_cast<T*>(obj);
                    asT->~T();
                },
                sizeof(T));
        }

        this->inner = other.inner;
        other.inner = nullptr;
        return *this;
    }

    template <typename T>
    inline Shared<T>::~Shared()
    {
        if(this->inner == nullptr) {
            return;
        }

        detail::syncObjDestroyAndFreeShared(
            this->inner, 
            [](void* obj){
                T* asT = reinterpret_cast<T*>(obj);
                asT->~T();
            },
            sizeof(T));

        this->inner = nullptr;
    }

    template<typename T>
    const T* Shared<T>::get() const 
    {
        const void* obj = detail::syncObjValueMem(this->inner);
        return reinterpret_cast<const T*>(obj);
    }

    template<typename T>
    T* Shared<T>::get()  
    {
        void* obj = detail::syncObjValueMemMut(this->inner);
        return reinterpret_cast<T*>(obj);
    }

    template <typename T>
    inline Weak<T> Shared<T>::makeWeak() const
    {
        return Weak<T>(this->inner);
    }

    template <typename T>
    inline Weak<T>::Weak(const Weak &other)
        : BaseSyncObj(other.inner)
    {
        detail::syncObjAddWeakCount(this->inner);
    }

    template <typename T>
    inline Weak<T>& Weak<T>::operator=(const Weak &other)
    {
        if(this->inner != nullptr) {
            detail::syncObjDestroyAndFreeWeak(this->inner, sizeof(T));
        }

        this->inner = other.inner;
        detail::syncObjAddWeakCount(this->inner);
        return *this;
    }

    template <typename T>
    inline Weak<T>::Weak(Weak &&other)
        : BaseSyncObj(other.inner)
    {
        other.inner = nullptr;
    }

    template <typename T>
    inline Weak<T>& Weak<T>::operator=(Weak &&other)
    {
        if(this->inner != nullptr) {
            detail::syncObjDestroyAndFreeWeak(this->inner, sizeof(T));
        }

        this->inner = other.inner;
        other.inner = nullptr;
        return *this;
    }

    template <typename T>
    inline Weak<T>::~Weak()
    {
        if(this->inner == nullptr) {
            return;
        }

        detail::syncObjDestroyAndFreeWeak(this->inner, sizeof(T));
        
        this->inner = nullptr;
    }

    template<typename T>
    bool Weak<T>::expired() const 
    {
        return detail::syncObjExpired(this->inner);
    }

    template<typename T>
    const T* Weak<T>::get() const 
    {
        this->checkNotExpired();
        const void* obj = detail::syncObjValueMem(this->inner);
        return reinterpret_cast<const T*>(obj);
    }

    template<typename T>
    T* Weak<T>::get()  
    {
        this->checkNotExpired();
        void* obj = detail::syncObjValueMemMut(this->inner);
        return reinterpret_cast<T*>(obj);
    }

    template<typename T>
    inline Weak<T>::Weak(const void* inInner) 
        : BaseSyncObj(const_cast<void*>(inInner))
    {
        detail::syncObjAddWeakCount(this->inner);
    }
}

#endif // SY_TYPES_SYNC_OBJ_SYNC_OBJ_HPP_