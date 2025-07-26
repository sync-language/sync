//! API
#pragma once
#ifndef SY_MEM_ALLOC_EXPECT_HPP_
#define SY_MEM_ALLOC_EXPECT_HPP_

#include "../core.h"
#include <type_traits>
#include <new>

namespace sy {
    namespace detail {
        extern void debugAssertNonNull(void* ptr);
        extern void debugAssertHasVal(bool hasVal);
    }

    template<typename T, typename Enable = void> 
    class SY_API AllocExpect SY_CLASS_FINAL {};

    template<typename T>
    class SY_API AllocExpect<typename T, std::enable_if_t<std::is_same_v<T, void>>> SY_CLASS_FINAL {
    public:
        AllocExpect() = default;
        AllocExpect(std::true_type) : hasVal_(true) {}

        [[nodiscard]] bool hasValue() const { return this->hasVal_; }
        [[nodiscard]] operator bool() const { return this->hasVal_; }

    private:
        bool hasVal_ = false;
    };

    template<typename T>
    class SY_API AllocExpect
        <typename T, std::enable_if_t<std::is_pointer_v<T>>>
    SY_CLASS_FINAL {
    public:
        AllocExpect() = default;
        AllocExpect(T ptr) : ptr_(ptr) {}

        [[nodiscard]] bool hasValue() const { return this->ptr_ != nullptr; }
        [[nodiscard]] operator bool() const { return this->ptr_ != nullptr; }

        T value() const {
            detail::debugAssertNonNull(this->ptr_);
            return ptr_;
        }

    private:
        mutable T ptr_ = nullptr;
    };

    template<typename T>
    class SY_API AllocExpect
        <typename T, std::enable_if_t<!std::is_same_v<T, void> && !std::is_pointer_v<T>>>
    SY_CLASS_FINAL {
    public:
        AllocExpect() = default;
        AllocExpect(T&& val) : val_(std::move(val)), hasVal_(true) {}
        AllocExpect(const T& val) : val_(val), hasVal_(true) {}
        ~AllocExpect() {
            if(hasVal_) {
                val_.val.~T();
            }
            hasVal_ = false;
        }

        [[nodiscard]] bool hasValue() const { return this->hasVal_; }
        [[nodiscard]] operator bool() const { return this->hasVal_; }

        [[nodiscard]] const T& value() const {
            detail::debugAssertHasVal(this->hasVal_);
            return val_.val;
        }
        [[nodiscard]] T& value() {
            detail::debugAssertHasVal(this->hasVal_);
            return val_.val;
        }
        [[nodiscard]] T&& take() {
            detail::debugAssertHasVal(this->hasVal_);
            this->hasVal_ = false;
            return val_.val;
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

        Val val_;
        bool hasVal_;
    };
}

#endif // SY_MEM_ALLOC_EXPECT_HPP_
