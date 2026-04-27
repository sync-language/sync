//! API
#pragma once
#ifndef SY_TYPES_REFLECT_FWD_HPP_
#define SY_TYPES_REFLECT_FWD_HPP_

namespace sy {
class Type;

template <typename T, typename Enable = void> struct Reflect {
    static const Type* get() noexcept {
        static_assert(sizeof(T) == 0, "sy::Reflect must be specialized for this type");
        return nullptr;
    }
};
} // namespace sy

#endif // SY_TYPES_REFLECT_FWD_HPP_
