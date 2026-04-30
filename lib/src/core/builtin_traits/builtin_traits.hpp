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
    Option<const Function<void(void* dst, const void* src)>*> elementWiseAtomicClone;
    Option<const Function<void(void* dst, void* src)>*> elementWiseAtomicMove;

    template <typename T> static const Function<void(void* dst, const void* src)>* makeClone() {
        static Function<void(void* dst, const void* src)> func = +[](void* dst, const void* src) {
            T* tDst = reinterpret_cast<T*>(dst);
            const T* tSrc = reinterpret_cast<const T*>(src);
            new (tDst) T(*tSrc);
        };
        return &func;
    }

    template <typename T>
    static const Function<bool(const void* lhs, const void* rhs)>* makeEqualityFunction() {
        static Function<bool(const void* lhs, const void* rhs)> func =
            +[](const void* lhs, const void* rhs) -> bool {
            const T* tLhs = reinterpret_cast<const T*>(lhs);
            const T* tRhs = reinterpret_cast<const T*>(rhs);
            return *tLhs == *tRhs;
        };
        return &func;
    }

    template <typename T> static const Function<size_t(const void* obj)>* makeHashFunction() {
        static Function<size_t(const void* obj)> func = +[](const void* obj) -> size_t {
            const T* tObj = reinterpret_cast<const T*>(obj);
            std::hash<T> h;
            return h(*tObj);
        };
        return &func;
    }

    template <typename T>
    static Function<Ordering(const void* lhs, const void* rhs)>* makeCompareFunction() {
        static Function<Ordering(const void* lhs, const void* rhs)> func =
            +[](const void* lhs, const void* rhs) -> Ordering {
            const T* tLhs = reinterpret_cast<const T*>(lhs);
            const T* tRhs = reinterpret_cast<const T*>(rhs);
            bool equal = (*tLhs) == (*tRhs);
            if (equal) {
                return Ordering::Equal;
            } else if ((*tLhs) < (*tRhs)) {
                return Ordering::Less;
            } else {
                return Ordering::Greater;
            }
        };
        return &func;
    }
};

} // namespace sy

#endif // _SY_CORE_BUILTIN_TRAITS_BUILTIN_TRAITS_HPP_
