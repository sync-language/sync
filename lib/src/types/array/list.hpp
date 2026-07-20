//! API
#pragma once
#ifndef SY_TYPES_ARRAY_LIST_HPP_
#define SY_TYPES_ARRAY_LIST_HPP_

#include "../../core/builtin_traits/builtin_traits.hpp"
#include "../../core/exceptional.hpp"
#include "../../mem/allocator.hpp"
#include "../result/result.hpp"
#include <new>
#include <type_traits>

namespace sy {
namespace internal {
struct Test_List;
} // namespace internal

template <typename T> class List final {
  public:
    List() noexcept;

    List(Allocator alloc) noexcept;

    /// @brief
    /// @param other a
    /// @remark Will trigger the sync fatal error handler if memory allocation fails. For the
    /// checked version, see `List<T>::clone()`.
    List(const List& other) noexcept;

    /// @remark Will trigger the sync fatal error handler if memory allocation fails. For the
    /// checked version, see `List<T>::clone()`.
    List& operator=(const List& other) noexcept;

    [[nodiscard]] Result<List, Exceptional> clone() const noexcept;

    List(List&& other) noexcept;

    List& operator=(List&& other) noexcept;

    ~List() noexcept;

    [[nodiscard]] size_t len() const { return this->len_; }

    [[nodiscard]] const T& at(size_t index) const noexcept;

    [[nodiscard]] T& at(size_t index) noexcept;

    [[nodiscard]] const T& operator[](size_t index) const noexcept { return this->at(index); }

    [[nodiscard]] T& operator[](size_t index) noexcept { return this->at(index); }

    [[nodiscard]] const T* dataUnchecked() const noexcept {
        return static_cast<const T*>(this->data_);
    }

    [[nodiscard]] T* dataUnchecked() noexcept { return static_cast<T*>(this->data_); }

    [[nodiscard]] Allocator allocator() const noexcept { return this->allocator_; }

    template <typename U> [[nodiscard]] Result<void, AllocErr> push(U&& value) noexcept;

    template <typename U> [[nodiscard]] Result<void, AllocErr> pushFront(U&& value) noexcept;

    template <typename U>
    [[nodiscard]] Result<void, AllocErr> insertAt(U&& value, size_t index) noexcept;

    void removeAt(size_t index) noexcept;

    Result<void, AllocErr> reserve(size_t minCapacity) noexcept;

  private:
    friend struct Test_List;

    void* data_ = nullptr;
    size_t len_ = 0;
    size_t capacity_ = 0;
    void* allocated_ = nullptr;
    Allocator allocator_;
};

namespace internal {
SY_API void sy_list_free_impl(void* list, size_t typeSize, size_t typeAlign,
                              NativeDestructorFn destruct) noexcept;
SY_API int sy_list_clone_impl(const void* srcList, void* dstList, size_t typeSize,
                              NativeCloneFn clone) noexcept;
} // namespace internal

template <typename T> inline List<T>::List() noexcept : allocator_(Allocator()) {}

template <typename T> inline List<T>::List(Allocator alloc) noexcept : allocator_(alloc) {}

template <typename T> inline List<T>::List(const List<T>& other) noexcept {
    new (this) List(other.clone().takeValue());
}

template <typename T> inline List<T>& List<T>::operator=(const List<T>& other) noexcept {
    if (this != other) {
        internal::sy_list_free_impl(this);
        new (this) List(other.clone().takeValue());
    }
    return *this;
}

template <typename T> inline Result<List<T>, Exceptional> List<T>::clone() const noexcept {
    List out;
    if (sy_list_clone_impl(this, &out) == 0) {
        return out;
    }
    return Error(AllocErr::OutOfMemory);
}
} // namespace sy

#endif // SY_TYPES_ARRAY_LIST_HPP_
