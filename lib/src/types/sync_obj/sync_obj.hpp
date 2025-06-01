//! API
#ifndef SY_TYPES_SYNC_OBJ_SYNC_OBJ_HPP_
#define SY_TYPES_SYNC_OBJ_SYNC_OBJ_HPP_

#include "../../core.h"
#include "../type_info.hpp"
#include "../../threading/sync_queue.hpp"
#include <cstddef>
#include <new>
#include <type_traits>

namespace sy {
    template<typename T>
    class Weak;

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
    /// owned.get() += 5;
    /// assert(owned.get() == 10);
    ///
    /// sy::sync_queue::unlock();
    /// ```
    template<typename T>
    class SY_API Owned final {
    public:
        Owned() = default; // TODO is there an edge case here?

        ~Owned();

        Owned(T value);

        void lockExclusive();

        bool tryLockExclusive();

        void unlockExclusive();

        void lockShared();

        bool tryLockShared();

        void unlockShared();

        const T& get() const;

        T& get();

        //Weak<T> makeWeak() const;

        operator sync_queue::SyncObject () const;

    private:
        void* inner = nullptr;
    };

    template<typename T>
    class SY_API Shared final {
    public:
        Shared() = default;

        ~Shared();

    private:
        void* inner = nullptr;
    };

    template<typename T>
    class SY_API Weak final {
    public:
        Weak() = default;

        ~Weak();

    private:
        void* inner = nullptr;
    };

    namespace detail {
        void* syncObjCreate(const size_t sizeType, const size_t alignType);
        void syncObjDestroy(void* inner, const size_t sizeType, const size_t alignType);
        void syncObjLockExclusive(void* inner);
        bool syncObjTryLockExclusive(void* inner);
        void syncObjUnlockExclusive(void* inner);
        void syncObjLockShared(const void* inner);
        bool syncObjTryLockShared(const void* inner);
        void syncObjUnlockShared(const void* inner);
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
    inline Owned<T>::~Owned()
    {
        if(this->inner == nullptr) {
            return;
        }

        bool shouldFree = false;

        detail::syncObjLockExclusive(this->inner);
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
        detail::syncObjUnlockExclusive(this->inner);

        if(shouldFree) {
            detail::syncObjDestroy(this->inner, sizeof(T), alignof(T));
        }

        this->inner = nullptr;
    }

    template <typename T>
    inline Owned<T>::Owned(T value)
        : inner(detail::syncObjCreate(sizeof(T), alignof(T)))
    {
        void* _ = new (detail::syncObjValueMemMut(this->inner, alignof(T))) T(std::move(value));
        (void)_; 
    }

    template<typename T>
    inline void Owned<T>::lockExclusive() 
    {
        detail::syncObjLockExclusive(this->inner);
    }

    template<typename T>
    inline bool Owned<T>::tryLockExclusive() 
    {
        return detail::syncObjTryLockExclusive(this->inner);
    }

    template<typename T>
    inline void Owned<T>::unlockExclusive() 
    {
        detail::syncObjUnlockExclusive(this->inner);
    }

    template<typename T>
    inline void Owned<T>::lockShared() 
    {
        detail::syncObjLockShared(this->inner);
    }

    template<typename T>
    inline bool Owned<T>::tryLockShared() 
    {
        return detail::syncObjTryLockShared(this->inner);
    }

    template<typename T>
    inline void Owned<T>::unlockShared() 
    {
        detail::syncObjUnlockShared(this->inner);
    }

    template<typename T>
    const T& Owned<T>::get() const 
    {
        const void* obj = detail::syncObjValueMem(this->inner, alignof(T));
        return *reinterpret_cast<const T*>(obj);
    }

    template<typename T>
    T& Owned<T>::get()  
    {
        void* obj = detail::syncObjValueMemMut(this->inner, alignof(T));
        return *reinterpret_cast<T*>(obj);
    }

    template <typename T>
    inline Owned<T>::operator sync_queue::SyncObject() const
    {
        return detail::syncObjToQueueObj(this->inner);
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

        detail::syncObjLockExclusive(this->inner);
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
        detail::syncObjUnlockExclusive(this->inner);

        if(shouldFree) {
            detail::syncObjDestroy(this->inner, sizeof(T), alignof(T));
        }

        this->inner = nullptr;
    }

    template <typename T>
    inline Weak<T>::~Weak()
    {
        if(this->inner = nullptr) {
            return;
        }

        const bool isExpired = detail::syncObjExpired(this->inner);
        const bool isLastWeakRef = detail::syncObjRemoveWeakCount(this->inner);

        if(isExpired && isLastWeakRef) {
            detail::syncObjDestroy(this->inner, sizeof(T), alignof(T));
        }
        
        this->inner = nullptr;
    }
}

#endif // SY_TYPES_SYNC_OBJ_SYNC_OBJ_HPP_