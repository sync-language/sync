//! API
#pragma once
#ifndef SY_TYPES_ARRAY_LIST_HPP_
#define SY_TYPES_ARRAY_LIST_HPP_

#include "../../core/builtin_traits/builtin_traits.hpp"
#include "../../core/exceptional.hpp"
#include "../../mem/allocator.hpp"
#include "../reflect_fwd.hpp"
#include "../result/result.hpp"
#include <iterator>
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

    [[nodiscard]] const T* dataUnchecked() const noexcept { return this->data_; }

    [[nodiscard]] T* dataUnchecked() noexcept { return this->data_; }

    [[nodiscard]] Allocator allocator() const noexcept { return this->allocator_; }

    template <typename U> [[nodiscard]] Result<void, AllocErr> push(U&& value) noexcept;

    template <typename U> [[nodiscard]] Result<void, AllocErr> pushFront(U&& value) noexcept;

    template <typename U>
    [[nodiscard]] Result<void, AllocErr> insertAt(U&& value, size_t index) noexcept;

    void removeAt(size_t index) noexcept;

    Result<void, AllocErr> reserve(size_t minCapacity) noexcept;

    Result<void, AllocErr> reserveFront(size_t minCapacity) noexcept;

    using Iterator = T*;
    using ConstIterator = const T*;
    using ReverseIterator = std::reverse_iterator<T*>;
    using ConstReverseIterator = std::reverse_iterator<const T*>;

    [[nodiscard]] Iterator begin() noexcept { return this->dataUnchecked(); }
    [[nodiscard]] Iterator end() noexcept { return this->dataUnchecked() + this->len_; }
    [[nodiscard]] ConstIterator begin() const noexcept { return this->dataUnchecked(); }
    [[nodiscard]] ConstIterator end() const noexcept { return this->dataUnchecked() + this->len_; }

    [[nodiscard]] ReverseIterator rbegin() noexcept { return ReverseIterator(this->end()); }
    [[nodiscard]] ReverseIterator rend() noexcept { return ReverseIterator(this->begin()); }
    [[nodiscard]] ConstReverseIterator rbegin() const noexcept {
        return ConstReverseIterator(this->end());
    }
    [[nodiscard]] ConstReverseIterator rend() const noexcept {
        return ConstReverseIterator(this->begin());
    }

  private:
    friend struct Test_List;

    T* data_ = nullptr;
    size_t len_ = 0;
    size_t capacity_ = 0;
    T* allocated_ = nullptr;
    Allocator allocator_;
};

namespace internal {
SY_API void sy_list_free_impl(void* list, size_t typeSize, size_t typeAlign,
                              NativeDestructorFn destruct) noexcept;

SY_API Result<void, Exceptional> sy_list_clone_impl(const void* srcList, void* dstList,
                                                    size_t typeSize, size_t typeAlign,
                                                    NativeCloneFn clone,
                                                    NativeDestructorFn destruct) noexcept;

SY_API void sy_list_assert_release_index_in_range(const void* list, size_t index) noexcept;

SY_API Result<void*, AllocErr> sy_list_add_one_front_impl(void* list, size_t typeSize,
                                                          size_t typeAlign) noexcept;

SY_API Result<void*, AllocErr> sy_list_add_one_back_impl(void* list, size_t typeSize,
                                                         size_t typeAlign) noexcept;

SY_API Result<void*, AllocErr> sy_list_add_one_insert_at_impl(void* list, size_t typeSize,
                                                              size_t typeAlign,
                                                              size_t index) noexcept;

SY_API void sy_list_remove_at_impl(void* list, size_t index, size_t typeSize,
                                   NativeDestructorFn destruct) noexcept;

SY_API Result<void, AllocErr> sy_list_reserve_back_impl(void* list, size_t minCapacity,
                                                        size_t typeSize, size_t typeAlign) noexcept;

SY_API Result<void, AllocErr> sy_list_reserve_front_impl(void* list, size_t minCapacity,
                                                         size_t typeSize,
                                                         size_t typeAlign) noexcept;
} // namespace internal

template <typename T> inline List<T>::List() noexcept : allocator_(Allocator()) {}

template <typename T> inline List<T>::List(Allocator alloc) noexcept : allocator_(alloc) {}

template <typename T> inline List<T>::List(const List<T>& other) noexcept {
    new (this) List(other.clone().takeValue());
}

template <typename T> inline List<T>& List<T>::operator=(const List<T>& other) noexcept {
    if (this != &other) {
        Type rttiType = Reflect<T>::get();
        internal::sy_list_free_impl(
            this, rttiType.byteSize(), rttiType.byteAlign(),
            +[](void* obj) { static_cast<T*>(obj)->~T(); });
        new (this) List(other.clone().takeValue()); // invokes fatal handler if is error
    }
    return *this;
}

