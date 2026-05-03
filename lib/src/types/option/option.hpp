//! API
#pragma once
#ifndef SY_TYPES_OPTION_OPTION_HPP_
#define SY_TYPES_OPTION_OPTION_HPP_

#include "../../core/core.h"
#include <new>
#include <optional> // TODO only here for nullopt, replace with custom one
#include <type_traits>
#include <utility>

#if _MSC_VER // allow non dll-interface types to be used here
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace sy {
namespace detail {
void SY_API debugAssertPtrNotNull(const void* ptr);
void SY_API debugAssertOptionHasValue(bool hasVal);
} // namespace detail

template <typename T, typename Enable = void> class SY_API Option final {};

template <typename T> class SY_API Option<T, std::enable_if_t<std::is_pointer_v<T>>> final {
  public:
    constexpr Option() = default;
    constexpr Option(std::nullptr_t) {};
    constexpr Option(std::nullopt_t) {};
    constexpr Option(T ptr) : ptr_(ptr) {}
    [[nodiscard]] constexpr bool hasValue() const { return ptr_ != nullptr; }
    [[nodiscard]] constexpr operator bool() const { return this->hasValue(); }
    [[nodiscard]] constexpr bool operator!() const { return ptr_ == nullptr; }
    [[nodiscard]] constexpr T value() {
        if (!std::is_constant_evaluated()) {
            detail::debugAssertPtrNotNull(ptr_);
        }
        return ptr_;
    }
    [[nodiscard]] constexpr const T value() const {
        if (!std::is_constant_evaluated()) {
            detail::debugAssertPtrNotNull(ptr_);
        }
        return ptr_;
    }

  private:
    T ptr_ = nullptr;
};

template <typename T> class SY_API Option<T, std::enable_if_t<std::is_reference_v<T>>> final {
  private:
    using NonReference = std::remove_reference_t<T>;
    using PtrT = std::conditional_t<std::is_const_v<T>, const NonReference*, NonReference*>;

  public:
    constexpr Option() = default;
    constexpr Option(std::nullptr_t) {};
    constexpr Option(std::nullopt_t) {};
    constexpr Option(T ref) : ptr_(&ref) {}
    [[nodiscard]] constexpr bool hasValue() const { return ptr_ != nullptr; }
    [[nodiscard]] constexpr operator bool() const { return this->hasValue(); }
    [[nodiscard]] constexpr bool operator!() const { return ptr_ == nullptr; }
    [[nodiscard]] constexpr T value() {
        if (!std::is_constant_evaluated()) {
            detail::debugAssertPtrNotNull(ptr_);
        }
        return *ptr_;
    }
    [[nodiscard]] constexpr const T value() const {
        if (!std::is_constant_evaluated()) {
            detail::debugAssertPtrNotNull(ptr_);
        }
        return *ptr_;
    }

  private:
    PtrT ptr_ = nullptr;
};

template <typename T>
class SY_API Option<T, std::enable_if_t<!std::is_reference_v<T> && !std::is_pointer_v<T>>> final {
  public:
    constexpr Option() = default;
    constexpr Option(std::nullopt_t) {};
    constexpr Option(T&& val) noexcept : hasVal_(true), val_(std::move(val)) {}
    constexpr Option(const T& val) : hasVal_(true), val_(val) {}
    constexpr ~Option() noexcept {
        if (hasVal_) {
            val_.val.~T();
        }
        hasVal_ = false;
    }
    constexpr Option(Option&& other) noexcept : hasVal_(other.hasVal_) {
        if (hasVal_) {
            new (&val_.val) T(std::move(other.val_.val));
            other.hasVal_ = false;
        }
    }
    constexpr Option& operator=(Option&& other) noexcept {
        if (this == &other)
            return *this;

        if (hasVal_) {
            val_.val.~T();
        }
        hasVal_ = other.hasVal_;
        if (hasVal_) {
            new (&val_.val) T(std::move(other.val_.val));
            other.hasVal_ = false;
        }
        return *this;
    }
    constexpr Option(const Option& other) : hasVal_(other.hasVal_) {
        if (hasVal_) {
            new (&val_.val) T(other.val_.val);
        }
    }
    constexpr Option& operator=(const Option& other) noexcept {
        if (this == &other)
            return *this;

        if (hasVal_) {
            val_.val.~T();
        }
        hasVal_ = other.hasVal_;
        if (hasVal_) {
            new (&val_.val) T(other.val_.val);
        }
        return *this;
    }
    [[nodiscard]] constexpr bool hasValue() const { return hasVal_; }
    [[nodiscard]] constexpr operator bool() const { return this->hasValue(); }
    [[nodiscard]] constexpr bool operator!() const { return !hasVal_; }
    [[nodiscard]] constexpr T& value() {
        if (!std::is_constant_evaluated()) {
            detail::debugAssertOptionHasValue(hasVal_);
        }
        return val_.val;
    }
    [[nodiscard]] constexpr const T& value() const {
        if (!std::is_constant_evaluated()) {
            detail::debugAssertOptionHasValue(hasVal_);
        }
        return val_.val;
    }
    [[nodiscard]] constexpr T&& take() {
        if (!std::is_constant_evaluated()) {
            detail::debugAssertOptionHasValue(hasVal_);
        }
        this->hasVal_ = false;
        return std::move(val_.val);
    }

  private:
    union SY_API Val {
        uint8_t empty;
        T val;
        Val() : empty(0) {}
        Val(T&& inVal) noexcept : val(std::move(inVal)) {}
        Val(const T& inVal) noexcept : val(inVal) {}
        ~Val() noexcept {}
    };

    bool hasVal_ = false;
    Val val_;
};
} // namespace sy

#if _MSC_VER
#pragma warning(pop)
#endif

#endif // SY_TYPES_OPTION_OPTION_HPP_
