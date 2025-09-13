//! API
#ifndef SY_TYPES_ARRAY_DYNAMIC_ARRAY_HPP_
#define SY_TYPES_ARRAY_DYNAMIC_ARRAY_HPP_

#include "../../core.h"
#include "../../mem/allocator.hpp"
#include "../template_type_operations.hpp"
#include <utility>

namespace sy {

class Type;

class SY_API RawDynArrayUnmanaged SY_CLASS_FINAL {
  public:
    RawDynArrayUnmanaged() = default;

    ~RawDynArrayUnmanaged() noexcept;

    void destroy(Allocator& alloc, void (*destruct)(void* ptr), size_t size, size_t align) noexcept;

    void destroyScript(Allocator& alloc, const Type* typeInfo) noexcept;

    RawDynArrayUnmanaged(RawDynArrayUnmanaged&& other) noexcept;

    RawDynArrayUnmanaged& operator=(RawDynArrayUnmanaged&& other) = delete;

    void moveAssign(RawDynArrayUnmanaged&& other, void (*destruct)(void* ptr), Allocator& alloc, size_t size,
                    size_t align) noexcept;

    RawDynArrayUnmanaged(const RawDynArrayUnmanaged& other) = delete;

    [[nodiscard]] static Result<RawDynArrayUnmanaged, AllocErr>
    copyConstruct(const RawDynArrayUnmanaged& other, Allocator& alloc,
                  void (*copyConstructFn)(void* dst, const void* src), size_t size, size_t align) noexcept;

    // TODO script types copy
    // [[nodiscard]] static AllocExpect<RawDynArrayUnmanaged> copyConstructScript(
    //     const RawDynArrayUnmanaged& other,
    //     Allocator& alloc,
    //     const Type* typeInfo
    // ) noexcept;

    RawDynArrayUnmanaged& operator=(const RawDynArrayUnmanaged& other) = delete;

    [[nodiscard]] Result<void, AllocErr> copyAssign(const RawDynArrayUnmanaged& other, Allocator& alloc,
                                                    void (*destruct)(void* ptr),
                                                    void (*copyConstructFn)(void* dst, const void* src), size_t size,
                                                    size_t align) noexcept;

    // TODO script types copy
    // [[nodiscard]] AllocExpect<void> copyAssignScript(
    //     const DynArrayUnmanaged& other,
    //     Allocator& alloc,
    //     const Type* typeInfo
    // ) noexcept;

    [[nodiscard]] size_t len() const { return len_; }

    [[nodiscard]] const void* at(size_t index, size_t size) const;

    [[nodiscard]] void* at(size_t index, size_t size);

    [[nodiscard]] const void* data() const { return this->data_; }

    [[nodiscard]] void* data() { return this->data_; }

    [[nodiscard]] Result<void, AllocErr> push(void* element, Allocator& alloc, size_t size, size_t align) noexcept;

    [[nodiscard]] Result<void, AllocErr> pushCustomMove(void* element, Allocator& alloc, size_t size, size_t align,
                                                        void (*moveConstructFn)(void* dst, void* src)) noexcept;

    [[nodiscard]] Result<void, AllocErr> pushScript(void* element, Allocator& alloc, const Type* typeInfo) noexcept;

    [[nodiscard]] Result<void, AllocErr> pushFront(void* element, Allocator& alloc, size_t size, size_t align) noexcept;

    [[nodiscard]] Result<void, AllocErr> pushFrontCustomMove(void* element, Allocator& alloc, size_t size, size_t align,
                                                             void (*moveConstructFn)(void* dst, void* src)) noexcept;

    [[nodiscard]] Result<void, AllocErr> pushFrontScript(void* element, Allocator& alloc,
                                                         const Type* typeInfo) noexcept;

    [[nodiscard]] Result<void, AllocErr> insertAt(void* element, Allocator& alloc, size_t index, size_t size,
                                                  size_t align) noexcept;

    [[nodiscard]] Result<void, AllocErr> insertAtCustomMove(void* element, Allocator& alloc, size_t index, size_t size,
                                                            size_t align,
                                                            void (*moveConstructFn)(void* dst, void* src)) noexcept;

    [[nodiscard]] Result<void, AllocErr> insertAtScript(void* element, Allocator& alloc, size_t index,
                                                        const Type* typeInfo) noexcept;

    [[nodiscard]] void removeAt(size_t index, void (*destruct)(void* ptr), size_t size) noexcept;

    [[nodiscard]] void removeAtCustomMove(size_t index, void (*destruct)(void* ptr), size_t size,
                                          void (*moveConstructFn)(void* dst, void* src)) noexcept;

    [[nodiscard]] void removeAtScript(size_t index, const Type* typeInfo) noexcept;

