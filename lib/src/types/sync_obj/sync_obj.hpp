//! API
#ifndef SY_TYPES_SYNC_OBJ_SYNC_OBJ_HPP_
#define SY_TYPES_SYNC_OBJ_SYNC_OBJ_HPP_

#include "../../core.h"
#include "../type_info.hpp"
#include <cstddef>

namespace sy {
    template<typename T>
    class Weak;

    template<typename T>
    class SY_API Owned {
        Owned() = default;

        ~Owned();
        
    private:
        void* inner = nullptr;
    };

    template<typename T>
    class SY_API Shared {
        Shared() = default;

        ~Shared();

    private:
        void* inner = nullptr;
    };

    template<typename T>
    class SY_API Weak {
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