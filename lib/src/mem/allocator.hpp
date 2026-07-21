//! API
#pragma once
#ifndef SY_MEM_ALLOCATOR_HPP_
#define SY_MEM_ALLOCATOR_HPP_

#include "../core/core.h"
#include "../types/result/result.hpp"

namespace sy {
enum class AllocErr : int { OutOfMemory = 1 };

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

    static void* allocImpl(void* self, size_t len, size_t align) noexcept;
    static void freeImpl(void* self, void* buf, size_t len, size_t align) noexcept;
};

namespace detail {
void debugAssertNonNull(void* ptr) noexcept;
void debugAssertHasVal(bool hasVal) noexcept;
} // namespace detail

template <typename T> class Allocated;

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
    template <typename T> Result<Allocated<T>, AllocErr> allocObject() noexcept;

    template <typename T> Result<Allocated<T>, AllocErr> allocArray(size_t len) noexcept;

    /// Allocate memory for a single instance of T. Does not call constructor.
    template <typename T> Result<Allocated<T>, AllocErr> allocAlignedObject(size_t align) noexcept;

    template <typename T>
    Result<Allocated<T>, AllocErr> allocAlignedArray(size_t len, size_t align) noexcept;

    template <typename T> void freeObject(T* obj) noexcept;

    template <typename T> void freeArray(T* obj, size_t len) noexcept;

    template <typename T> void freeAlignedObject(T* obj, size_t align) noexcept;

    template <typename T> void freeAlignedArray(T* obj, size_t len, size_t align) noexcept;

  private:
    void* allocImpl(size_t len, size_t align) noexcept;

    void freeImpl(void* buf, size_t len, size_t align) noexcept;

  private:
    friend class IAllocator;

    void* ptr_;
    const VTable* vtable_;
};

template <typename T> class SY_API Allocated final {
  public:
    Allocated(T* obj, Allocator allocator, size_t count, size_t align)
        : obj_(obj), allocator_(allocator), count_(count), alignBytes_(align) {}

    ~Allocated() noexcept;

    T* get() noexcept;

    T* take() noexcept;

  private:
    T* obj_;
    Allocator allocator_;
    size_t count_;
    size_t alignBytes_;
};

/// Allocate memory for a single instance of T. Does not call constructor.
template <typename T> Result<Allocated<T>, AllocErr> Allocator::allocObject() noexcept {
    T* ptr = reinterpret_cast<T*>(this->allocImpl(sizeof(T), alignof(T)));
    if (ptr == nullptr) {
        return Error(AllocErr::OutOfMemory);
    }
    return Allocated<T>(ptr, *this, 1, alignof(T));
}

template <typename T> Result<Allocated<T>, AllocErr> Allocator::allocArray(size_t len) noexcept {
    T* ptr = reinterpret_cast<T*>(this->allocImpl(sizeof(T) * len, alignof(T)));
    if (ptr == nullptr) {
        return Error(AllocErr::OutOfMemory);
    }
    return Allocated<T>(ptr, *this, len, alignof(T));
}

/// Allocate memory for a single instance of T. Does not call constructor.
template <typename T>
Result<Allocated<T>, AllocErr> Allocator::allocAlignedObject(size_t align) noexcept {
    const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
    T* ptr = reinterpret_cast<T*>(this->allocImpl(sizeof(T), actualAlign));
    if (ptr == nullptr) {
        return Error(AllocErr::OutOfMemory);
    }
    return Allocated<T>(ptr, *this, len, actualAlign);
}

template <typename T>
Result<Allocated<T>, AllocErr> Allocator::allocAlignedArray(size_t len, size_t align) noexcept {
    const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
    T* ptr = reinterpret_cast<T*>(this->allocImpl(sizeof(T) * len, actualAlign));
    if (ptr == nullptr) {
        return Error(AllocErr::OutOfMemory);
    }
    return Allocated<T>(ptr, *this, len, actualAlign);
}

template <typename T> void Allocator::freeObject(T* obj) noexcept {
    this->freeImpl(obj, sizeof(T), alignof(T));
}

template <typename T> void Allocator::freeArray(T* obj, size_t len) noexcept {
    this->freeImpl(obj, sizeof(T) * len, alignof(T));
}

template <typename T> void Allocator::freeAlignedObject(T* obj, size_t align) noexcept {
    const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
    this->freeImpl(obj, sizeof(T), actualAlign);
}

template <typename T> void Allocator::freeAlignedArray(T* obj, size_t len, size_t align) noexcept {
    const size_t actualAlign = alignof(T) > align ? alignof(T) : align;
    this->freeImpl(obj, sizeof(T) * len, actualAlign);
}

template <typename T> inline Allocated<T>::~Allocated() noexcept {
    if (this->obj_ == nullptr) {
        return;
    }

    this->allocator_.freeAlignedArray(this->obj_, this->count_, this->alignBytes_);
    this->obj_ = nullptr;
    this->len_ = 0;
    this->align_ = 0;
}

template <typename T> inline T* Allocated<T>::get() noexcept { return this->obj_; }

template <typename T> inline T* Allocated<T>::take() noexcept {
    T* obj = this->obj_;
    this->obj_ = nullptr;
    this->len_ = 0;
    this->align_ = 0;
    return obj;
}

} // namespace sy

#endif // SY_MEM_ALLOCATOR_HPP_
