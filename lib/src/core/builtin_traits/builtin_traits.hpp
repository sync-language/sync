//! API
#pragma once
#ifndef _SY_CORE_BUILTIN_TRAITS_BUILTIN_TRAITS_HPP_
#define _SY_CORE_BUILTIN_TRAITS_BUILTIN_TRAITS_HPP_

#include "../../types/function/function.hpp"
#include "../../types/option/option.hpp"
#include "../../types/ordering/ordering.hpp"
#include "../core.h"
#include <new>

namespace sy {
struct BuiltInCoherentTraits {
    Option<const Function<void(void* dst, const void* src)>*> clone;
    Option<const Function<bool(const void* lhs, const void* rhs)>*> equal;
    Option<const Function<size_t(const void* obj)>*> hash;
    Option<const Function<Ordering(const void* lhs, const void* rhs)>*> compare;
    Option<const Function<void(void* self)>*> elementWiseAtomicDestroy;
    Option<const Function<void(void* dst, const void* src)>*> elementWiseAtomicLoad;
    Option<const Function<void(void* dst, const void* src)>*> elementWiseAtomicStore;

    template <typename T>
    static constexpr Function<void(void* dst, const void* src)> CLONE_FN_OF =
        +[](void* dst, const void* src) {
            T* tDst = reinterpret_cast<T*>(dst);
            const T* tSrc = reinterpret_cast<const T*>(src);
            new (tDst) T(*tSrc);
        };

    template <typename T>
    static constexpr Function<bool(const void* lhs, const void* rhs)> EQUAL_FN_OF =
        +[](const void* lhs, const void* rhs) -> bool {
        const T* tLhs = reinterpret_cast<const T*>(lhs);
        const T* tRhs = reinterpret_cast<const T*>(rhs);
        return *tLhs == *tRhs;
    };

    template <typename T>
    static constexpr Function<size_t(const void* obj)> HASH_FN_OF = +[](const void* obj) -> size_t {
        const T* tObj = reinterpret_cast<const T*>(obj);
        std::hash<T> h;
        return h(*tObj);
    };

    template <typename T>
    static constexpr Function<Ordering(const void* lhs, const void* rhs)> COMPARE_FN_OF =
        +[](const void* lhs, const void* rhs) -> Ordering {
        const T* tLhs = reinterpret_cast<const T*>(lhs);
        const T* tRhs = reinterpret_cast<const T*>(rhs);
        bool eq = (*tLhs) == (*tRhs);
        if (eq) {
            return Ordering::Equal;
        } else if ((*tLhs) < (*tRhs)) {
            return Ordering::Less;
        } else {
            return Ordering::Greater;
        }
    };

    template <typename T>
    static constexpr const Function<void(void* dst, const void* src)>* makeClone() noexcept {
        return &CLONE_FN_OF<T>;
    }

    template <typename T>
    static constexpr const Function<bool(const void* lhs, const void* rhs)>*
    makeEqualityFunction() noexcept {
        return &EQUAL_FN_OF<T>;
    }

    template <typename T>
    static constexpr const Function<size_t(const void* obj)>* makeHashFunction() noexcept {
        return &HASH_FN_OF<T>;
    }

    template <typename T>
    static constexpr const Function<Ordering(const void* lhs, const void* rhs)>*
    makeCompareFunction() noexcept {
        return &COMPARE_FN_OF<T>;
    }
};

} // namespace sy

#endif // _SY_CORE_BUILTIN_TRAITS_BUILTIN_TRAITS_HPP_
