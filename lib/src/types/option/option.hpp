//! API
#pragma once
#ifndef SY_TYPES_OPTION_OPTION_HPP_
#define SY_TYPES_OPTION_OPTION_HPP_

#include "../../core.h"
#include <new>
#include <type_traits>
#include <utility>

#if _WIN32 // allow non dll-interface types to be used here
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace sy {
namespace detail {
void debugAssertPtrNotNull(const void* ptr, const char* errMsg);
void debugAssertOptionHasValue(bool hasVal, const char* errMsg);
} // namespace detail

template <typename T, typename Enable = void> class SY_API Option final {};

template <typename T> class SY_API Option<T, std::enable_if_t<std::is_pointer_v<T>>> final {
  public:
    Option() = default;
    Option(T ptr) : ptr_(ptr) {}
    [[nodiscard]] bool hasValue() const { return ptr_ != nullptr; }
    [[nodiscard]] operator bool() const { return this->hasValue(); }
    [[nodiscard]] bool operator!() const { return ptr_ == nullptr; }
    [[nodiscard]] T value() {
        detail::debugAssertPtrNotNull(ptr_, "Bad sy::Option access: Optional pointer is nullptr");
        return ptr_;
    }
    [[nodiscard]] const T value() const {
        detail::debugAssertPtrNotNull(ptr_, "Bad sy::Option access: Optional pointer is nullptr");
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
    Option() = default;
    Option(T ref) : ptr_(&ref) {}
    [[nodiscard]] bool hasValue() const { return ptr_ != nullptr; }
    [[nodiscard]] operator bool() const { return this->hasValue(); }
    [[nodiscard]] bool operator!() const { return ptr_ == nullptr; }
    [[nodiscard]] T value() {
        detail::debugAssertPtrNotNull(ptr_, "Bad sy::Option access: Optional reference is empty");
        return *ptr_;
    }
    [[nodiscard]] const T value() const {
        detail::debugAssertPtrNotNull(ptr_, "Bad sy::Option access: Optional reference is empty");
        return *ptr_;
    }

  private:
    PtrT ptr_ = nullptr;
};

template <typename T> class SY_API Option<T, std::enable_if_t<!std::is_reference_v<T> && !std::is_pointer_v<T>>> final {
  public:
    Option() = default;
    Option(T&& val) noexcept : hasVal_(true), val_(std::move(val)) {}
    Option(const T& val) : hasVal_(true), val_(val) {}
    ~Option() noexcept {
        if (hasVal_) {
            val_.val.~T();
        }
        hasVal_ = false;
    }
    Option(Option&& other) noexcept : hasVal_(other.hasVal_) {
        if (hasVal_) {
            new (&val_.val) T(std::move(other.val_.val));
            other.hasVal_ = false;
        }
    }
    Option& operator=(Option&& other) noexcept {
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
    Option(const Option& other) : hasVal_(other.hasVal_) {
        if (hasVal_) {
            new (&val_.val) T(other.val_.val);
        }
    }
    Option& operator=(const Option& other) noexcept {
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
    [[nodiscard]] bool hasValue() const { return hasVal_; }
    [[nodiscard]] operator bool() const { return this->hasValue(); }
    [[nodiscard]] bool operator!() const { return !hasVal_; }
    [[nodiscard]] T& value() {
        detail::debugAssertOptionHasValue(hasVal_, "Bad sy::Option access: Optional value is empty");
        return val_.val;
    }
    [[nodiscard]] const T& value() const {
        detail::debugAssertOptionHasValue(hasVal_, "Bad sy::Option access: Optional value is empty");
        return val_.val;
    }
    [[nodiscard]] T&& take() {
        detail::debugAssertOptionHasValue(hasVal_, "Bad sy::Option access: Optional value is empty");
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

#if _WIN32
#pragma warning(pop)
#endif

#endif // SY_TYPES_OPTION_OPTION_HPP_