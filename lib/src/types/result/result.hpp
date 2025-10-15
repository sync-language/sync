//! API
#pragma once
#ifndef SY_TYPES_RESULT_RESULT_HPP_
#define SY_TYPES_RESULT_RESULT_HPP_

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
void SY_API debugAssertResultIsOk(bool isOk, const char* errMsg);
void SY_API debugAssertResultIsErr(bool isErr, const char* errMsg);
} // namespace detail
template <typename T, typename E, typename Enable = void> class Result final {};

template <typename E> class Error final {
    static_assert(!std::is_same_v<E, void>, "Error type <E> may not be void");
    E err_;

  public:
    Error(E&& err) : err_(std::move(err)) {}
    Error(const E& err) : err_(err) {}
    E&& take() && { return std::move(err_); }
};

template <typename T, typename E> class Result<T, E, std::enable_if_t<std::is_same_v<T, void>>> final {
    static_assert(!std::is_same_v<E, void>, "Result Error type <E> may not be void");

  public:
    Result() = default;
    Result(Error<E>&& inErr) : hasErr_(true), val_(std::move(inErr).take()) {}
    ~Result() {
        if (hasErr_) {
            val_.err.~E();
        }
        hasErr_ = false;
    }
    Result(Result&& other) : hasErr_(other.hasErr_) {
        if (hasErr_) {
            new (&val_.err) E(std::move(other.val_.err));
            other.hasErr_ = false;
        }
    }
    Result& operator=(Result&& other) {
        if (this == &other)
            return *this;

        if (hasErr_) {
            val_.err.~E();
        }
        hasErr_ = other.hasErr_;
        if (hasErr_) {
            new (&val_.err) E(std::move(other.val_.err));
            other.hasErr_ = false;
        }
    }
    Result(const Result& other) : hasErr_(other.hasErr_) {
        if (hasErr_) {
            new (&val_.err) E(other.val_.err);
        }
    }
    Result& operator=(const Result& other) {
        if (this == &other)
            return *this;

        if (hasErr_) {
            val_.err.~E();
        }
        hasErr_ = other.hasErr_;
        if (hasErr_) {
            new (&val_.err) E(other.val_.err);
        }
    }

    [[nodiscard]] bool hasValue() const { return !hasErr_; }
    [[nodiscard]] bool hasErr() const { return hasErr_; }
    [[nodiscard]] operator bool() const { return this->hasValue(); }
    [[nodiscard]] bool operator!() const { return hasErr_; }

    [[nodiscard]] E& err() {
        detail::debugAssertResultIsErr(hasErr_, "Bad sy::Result access: Result is not an error");
        return val_.err;
    }

    [[nodiscard]] const E& err() const {
        detail::debugAssertResultIsErr(hasErr_, "Bad sy::Result access: Result is not an error");
        return val_.err;
    }

    [[nodiscard]] E&& takeErr() {
        detail::debugAssertResultIsErr(hasErr_, "Bad sy::Result access: Result is not an error");
        this->hasErr_ = false;
        return std::move(val_.err);
    }

  private:
    union SY_API Val {
        uint8_t empty;
        E err;
        Val() : empty(0) {}
        Val(E&& inErr) noexcept : err(std::move(inErr)) {}
        Val(const E& inErr) noexcept : err(inErr) {}
        ~Val() noexcept {}
    };

    bool hasErr_ = false;
    Val val_;
};

template <typename T, typename E> class Result<T, E, std::enable_if_t<!std::is_same_v<T, void>>> final {
    static_assert(!std::is_same_v<E, void>, "Result Error type <E> may not be void");

  public:
    Result(T&& inOk) : hasErr_(false), val_(std::move(inOk)) {}
    Result(const T& inOk) : hasErr_(false), val_(inOk) {}
    Result(Error<E>&& inErr) : hasErr_(true), val_(std::move(inErr)) {}
    ~Result() {
        if (hasErr_) {
            val_.err.~E();
        } else {
            val_.ok.~T();
        }
    }
    Result(Result&& other) : hasErr_(other.hasErr_) {
        if (hasErr_) {
            new (&val_.err) E(std::move(other.val_.err));
            other.hasErr_ = false;
        }
    }
    Result& operator=(Result&& other) {
        if (this == &other)
            return *this;

        if (hasErr_) {
            val_.err.~E();
        }
        hasErr_ = other.hasErr_;
        if (hasErr_) {
            new (&val_.err) E(std::move(other.val_.err));
            other.hasErr_ = false;
        }
    }
    Result(const Result& other) : hasErr_(other.hasErr_) {
        if (hasErr_) {
            new (&val_.err) E(other.val_.err);
        }
    }
    Result& operator=(const Result& other) {
        if (this == &other)
            return *this;

        if (hasErr_) {
            val_.err.~E();
        }
        hasErr_ = other.hasErr_;
        if (hasErr_) {
            new (&val_.err) E(other.val_.err);
        }
    }

    [[nodiscard]] bool hasValue() const { return !hasErr_; }
    [[nodiscard]] bool hasErr() const { return hasErr_; }
    [[nodiscard]] operator bool() const { return this->hasValue(); }
    [[nodiscard]] bool operator!() const { return hasErr_; }

    [[nodiscard]] T& value() {
        detail::debugAssertResultIsOk(!hasErr_, "Bad sy::Result access: Result is an error");
        return val_.ok;
    }

    [[nodiscard]] const T& value() const {
        detail::debugAssertResultIsOk(!hasErr_, "Bad sy::Result access: Result is an error");
        return val_.ok;
    }

    [[nodiscard]] T&& takeValue() {
        detail::debugAssertResultIsOk(!hasErr_, "Bad sy::Result access: Result is an error");
        return std::move(val_.ok);
    }

    [[nodiscard]] E& err() {
        detail::debugAssertResultIsErr(hasErr_, "Bad sy::Result access: Result is not an error");
        return val_.err;
    }

    [[nodiscard]] const E& err() const {
        detail::debugAssertResultIsErr(hasErr_, "Bad sy::Result access: Result is not an error");
        return val_.err;
    }

    [[nodiscard]] E&& takeErr() {
        detail::debugAssertResultIsErr(hasErr_, "Bad sy::Result access: Result is not an error");
        return std::move(val_.err);
    }

  private:
    union Val {
        T ok;
        E err;
        Val() noexcept {}
        Val(T&& inOk) noexcept : ok(std::move(inOk)) {}
        Val(const T& inOk) noexcept : ok(inOk) {}
        Val(Error<E>&& inErr) noexcept : err(std::move(inErr).take()) {}
        ~Val() noexcept {}
    };

    bool hasErr_ = false;
    Val val_;
};

} // namespace sy

#if _WIN32
#pragma warning(pop)
#endif

#endif // SY_TYPES_RESULT_RESULT_HPP_