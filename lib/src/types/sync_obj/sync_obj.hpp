//! API
#ifndef SY_TYPES_SYNC_OBJ_SYNC_OBJ_HPP_
#define SY_TYPES_SYNC_OBJ_SYNC_OBJ_HPP_

#include "../../core.h"
#include "../type_info.hpp"
#include "../../threading/sync_queue.hpp"
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace sy {
    template<typename T>
    class Weak;

    namespace detail {
        class BaseSyncObj {
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
    };

    template<typename T>
    class SY_API Weak final : detail::BaseSyncObj {
    public:
        Weak(const Weak& other);

        Weak& operator=(const Weak& other);

        Weak(Weak&& other);

        Weak& operator=(Weak&& other);

        ~Weak();

    private:

        friend class Owned<T>;
        friend class Shared<T>;

        Weak(const void* inInner);
    };

    namespace detail {
        void* syncObjCreate(const size_t sizeType, const size_t alignType);
        void syncObjDestroy(void* inner, const size_t sizeType, const size_t alignType);
        bool syncObjExpired(const void* inner);
        void syncObjAddWeakCount(void* inner);
        bool syncObjRemoveWeakCount(void* inner);
        void syncObjAddSharedCount(void* inner);
        bool syncObjRemoveSharedCount(void* inner);
        void syncObjDestroyHeldObjectCFunction(void* inner, void(*destruct)(void* ptr), const size_t alignType);
        const void* syncObjValueMem(const void* inner, const size_t alignType);
        void* syncObjValueMemMut(void* inner, const size_t alignType);
        bool syncObjNoWeakRefs(const void* inner);
        sync_queue::SyncObject syncObjToQueueObj(const void* inner);
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
            bool shouldFree = false;

            this->lockExclusive();
            {
                detail::syncObjDestroyHeldObjectCFunction(
                this->inner, 
                [](void* obj){
                    T* asT = reinterpret_cast<T*>(obj);
                    asT->~T();
                }, 
                alignof(T));

                shouldFree = detail::syncObjNoWeakRefs(this->inner);
            }
            this->unlockExclusive();

            if(shouldFree) {
                detail::syncObjDestroy(this->inner, sizeof(T), alignof(T));
            }
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

        bool shouldFree = false;

        this->lockExclusive();
        {
            detail::syncObjDestroyHeldObjectCFunction(
            this->inner, 
            [](void* obj){
                T* asT = reinterpret_cast<T*>(obj);
                asT->~T();
            }, 
            alignof(T));

            shouldFree = detail::syncObjNoWeakRefs(this->inner);
        }
        this->unlockExclusive();

        if(shouldFree) {
            detail::syncObjDestroy(this->inner, sizeof(T), alignof(T));
        }

        this->inner = nullptr;
    }

    template <typename T>
    inline Owned<T>::Owned(T value)
        : BaseSyncObj(detail::syncObjCreate(sizeof(T), alignof(T)))
    {
        void* _ = new (detail::syncObjValueMemMut(this->inner, alignof(T))) T(std::move(value));
        (void)_; 
    }

    template<typename T>
    const T* Owned<T>::get() const 
    {
        const void* obj = detail::syncObjValueMem(this->inner, alignof(T));
        return reinterpret_cast<const T*>(obj);
    }

    template<typename T>
    T* Owned<T>::get()  
    {
        void* obj = detail::syncObjValueMemMut(this->inner, alignof(T));
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
        void* _ = new (detail::syncObjValueMemMut(this->inner, alignof(T))) T(std::move(value));
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
            bool shouldFree = false;

            this->lockExclusive();
            {
                detail::syncObjDestroyHeldObjectCFunction(
                this->inner, 
                [](void* obj){
                    T* asT = reinterpret_cast<T*>(obj);
                    asT->~T();
                }, 
                alignof(T));

                shouldFree = detail::syncObjNoWeakRefs(this->inner);
            }
            this->unlockExclusive();

            if(shouldFree) {
                detail::syncObjDestroy(this->inner, sizeof(T), alignof(T));
            }
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
            bool shouldFree = false;

            this->lockExclusive();
            {
                detail::syncObjDestroyHeldObjectCFunction(
                this->inner, 
                [](void* obj){
                    T* asT = reinterpret_cast<T*>(obj);
                    asT->~T();
                }, 
                alignof(T));

                shouldFree = detail::syncObjNoWeakRefs(this->inner);
            }
            this->unlockExclusive();

            if(shouldFree) {
                detail::syncObjDestroy(this->inner, sizeof(T), alignof(T));
            }
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

        const bool lastRef = detail::syncObjRemoveSharedCount(this->inner);
        if(!lastRef) {
            this->inner = nullptr;
            return;
        }
        
        bool shouldFree = false;

        this->lockExclusive();
        {
            detail::syncObjDestroyHeldObjectCFunction(
            this->inner, 
            [](void* obj){
                T* asT = reinterpret_cast<T*>(obj);
                asT->~T();
            }, 
            alignof(T));

            shouldFree = detail::syncObjNoWeakRefs(this->inner);
        }
        this->unlockExclusive();

        if(shouldFree) {
            detail::syncObjDestroy(this->inner, sizeof(T), alignof(T));
        }

        this->inner = nullptr;
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
            const bool isExpired = detail::syncObjExpired(this->inner);
            const bool isLastWeakRef = detail::syncObjRemoveWeakCount(this->inner);

            if(isExpired && isLastWeakRef) {
                detail::syncObjDestroy(this->inner, sizeof(T), alignof(T));
            }
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
            const bool isExpired = detail::syncObjExpired(this->inner);
            const bool isLastWeakRef = detail::syncObjRemoveWeakCount(this->inner);

            if(isExpired && isLastWeakRef) {
                detail::syncObjDestroy(this->inner, sizeof(T), alignof(T));
            }
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

        const bool isExpired = detail::syncObjExpired(this->inner);
        const bool isLastWeakRef = detail::syncObjRemoveWeakCount(this->inner);

        if(isExpired && isLastWeakRef) {
            detail::syncObjDestroy(this->inner, sizeof(T), alignof(T));
        }
        
        this->inner = nullptr;
    }

    template<typename T>
    inline Weak<T>::Weak(const void* inInner) 
        : BaseSyncObj(const_cast<void*>(inInner))
    {
        detail::syncObjAddWeakCount(this->inner);
    }
}

#endif // SY_TYPES_SYNC_OBJ_SYNC_OBJ_HPP_