  private:
    [[nodiscard]] Result<void, AllocErr> reallocateBack(Allocator& alloc, const size_t size,
                                                        const size_t align) noexcept;

    [[nodiscard]] Result<void, AllocErr>
    reallocateBackCustomMove(Allocator& alloc, const size_t size, const size_t align,
                             void (*moveConstructFn)(void* dst, void* src)) noexcept;

    [[nodiscard]] Result<void, AllocErr> reallocateFront(Allocator& alloc, const size_t size,
                                                         const size_t align) noexcept;

    [[nodiscard]] Result<void, AllocErr>
    reallocateFrontCustomMove(Allocator& alloc, const size_t size, const size_t align,
                              void (*moveConstructFn)(void* dst, void* src)) noexcept;

    [[nodiscard]] void* beforeFront(const size_t size);

  private:
    size_t len_ = 0;
    void* data_ = nullptr;
    size_t capacity_ = 0;
    void* alloc_ = nullptr;
};

template <typename T> class SY_API DynArrayUnmanaged final {
  public:
    DynArrayUnmanaged() = default;

    ~DynArrayUnmanaged() noexcept = default;

    void destroy(Allocator& alloc) noexcept;

    DynArrayUnmanaged(DynArrayUnmanaged&& other) noexcept = default;

    DynArrayUnmanaged& operator=(DynArrayUnmanaged&& other) = delete;

    void moveAssign(DynArrayUnmanaged&& other, Allocator& alloc) noexcept;

    DynArrayUnmanaged(const DynArrayUnmanaged& other) = delete;

    [[nodiscard]] static Result<DynArrayUnmanaged, AllocErr> copyConstruct(const DynArrayUnmanaged& other,
                                                                           Allocator& alloc) noexcept;

    DynArrayUnmanaged& operator=(const DynArrayUnmanaged& other) = delete;

    [[nodiscard]] Result<void, AllocErr> copyAssign(const DynArrayUnmanaged& other, Allocator& alloc) noexcept;

    [[nodiscard]] size_t len() const { return this->inner_.len(); }

    [[nodiscard]] const T& at(size_t index) const;

    [[nodiscard]] T& at(size_t index);

    [[nodiscard]] const T& operator[](size_t index) const { return this->at(index); }

    [[nodiscard]] T& operator[](size_t index) { return this->at(index); }

    [[nodiscard]] const T* data() const { return reinterpret_cast<const T*>(this->inner_.data()); }

    [[nodiscard]] T* data() { return reinterpret_cast<T*>(this->inner_.data()); }

    [[nodiscard]] Result<void, AllocErr> push(T&& element, Allocator& alloc) noexcept;

    [[nodiscard]] Result<void, AllocErr> push(const T& element, Allocator& alloc) noexcept;

    [[nodiscard]] Result<void, AllocErr> pushFront(T&& element, Allocator& alloc) noexcept;

    [[nodiscard]] Result<void, AllocErr> pushFront(const T& element, Allocator& alloc) noexcept;

    [[nodiscard]] Result<void, AllocErr> insertAt(T&& element, Allocator& alloc, size_t index) noexcept;

    [[nodiscard]] Result<void, AllocErr> insertAt(const T& element, Allocator& alloc, size_t index) noexcept;

    [[nodiscard]] void removeAt(size_t index) noexcept;

  private:
    constexpr static detail::DestructFn elementDestruct = detail::makeDestructor<T>();

  private:
    RawDynArrayUnmanaged inner_;
};

/// Dynamically resizable array. Grows to fit elements you push into it.
template <typename T> class DynArray {
    T* _data = nullptr;
    size_t _length = 0;
    size_t _capacity = 0;

  public:
    DynArray() = default;
    ~DynArray() {
        if (_data == nullptr) {
            return;
        }
        for (size_t i = 0; i < _length; i++) {
            _data[i].~T();
        }
        delete[] _data;
        _data = nullptr;
    };
    void push(const T& v) {
        if (_length == _capacity) {
            resize();
        }
        _data[_length] = v;
        _length++;
    }
    void push(T&& v) {
        // object has pointer and type t is another dynarray instance
        if (_length == _capacity) {
            resize();
        }
        _data[_length] = std::move(v);
        _length++;
    };
    const T& operator[](const size_t i) const {
        // return data indiv
        return _data[i];
    };
    T& operator[](const size_t i) { return _data[i]; };

    size_t len() const { return _length; }

  private:
    void resize() {
        const size_t newCapacity = calculateNewCapacity(_capacity);
        T* temp = new T[newCapacity];
        for (size_t i = 0; i < _length; i++) {
            temp[i] = _data[i];
        }
        delete[] _data;
        _data = temp;
        _capacity = newCapacity;
    }
    static size_t calculateNewCapacity(size_t currentCapacity) {
        if (currentCapacity == 0) {
            currentCapacity = 1;
        }
        currentCapacity *= 2;
        return currentCapacity;
    }
};

template <typename T> inline void DynArrayUnmanaged<T>::destroy(Allocator& alloc) noexcept {
    this->inner_.destroy(alloc, elementDestruct, sizeof(T), alignof(T));
}

template <typename T>
inline void DynArrayUnmanaged<T>::moveAssign(DynArrayUnmanaged&& other, Allocator& alloc) noexcept {
    this->inner_.moveAssign(std::move(other.inner_), elementDestruct, alloc, sizeof(T), alignof(T));
}

template <typename T>
inline Result<DynArrayUnmanaged<T>, AllocErr> DynArrayUnmanaged<T>::copyConstruct(const DynArrayUnmanaged& other,
                                                                                  Allocator& alloc) noexcept {
    auto result = RawDynArrayUnmanaged::copyConstruct(other.inner_, alloc, detail::makeCopyConstructor<T>(), sizeof(T),
                                                      alignof(T));
    if (result.hasErr())
        return Error(AllocErr::OutOfMemory);

    DynArrayUnmanaged self;
    auto _ = new (&self.inner_) RawDynArrayUnmanaged(std::move(result.value()));
    (void)_;
    return self;
}

template <typename T>
inline Result<void, AllocErr> DynArrayUnmanaged<T>::copyAssign(const DynArrayUnmanaged& other,
                                                               Allocator& alloc) noexcept {
    return this->inner_.copyAssign(other.inner_, alloc, elementDestruct, detail::makeCopyConstructor<T>(), sizeof(T),
                                   alignof(T));
}

template <typename T> inline const T& DynArrayUnmanaged<T>::at(size_t index) const {
    return *reinterpret_cast<const T*>(this->inner_.at(index, sizeof(T)));
}

template <typename T> inline T& DynArrayUnmanaged<T>::at(size_t index) {
    return *reinterpret_cast<T*>(this->inner_.at(index, sizeof(T)));
}

template <typename T> inline Result<void, AllocErr> DynArrayUnmanaged<T>::push(T&& element, Allocator& alloc) noexcept {
    if constexpr (std::is_trivially_copyable_v<T>) {
        return this->inner_.push(&element, alloc, sizeof(T), alignof(T));
    } else {
        return this->inner_.pushCustomMove(&element, alloc, sizeof(T), alignof(T), detail::makeMoveConstructor<T>());
    }
}

template <typename T>
inline Result<void, AllocErr> DynArrayUnmanaged<T>::push(const T& element, Allocator& alloc) noexcept {
    T elem = element;
    return this->push(std::move(elem), alloc);
}

template <typename T>
inline Result<void, AllocErr> DynArrayUnmanaged<T>::pushFront(T&& element, Allocator& alloc) noexcept {
    if constexpr (std::is_trivially_copyable_v<T>) {
        return this->inner_.pushFrom(&element, alloc, sizeof(T), alignof(T));
    } else {
        return this->inner_.pushFrontCustomMove(&element, alloc, sizeof(T), alignof(T),
                                                detail::makeMoveConstructor<T>());
    }
}

template <typename T>
inline Result<void, AllocErr> DynArrayUnmanaged<T>::pushFront(const T& element, Allocator& alloc) noexcept {
    T elem = element;
    return this->pushFront(std::move(elem), alloc);
}

template <typename T>
inline Result<void, AllocErr> DynArrayUnmanaged<T>::insertAt(T&& element, Allocator& alloc, size_t index) noexcept {
    if constexpr (std::is_trivially_copyable_v<T>) {
        return this->inner_.insertAt(&element, alloc, index, sizeof(T), alignof(T));
    } else {
        return this->inner_.insertAtCustomMove(&element, alloc, index, sizeof(T), alignof(T),
                                               detail::makeMoveConstructor<T>());
    }
}

template <typename T>
inline Result<void, AllocErr> DynArrayUnmanaged<T>::insertAt(const T& element, Allocator& alloc,
                                                             size_t index) noexcept {
    T elem = element;
    return this->insertAt(std::move(elem), alloc, index);
}

template <typename T> inline void DynArrayUnmanaged<T>::removeAt(size_t index) noexcept {
    if constexpr (std::is_trivially_copyable_v<T>) {
        this->inner_.removeAt(index, elementDestruct, sizeof(T));
    } else {
        this->inner_.removeAtCustomMove(index, elementDestruct, sizeof(T), detail::makeMoveConstructor<T>());
    }
}

}; // namespace sy

#endif // SY_TYPES_ARRAY_DYNAMIC_ARRAY_HPP_