//! API
#pragma once
#ifndef SY_MEM_ALLOCATOR_HPP_
#define SY_MEM_ALLOCATOR_HPP_

#include "../core.h"

namespace sy {
    namespace c {
        #include "allocator.h"

        using SyAllocator = SyAllocator;
        using SyAllocatorVTable = SyAllocatorVTable;
        using sy_allocator_alloc_fn = sy_allocator_alloc_fn;
        using sy_allocator_free_fn = sy_allocator_free_fn;
        using sy_allocator_destructor_fn = sy_allocator_destructor_fn;

        // What the flip
        extern "C" {
            SY_API void* sy_allocator_alloc(SyAllocator* self, size_t len, size_t align);
                        /// @param `buf` Non-null. 
            SY_API void sy_allocator_free(SyAllocator* self, void* buf, size_t len, size_t align);

            SY_API void sy_allocator_destructor(SyAllocator* self);
        }
    }

    class Allocator;

    /// Interface for C++ specific allocators
    class IAllocator {
        friend class Allocator;
    protected:
        /// NOTE when overriding, the `this` needs to be free'd, or else a memory leak will occur.
        virtual ~IAllocator() {};
        virtual void* alloc(size_t len, size_t align) = 0;
        virtual void free(void* buf, size_t len, size_t align) = 0;
    };

    namespace detail {
        void allocator_result_ensure_non_null(void* ptr);
    }

    /// Can be bitcast to `c::SyAllocator`.
    class SY_API Allocator final {
    public:

        template<typename T>
        class Result {
            friend class Allocator;
        public:
            Result() : _mem(nullptr) {}
            
            T* get() const {
                T* mem = const_cast<T*>(this->_mem);
                detail::allocator_result_ensure_non_null(reinterpret_cast<void*>(mem));
                return mem;
            }
        private:
            Result(T* ptr) : _mem(ptr) {}
        private:
            T* _mem;
        };

        Allocator();

        Allocator(c::SyAllocator&& ownedAllocator);

        ~Allocator();

        Allocator(const Allocator&) = delete;
        Allocator& operator=(const Allocator&) = delete;

        Allocator(Allocator&& other);
        Allocator& operator=(Allocator&& other);

        template<typename T, typename ...AllocatorConstructArgs>
        static Allocator initWith(AllocatorConstructArgs&&... args) {
            return Allocator(new T(args...));
        }

        /// Allocate memory for a single instance of T. Does not call constructor.
        template<typename T>
        Result<T> allocObject() {
            return reinterpret_cast<T*>(
                c::sy_allocator_alloc(&this->_allocator, sizeof(T), alignof(T)));      
        }

        template<typename T>
        Result<T> allocArray(size_t len) {
            return reinterpret_cast<T*>(
                c::sy_allocator_alloc(&this->_allocator, sizeof(T) * len, alignof(T)));    
        }

        /// Allocate memory for a single instance of T. Does not call constructor.
        template<typename T>
        Result<T> allocAlignedObject(size_t align) {
            const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
            return reinterpret_cast<T*>(
                c::sy_allocator_alloc(&this->_allocator, sizeof(T), actualAlign));      
        }

        template<typename T>
        Result<T> allocAlignedArray(size_t len, size_t align) {
            const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
            return reinterpret_cast<T*>(
                c::sy_allocator_alloc(&this->_allocator, sizeof(T) * len, actualAlign));    
        }

        template<typename T>
        void freeObject(T*& obj) {
            c::sy_allocator_free(&this->_allocator, obj, sizeof(T), alignof(T));
            obj = nullptr;
        }

        template<typename T>
        void freeArray(T*& obj, size_t len) {
            c::sy_allocator_free(&this->_allocator, obj, sizeof(T) * len, alignof(T));
            obj = nullptr;
        }

        template<typename T>
        void freeAlignedObject(T*& obj, size_t align) {
            const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
            c::sy_allocator_free(&this->_allocator, obj, sizeof(T), actualAlign);
            obj = nullptr;
        }

        template<typename T>
        void freeAlignedArray(T*& obj, size_t len, size_t align) {
            const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
            c::sy_allocator_free(&this->_allocator, obj, sizeof(T) * len, actualAlign);
            obj = nullptr;
        }

        c::SyAllocator& cAllocator() { return this->_allocator; }

    private:

        Allocator(IAllocator* ownedAllocator);

        static void* cppAllocFn(void* self, size_t len, size_t align);
        static void cppFreeFn(void* self, void* buf, size_t len, size_t align);
        static void cppDestructorFn(void* self);

        static c::SyAllocatorVTable cppVTable;    

    private:

        c::SyAllocator _allocator;
        
    };
}

#endif // SY_MEM_ALLOCATOR_HPP_
