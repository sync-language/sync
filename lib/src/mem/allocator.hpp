//! API
#pragma once
#ifndef SY_MEM_ALLOCATOR_HPP_
#define SY_MEM_ALLOCATOR_HPP_

#include "../core.h"

namespace sy {
    class Allocator;

    /// Interface for C++ specific allocators
    class SY_API IAllocator {
        friend class Allocator;
    public:
        virtual ~IAllocator() {}

        Allocator asAllocator();
    protected:
        virtual void* alloc(size_t len, size_t align) = 0;

        virtual void free(void* buf, size_t len, size_t align) = 0;
    
    private:
        static void* allocImpl(IAllocator* self, size_t len, size_t align);
        static void freeImpl(IAllocator* self, void* buf, size_t len, size_t align);
    };

    /// Can be bitcast to `c::SyAllocator`.
    class SY_API Allocator final {
    public:

        struct VTable {
            using alloc_fn = void*(*)(void* self, size_t len, size_t align);
            using free_fn = void(*)(void* self, void* buf, size_t len, size_t align);

            alloc_fn    allocFn;
            free_fn     freeFn;
        };

        template<typename T>
        class Result final {
            friend class Allocator;
        public:
            Result() : _mem(nullptr) {}
            
            T* get() const {
                T* mem = const_cast<T*>(this->_mem);
                Allocator::debugAssertNonNull(reinterpret_cast<void*>(mem));
                return mem;
            }
        private:
            Result(T* ptr) : _mem(ptr) {}
        private:
            T* _mem;
        };

        /// Default initializes to the global allocator.
        Allocator();

        // Does nothing
        ~Allocator() = default;

        Allocator(const Allocator&) = default;
        Allocator& operator=(const Allocator&) = default;

        Allocator(Allocator&& other) = default;
        Allocator& operator=(Allocator&& other) = default;

        /// Allocate memory for a single instance of T. Does not call constructor.
        template<typename T>
        Result<T> allocObject() {
            return reinterpret_cast<T*>(
                this->allocImpl(sizeof(T), alignof(T)));      
        }

        template<typename T>
        Result<T> allocArray(size_t len) {
            return reinterpret_cast<T*>(
                this->allocImpl(sizeof(T) * len, alignof(T)));    
        }

        /// Allocate memory for a single instance of T. Does not call constructor.
        template<typename T>
        Result<T> allocAlignedObject(size_t align) {
            const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
            return reinterpret_cast<T*>(
                this->allocImpl(sizeof(T), actualAlign));      
        }

        template<typename T>
        Result<T> allocAlignedArray(size_t len, size_t align) {
            const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
            return reinterpret_cast<T*>(
                this->allocImpl(sizeof(T) * len, actualAlign));    
        }

        template<typename T>
        void freeObject(T*& obj) {
            this->freeImpl(obj, sizeof(T), alignof(T));
            obj = nullptr;
        }

        template<typename T>
        void freeArray(T*& obj, size_t len) {
            this->freeImpl(obj, sizeof(T) * len, alignof(T));
            obj = nullptr;
        }

        template<typename T>
        void freeAlignedObject(T*& obj, size_t align) {
            const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
            this->freeImpl(obj, sizeof(T), actualAlign);
            obj = nullptr;
        }

        template<typename T>
        void freeAlignedArray(T*& obj, size_t len, size_t align) {
            const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
            this->freeImpl(obj, sizeof(T) * len, actualAlign);
            obj = nullptr;
        }

    private:

        void* allocImpl(size_t len, size_t align);

        void freeImpl(void* buf, size_t len, size_t align);

        static void debugAssertNonNull(void* ptr);

    private:

        friend class IAllocator;
        template<typename T>
        friend class Allocator::Result;

        void* ptr;
        const VTable* vtable;
    };
}

#endif // SY_MEM_ALLOCATOR_HPP_
