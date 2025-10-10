//! API
#pragma once
#ifndef SY_MEM_ALLOCATOR_HPP_
#define SY_MEM_ALLOCATOR_HPP_

#include "../core.h"
#include "../types/result/result.hpp"

namespace sy {
enum class AllocErr : int { OutOfMemory = 0 };

class Allocator;

/// Interface for C++ specific allocators
class SY_API IAllocator {
  public:
    virtual ~IAllocator() {}

    Allocator asAllocator();

  protected:
    virtual void* alloc(size_t len, size_t align) noexcept = 0;

    virtual void free(void* buf, size_t len, size_t align) noexcept = 0;

  private:
    friend class Allocator;

    static void* allocImpl(IAllocator* self, size_t len, size_t align) noexcept;
    static void freeImpl(IAllocator* self, void* buf, size_t len, size_t align) noexcept;
};

namespace detail {
void debugAssertNonNull(void* ptr) noexcept;
void debugAssertHasVal(bool hasVal) noexcept;
} // namespace detail

/// Can be bitcast to `c::SyAllocator`.
class SY_API Allocator final {
  public:
    struct VTable {
        using alloc_fn = void* (*)(void* self, size_t len, size_t align);
        using free_fn = void (*)(void* self, void* buf, size_t len, size_t align);

        alloc_fn allocFn;
        free_fn freeFn;
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
    template <typename T> Result<T*, AllocErr> allocObject() noexcept {
        T* ptr = reinterpret_cast<T*>(this->allocImpl(sizeof(T), alignof(T)));
        if (ptr == nullptr) {
            return Error(AllocErr::OutOfMemory);
        }
        return ptr;
    }

    template <typename T> Result<T*, AllocErr> allocArray(size_t len) noexcept {
        T* ptr = reinterpret_cast<T*>(this->allocImpl(sizeof(T) * len, alignof(T)));
        if (ptr == nullptr) {
            return Error(AllocErr::OutOfMemory);
        }
        return ptr;
    }

    /// Allocate memory for a single instance of T. Does not call constructor.
    template <typename T> Result<T*, AllocErr> allocAlignedObject(size_t align) noexcept {
        const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
        T* ptr = reinterpret_cast<T*>(this->allocImpl(sizeof(T), actualAlign));
        if (ptr == nullptr) {
            return Error(AllocErr::OutOfMemory);
        }
        return ptr;
    }

    template <typename T> Result<T*, AllocErr> allocAlignedArray(size_t len, size_t align) noexcept {
        const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
        T* ptr = reinterpret_cast<T*>(this->allocImpl(sizeof(T) * len, actualAlign));
        if (ptr == nullptr) {
            return Error(AllocErr::OutOfMemory);
        }
        return ptr;
    }

    template <typename T> void freeObject(T* obj) noexcept { this->freeImpl(obj, sizeof(T), alignof(T)); }

    template <typename T> void freeArray(T* obj, size_t len) noexcept {
        this->freeImpl(obj, sizeof(T) * len, alignof(T));
    }

    template <typename T> void freeAlignedObject(T* obj, size_t align) noexcept {
        const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
        this->freeImpl(obj, sizeof(T), actualAlign);
    }

    template <typename T> void freeAlignedArray(T* obj, size_t len, size_t align) noexcept {
        const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
        this->freeImpl(obj, sizeof(T) * len, actualAlign);
    }

  private:
    void* allocImpl(size_t len, size_t align) noexcept;

    void freeImpl(void* buf, size_t len, size_t align) noexcept;

  private:
    friend class IAllocator;

    void* ptr_;
    const VTable* vtable_;
};
} // namespace sy

#endif // SY_MEM_ALLOCATOR_HPP_
