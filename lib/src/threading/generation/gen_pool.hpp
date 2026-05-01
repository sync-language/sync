//! API
#pragma once
#ifndef SY_THREADING_GENERATION_GEN_POOL_HPP_
#define SY_THREADING_GENERATION_GEN_POOL_HPP_

#include "../../core/core.h"
#include "../../mem/allocator.hpp"
#include "../../program/program_error.hpp"
#include "../../types/reflect_fwd.hpp"
#include "../../types/result/result.hpp"
#include "../../util/move_and_leak.hpp"
#include <cstring>

namespace sy {
class Type;
class GenPool;
template <typename T> class GenOwner;
template <typename T> class GenRef;

namespace internal {
struct GenTypedPool;
struct GenPoolImpl;
struct Test_GenPool;
struct Test_GenOwner;
struct Test_GenRef;
} // namespace internal

/// Generational reference pool, supporting atomic data access.
/// Supports two access modes, the first being clone / swap, the second being readonly/readwrite
/// locked iteration.
class GenPool {
  public:
    GenPool() = default;

    GenPool(GenPool&& other) noexcept;

    GenPool& operator=(GenPool&& other) noexcept;

    ~GenPool() noexcept;

    GenPool(const GenPool& other) = delete;

    GenPool& operator=(const GenPool& other) = delete;

    static Result<GenPool, AllocErr> init(Allocator allocator = Allocator()) noexcept;

    template <typename T> Result<GenOwner<T>, AllocErr> add(T obj) noexcept;

  private:
    friend struct internal::GenTypedPool;
    friend struct internal::Test_GenPool;

    internal::GenPoolImpl* impl_ = nullptr;
};

template <typename T> class GenOwner {
  public:
    GenOwner() = default;

    GenOwner(GenOwner&& other) noexcept;

    GenOwner& operator=(GenOwner&& other) noexcept;

    ~GenOwner() noexcept;

    GenOwner(const GenOwner& other) = delete;

    GenOwner& operator=(const GenOwner& other) = delete;

    Result<T, ProgramError> load() const noexcept;

    Result<void, ProgramError> store(T obj) noexcept;

    GenRef<T> ref() noexcept;

  private:
    friend class GenPool;
    friend struct internal::Test_GenOwner;

    uint64_t gen_;
    void* chunk_ = nullptr;
    uint32_t objectIndex_ = 0;
};

/// Explicitly read-only to prevent TOCTOU races for the generational reference while keeping it
/// lock free. Kinda unavoidable.
/// @tparam T
template <typename T> class GenRef {
  public:
    GenRef() = default;
    GenRef(const GenRef& other) = default;
    GenRef& operator=(const GenRef& other) = default;
    GenRef(GenRef&& other) noexcept = default;
    GenRef& operator=(GenRef&& other) noexcept = default;
    ~GenRef() noexcept = default;

    Result<T, ProgramError> load() const noexcept;

    Result<void, ProgramError> store(T obj) noexcept;

  private:
    template <typename U> friend class GenOwner;
    friend struct internal::Test_GenRef;

    uint64_t gen_;
    void* chunk_ = nullptr;
    uint32_t objectIndex_ = 0;
};

namespace internal {
SY_API void ensureNoProgramError(int err);
SY_API int sy_gen_pool_add_impl(GenPool* self, void* obj, const sy::Type* objType, void* outGenRef);
SY_API int sy_gen_owner_destroy_impl(void* self);
SY_API int sy_gen_owner_load_impl(void* self, void* outObj);
SY_API int sy_gen_ref_load_impl(void* self, void* outObj);
SY_API int sy_gen_owner_store_impl(void* self, void* obj);
SY_API int sy_gen_ref_store_impl(void* self, void* obj);
} // namespace internal

template <typename T> inline Result<GenOwner<T>, AllocErr> GenPool::add(T obj) noexcept {
    GenOwner<T> ref{};
    const sy::Type* typeOfObj = sy::Reflect<T>::get();

    const int err = internal::sy_gen_pool_add_impl(this, &obj, typeOfObj, &ref);
    if (err == 0) {
        return ref;
    }

    internal::moveAndLeak(std::move(obj));
    return Error(static_cast<AllocErr>(err));
}

template <typename T>
GenOwner<T>::GenOwner(GenOwner&& other) noexcept
    : gen_(other.gen_), chunk_(other.chunk_), objectIndex_(other.objectIndex_) {
    other.gen_ = 0;
    other.chunk_ = nullptr;
    other.objectIndex_ = 0;
}

template <typename T> GenOwner<T>& GenOwner<T>::operator=(GenOwner&& other) noexcept {
    if (this != &other) {
        internal::ensureNoProgramError(
            internal::sy_gen_owner_destroy_impl(static_cast<void*>(this)));
        this->gen_ = other.gen_;
        this->chunk_ = other.chunk_;
        this->objectIndex_ = other.objectIndex_;
        other.gen_ = 0;
        other.chunk_ = nullptr;
        other.objectIndex_ = 0;
    }
    return *this;
}

template <typename T> inline GenOwner<T>::~GenOwner() noexcept {
    if (this->gen_ == 0) {
        return;
    }

    internal::ensureNoProgramError(internal::sy_gen_owner_destroy_impl(static_cast<void*>(this)));

    this->gen_ = 0;
    this->chunk_ = nullptr;
    this->objectIndex_ = 0;
}

template <typename T> inline Result<T, ProgramError> GenOwner<T>::load() const noexcept {
    T out{};
    const int err = internal::sy_gen_owner_load_impl(this, &out);
    if (err == 0) {
        return out;
    }

    return Error(static_cast<ProgramError>(err));
}

template <typename T> inline Result<void, ProgramError> GenOwner<T>::store(T obj) noexcept {
    const int err = internal::sy_gen_owner_store_impl(this, &obj);
    if (err == 0) {
        return {};
    }

    internal::moveAndLeak(std::move(obj));
    return Error(static_cast<ProgramError>(err));
}

template <typename T> inline GenRef<T> GenOwner<T>::ref() noexcept {
    GenRef<T> ref{};
    ref.gen_ = this->gen_;
    ref.chunk_ = this->chunk_;
    ref.objectIndex_ = this->objectIndex_;
    return ref;
}

template <typename T> inline Result<T, ProgramError> GenRef<T>::load() const noexcept {
    T out{};
    const int err = internal::sy_gen_ref_load_impl(this, &out);
    if (err == 0) {
        return out;
    }

    return Error(static_cast<ProgramError>(err));
}

template <typename T> inline Result<void, ProgramError> GenRef<T>::store(T obj) noexcept {
    const int err = internal::sy_gen_ref_store_impl(this, &obj);
    if (err == 0) {
        return {};
    }

    internal::moveAndLeak(std::move(obj));
    return Error(static_cast<ProgramError>(err));
}

} // namespace sy

#endif // SY_THREADING_GENERATION_GEN_POOL_HPP_
