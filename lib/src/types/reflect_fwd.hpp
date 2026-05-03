//! API
#pragma once
#ifndef SY_TYPES_REFLECT_FWD_HPP_
#define SY_TYPES_REFLECT_FWD_HPP_

namespace sy {
class Type;

template <typename T, typename Enable = void> struct Reflect {
    static constexpr const Type* get() noexcept {
        static_assert(sizeof(T) == 0, "sy::Reflect must be specialized for this type");
        return nullptr;
    }

    // You may also define the following two functions two control the presence of reference types.
    // They do not have to be constexpr.
    // static constexpr const Type* constRef() noexcept { ... }
    // static constexpr const Type* mutRef() noexcept { ... }
};

} // namespace sy

#endif // SY_TYPES_REFLECT_FWD_HPP_