template <typename T> inline Result<List<T>, Exceptional> List<T>::clone() const noexcept {
    List out;
    if (auto res = internal::sy_list_clone_impl(
            this, &out, Reflect<T>::get().byteSize(), BuiltInCoherentTraits::NATIVE_CLONE_FN_OF<T>,
            +[](void* obj) { static_cast<T*>(obj)->~T(); });
        res.hasErr()) {
        return Error(res.err());
    }
    return out;
}

template <typename T>
inline List<T>::List(List&& other) noexcept
    : data_(other.data_), len_(other.len_), capacity_(other.capacity_),
      allocated_(other.allocated_), allocator_(other.allocator_) {
    other.data_ = nullptr;
    other.len_ = 0;
    other.capacity_ = 0;
    other.allocated_ = nullptr;
}

template <typename T> inline List<T>& List<T>::operator=(List&& other) noexcept {
    if (this != &other) {
        Type rttiType = Reflect<T>::get();
        internal::sy_list_free_impl(
            this, rttiType.byteSize(), rttiType.byteAlign(),
            +[](void* obj) { static_cast<T*>(obj)->~T(); });
        this->data_ = other.data_;
        this->len_ = other.len_;
        this->capacity_ = other.capacity_;
        this->allocated_ = other.allocated_;
        this->allocator_ = other.allocator_;
        other.data_ = nullptr;
        other.len_ = 0;
        other.capacity_ = 0;
        other.allocated_ = nullptr;
    }
    return *this;
}

template <typename T> inline List<T>::~List() noexcept {
    Type rttiType = Reflect<T>::get();
    // no-op if empty, and can be called multiple times on the same object, safe :)
    internal::sy_list_free_impl(
        this, rttiType.byteSize(), rttiType.byteAlign(),
        +[](void* obj) { static_cast<T*>(obj)->~T(); });
}

template <typename T> inline const T& List<T>::at(size_t index) const noexcept {
    internal::sy_list_assert_release_index_in_range(this, index);
    return this->data_[index];
}

template <typename T> inline T& List<T>::at(size_t index) noexcept {
    internal::sy_list_assert_release_index_in_range(this, index);
    return this->data_[index];
}

template <typename T>
template <typename U>
inline Result<void, AllocErr> List<T>::push(U&& value) noexcept {
    Type rttiType = Reflect<T>::get();
    auto res = internal::sy_list_add_one_back_impl(this, rttiType.byteSize(), rttiType.byteAlign());
    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    T* mem = reinterpret_cast<T*>(res.takeValue());
    new (mem) T(std::forward<U>(value));
    this->len_ += 1;
    return {};
}

template <typename T>
template <typename U>
inline Result<void, AllocErr> List<T>::pushFront(U&& value) noexcept {
    Type rttiType = Reflect<T>::get();
    auto res =
        internal::sy_list_add_one_front_impl(this, rttiType.byteSize(), rttiType.byteAlign());
    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    T* mem = reinterpret_cast<T*>(res.takeValue());
    new (mem) T(std::forward<U>(value));
    this->data_ = mem; // shift the data offset back
    this->len_ += 1;
    return {};
}

template <typename T>
template <typename U>
inline Result<void, AllocErr> List<T>::insertAt(U&& value, size_t index) noexcept {
    Type rttiType = Reflect<T>::get();
    auto res = internal::sy_list_add_one_insert_at_impl(this, rttiType.byteSize(),
                                                        rttiType.byteAlign(), index);
    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }
    T* mem = reinterpret_cast<T*>(res.takeValue());
    new (mem) T(std::forward<U>(value));
    this->len_ += 1;
    return {};
}

template <typename T> inline void List<T>::removeAt(size_t index) noexcept {
    Type rttiType = Reflect<T>::get();
    internal::sy_list_remove_at_impl(
        this, index, rttiType.byteSize(), +[](void* obj) { static_cast<T*>(obj)->~T(); });
}

template <typename T> inline Result<void, AllocErr> List<T>::reserve(size_t minCapacity) noexcept {
    Type rttiType = Reflect<T>::get();
    internal::sy_list_reserve_back_impl(this, minCapacity, rttiType.byteSize(),
                                        rttiType.byteAlign());
}

template <typename T>
inline Result<void, AllocErr> List<T>::reserveFront(size_t minCapacity) noexcept {
    Type rttiType = Reflect<T>::get();
    internal::sy_list_reserve_front_impl(this, minCapacity, rttiType.byteSize(),
                                         rttiType.byteAlign());
}

} // namespace sy

#endif // SY_TYPES_ARRAY_LIST_HPP_
