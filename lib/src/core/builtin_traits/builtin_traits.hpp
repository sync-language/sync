//! API
#pragma once
#ifndef _SY_CORE_BUILTIN_TRAITS_BUILTIN_TRAITS_HPP_
#define _SY_CORE_BUILTIN_TRAITS_BUILTIN_TRAITS_HPP_

#include "../../types/function/function.hpp"
#include "../../types/option/option.hpp"
#include "../../types/ordering/ordering.hpp"
#include "../../types/anyerror/anyerror.hpp"
#include "../../types/result/result.hpp"
#include "../core.h"
#include "../exceptional.hpp"
#include <new>
#include <type_traits>
#include <utility>

namespace sy {
using NativeDestructorFn = void (*)(void* obj);
using NativeCloneFn = Result<void, Exceptional> (*)(void* dst, const void* src);

using BuiltInDestructorFn = Function<void(void* obj)>;
// Success type is `void`; failure flows through `Function`'s `AnyError` channel. The native
// clone's `Exceptional` is swallowed into an (inline, non-allocating) `AnyError` by `CLONE_FN_OF`.
using BuiltInCloneFn = Function<void(void* dst, const void* src)>;

namespace detail {
// Detects an opt-in member `clone()`. Types without one fall back to the copy constructor.
template <typename T, typename = void> struct has_member_clone : std::false_type {};
template <typename T>
struct has_member_clone<T, std::void_t<decltype(std::declval<const T&>().clone())>>
    : std::true_type {};

// Widen an accepted clone error into the native clone channel (`Exceptional`). `AnyError`-returning
// clones are intentionally unsupported here; a future `TryClone` will upcast to `AnyError`.
constexpr Exceptional cloneErrToExceptional(AllocErr) noexcept { return Exceptional::OOM; }
constexpr Exceptional cloneErrToExceptional(Exceptional e) noexcept { return e; }
} // namespace detail

struct BuiltInCoherentTraits {
    Option<const BuiltInCloneFn*> clone;
    Option<const Function<bool(const void* lhs, const void* rhs)>*> equal;
    Option<const Function<size_t(const void* obj)>*> hash;
    Option<const Function<Ordering(const void* lhs, const void* rhs)>*> compare;
    Option<const Function<void(void* self)>*> elementWiseAtomicDestroy;
    Option<const Function<void(void* dst, const void* src)>*> elementWiseAtomicLoad;
    Option<const Function<void(void* dst, const void* src)>*> elementWiseAtomicStore;

    // Physical clone. Dispatches to an opt-in member `clone()` (accepting `Result<T, AllocErr>` or
    // `Result<T, Exceptional>`), otherwise firewalls the copy constructor. Speaks `Exceptional`.
    template <typename T>
    static constexpr NativeCloneFn NATIVE_CLONE_FN_OF =
        +[](void* dst, const void* src) -> Result<void, Exceptional> {
        T* tDst = reinterpret_cast<T*>(dst);
        const T* tSrc = reinterpret_cast<const T*>(src);
        if constexpr (detail::has_member_clone<T>::value) {
            auto r = tSrc->clone();
            if (r.hasErr()) {
                return Error(detail::cloneErrToExceptional(r.takeErr()));
            }
            new (tDst) T(r.takeValue());
            return {};
        } else {
            return wrapExceptionalCall([tDst, tSrc]() { new (tDst) T(*tSrc); });
        }
    };

    // Interpreter-facing clone (`Function`, `AnyError` channel): swallows the native `Exceptional`
    // into an inline `AnyError`.
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

    template <typename T> static constexpr const BuiltInCloneFn* makeClone() noexcept {
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
