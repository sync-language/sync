//! API
#pragma once
#ifndef _SY_CORE_BUILTIN_TRAITS_BUILTIN_TRAITS_HPP_
#define _SY_CORE_BUILTIN_TRAITS_BUILTIN_TRAITS_HPP_

#include "../../types/anyerror/anyerror.hpp"
#include "../../types/function/function.hpp"
#include "../../types/option/option.hpp"
#include "../../types/ordering/ordering.hpp"
#include "../../types/result/result.hpp"
#include "../core.h"
#include "../exceptional.hpp"
#include <new>
#include <type_traits>
#include <utility>

namespace sy {
using NativeDestructorFn = void (*)(void* obj);
/// Most of the built-in trait functions cannot really fail, but this one absolutely can.
using NativeCloneFn = Result<void, Exceptional> (*)(void* dst, const void* src);
using NativeEqualFn = bool (*)(const void* lhs, const void* rhs);
using NativeHashFn = size_t (*)(const void* obj);
using NativeCompareFn = Ordering (*)(const void* lhs, const void* rhs);
using NativeElementWiseAtomicDestructorFn = void (*)(void* obj);
using NativeElementWiseAtomicLoadFn = void (*)(void* dst, const void* src);
using NativeElementWiseAtomicStoreFn = void (*)(void* dst, const void* src);

using BuiltInDestructorFn = Function<void(void* obj)>;
// The failure of this gets wrapped through `Function`'s `AnyError` return. See `NativeCloneFn`.
using BuiltInCloneFn = Function<void(void* dst, const void* src)>;
using BuiltInEqualFn = Function<bool(const void* lhs, const void* rhs)>;
using BuiltInHashFn = Function<size_t(const void* obj)>;
using BuiltInCompareFn = Function<Ordering(const void* lhs, const void* rhs)>;
using BuiltInElementWiseAtomicDestructorFn = Function<void(void* obj)>;
using BuiltInElementWiseAtomicLoadFn = Function<void(void* dst, const void* src)>;
using BuiltInElementWiseAtomicStoreFn = Function<void(void* dst, const void* src)>;

namespace internal {
template <typename T, typename = void> struct has_member_clone : std::false_type {};
template <typename T>
struct has_member_clone<T, std::void_t<decltype(std::declval<const T&>().clone())>>
    : std::true_type {};

constexpr Exceptional cloneErrToExceptional(AllocErr) noexcept { return Exceptional::OOM; }
constexpr Exceptional cloneErrToExceptional(Exceptional e) noexcept { return e; }
} // namespace internal

struct BuiltInCoherentTraits {
    /// If a type implements `sy::Result<T, sy::AllocErr> T::clone()` as a member function, it will
    /// be prioritized over the copy constructor. If a type implements `sy::Result<T,
    /// sy::Exceptional> T::clone()` as a member function, it will be prioritized over the copy
    /// constructor.
    Option<const BuiltInCloneFn*> clone;
    Option<const BuiltInEqualFn*> equal;
    Option<const BuiltInHashFn*> hash;
    Option<const BuiltInCompareFn*> compare;
    Option<const BuiltInElementWiseAtomicDestructorFn*> elementWiseAtomicDestroy;
    Option<const BuiltInElementWiseAtomicLoadFn*> elementWiseAtomicLoad;
    Option<const BuiltInElementWiseAtomicStoreFn*> elementWiseAtomicStore;

    template <typename T>
    static constexpr NativeCloneFn NATIVE_CLONE_FN_OF =
        +[](void* dst, const void* src) -> Result<void, Exceptional> {
        T* tDst = reinterpret_cast<T*>(dst);
        const T* tSrc = reinterpret_cast<const T*>(src);
        if constexpr (internal::has_member_clone<T>::value) {
            auto r = tSrc->clone();
            if (r.hasErr()) {
                return Error(internal::cloneErrToExceptional(r.takeErr()));
            }
            new (tDst) T(r.takeValue());
            return {};
        } else {
            return wrapExceptionalCall([tDst, tSrc]() { new (tDst) T(*tSrc); });
        }
    };

    template <typename T>
    static constexpr BuiltInCloneFn CLONE_FN_OF =
        +[](void* dst, const void* src) -> Result<void, AnyError> {
        auto r = NATIVE_CLONE_FN_OF<T>(dst, src);
        if (r.hasErr()) {
            return Error(AnyError(r.takeErr()));
        }
        return {};
    };

    template <typename T>
    static constexpr BuiltInEqualFn EQUAL_FN_OF = +[](const void* lhs, const void* rhs) -> bool {
        const T* tLhs = reinterpret_cast<const T*>(lhs);
        const T* tRhs = reinterpret_cast<const T*>(rhs);
        return *tLhs == *tRhs;
    };

    template <typename T>
    static constexpr BuiltInHashFn HASH_FN_OF = +[](const void* obj) -> size_t {
        const T* tObj = reinterpret_cast<const T*>(obj);
        std::hash<T> h;
        return h(*tObj);
    };

    template <typename T>
    static constexpr BuiltInCompareFn COMPARE_FN_OF =
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

    template <typename T> static constexpr const BuiltInCloneFn* makeClone() noexcept {
        return &CLONE_FN_OF<T>;
    }

    template <typename T> static constexpr const BuiltInEqualFn* makeEqualityFunction() noexcept {
        return &EQUAL_FN_OF<T>;
    }

    template <typename T> static constexpr const BuiltInHashFn* makeHashFunction() noexcept {
        return &HASH_FN_OF<T>;
    }

    template <typename T> static constexpr const BuiltInCompareFn* makeCompareFunction() noexcept {
        return &COMPARE_FN_OF<T>;
    }
};

} // namespace sy

#endif // _SY_CORE_BUILTIN_TRAITS_BUILTIN_TRAITS_HPP_
