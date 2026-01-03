#ifndef SY_TYPES_BOX_BOX_HPP_
#define SY_TYPES_BOX_BOX_HPP_

#include "../../core/core.h"
#include "../../mem/allocator.hpp"
#include "../template_type_operations.hpp"
#include <type_traits>
#include <utility>

namespace sy {
class Type;

template <typename T> class Box;

class SY_API RawBox final {
  public:
    static Result<RawBox, AllocErr> init(Allocator alloc, void* obj, size_t size, size_t align) noexcept;

    static Result<RawBox, AllocErr> initCustomMove(Allocator alloc, void* obj, size_t size, size_t align,
                                                   void (*moveConstructFn)(void* dst, void* src)) noexcept;

    static Result<RawBox, AllocErr> initScript(Allocator alloc, void* obj, const Type* typeInfo) noexcept;

    ~RawBox() noexcept;

    void destroy(void (*destruct)(void* ptr), size_t size, size_t align) noexcept;

    void destroyScript(const Type* typeInfo) noexcept;

    RawBox(RawBox&& other) noexcept;

    RawBox& operator=(RawBox&& other) = delete;

    void moveAssign(RawBox&& other, void (*destruct)(void* ptr), size_t size, size_t align) noexcept;

    void moveAssignScript(RawBox&& other, const Type* typeInfo) noexcept;

    RawBox(const RawBox& other) = delete;

    RawBox& operator=(const RawBox& other) = delete;

    void* get() { return obj_; }

    const void* get() const { return obj_; }

  private:
    RawBox(void* obj, Allocator alloc) : obj_(obj), alloc_(alloc) {}
    template <typename T> friend class Box;
    RawBox() = default;

    void* obj_ = nullptr;
    Allocator alloc_{};
};

template <typename T> class SY_API Box final {
  public:
    Box(T obj) noexcept;

    Box(T obj, Allocator alloc) noexcept;

    Box(T* takeObj, Allocator alloc) noexcept;

    [[nodiscard]] static Result<Box, AllocErr> init(Allocator alloc, T obj) noexcept;

    ~Box() noexcept;

    Box(Box&& other) noexcept;

    Box& operator=(Box&& other) noexcept;

    T* get() { return reinterpret_cast<T*>(this->inner_.get()); }

    const T* get() const { return reinterpret_cast<const T*>(this->inner_.get()); }

    T& operator*() { return *get(); }

    const T& operator*() const { return *get(); }

    T* operator->() { return get(); }

    const T* operator->() const { return get(); }

  private:
    constexpr static detail::DestructFn elementDestruct = detail::makeDestructor<T>();

  private:
    Box(RawBox&& inner) : inner_(std::move(inner)) {}
    RawBox inner_{};
};

template <typename T> Box<T>::Box(T obj) noexcept {
    auto result = Box::init(Allocator(), std::move(obj));
    new (this) Box(result.takeValue());
}

template <typename T> Box<T>::Box(T obj, Allocator alloc) noexcept {
    auto result = Box::init(alloc, std::move(obj));
    new (this) Box(result.takeValue());
}

template <typename T>
Box<T>::Box(T* takeObj, Allocator alloc) noexcept : inner_(reinterpret_cast<void*>(takeObj), alloc) {}

template <typename T> inline Result<Box<T>, AllocErr> Box<T>::init(Allocator alloc, T obj) noexcept {
    if constexpr (std::is_trivially_copyable_v<T>) {
        auto result = RawBox::init(alloc, reinterpret_cast<void*>(&obj), sizeof(T), alignof(T));
        if (result.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        return Box(result.takeValue());
    } else {
        auto result = RawBox::initCustomMove(alloc, reinterpret_cast<void*>(&obj), detail::makeMoveConstructor<T>(),
                                             sizeof(T), alignof(T));
        if (result.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        return Box(result.takeValue());
    }
}

template <typename T> inline Box<T>::~Box() noexcept { this->inner_.destroy(elementDestruct, sizeof(T), alignof(T)); }

template <typename T> inline Box<T>::Box(Box&& other) noexcept : inner_(std::move(other.inner_)) {}

template <typename T> inline Box<T>& Box<T>::operator=(Box&& other) noexcept {
    if (this != &other) {
        this->inner_.moveAssign(std::move(other.inner_), elementDestruct, sizeof(T), alignof(T));
    }
    return *this;
}

} // namespace sy

#endif // SY_TYPES_BOX_BOX_HPP_