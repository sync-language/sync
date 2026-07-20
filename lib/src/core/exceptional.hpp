//! API
#pragma once
#ifndef SY_CORE_EXCEPTIONAL_HPP_
#define SY_CORE_EXCEPTIONAL_HPP_

#include "../types/result/result.hpp"
#include "core.h"
#include <type_traits>
#include <utility>

namespace sy {
// https://en.cppreference.com/cpp/error/exception
enum class Exceptional : int {
    OOM = 1,
    Bounds = 2,
    System = 3,
    Io = 4,
    Arithmetic = 5,
    Capacity = 6,
    Other = 7,
};

namespace internal {
/// Re-throws the current exception (so must be called inside a `catch` block), and converts it
/// to a `sy::Exceptional`.
SY_API Exceptional translateCurrentException() noexcept;
} // namespace internal

template <typename Func, typename... Args>
auto wrapExceptionalCall(Func&& func, Args&&... args) noexcept {
    using RetT = std::invoke_result_t<Func, Args...>;
#if SYNC_HAS_EXCEPTIONS
    try {
#endif
        if constexpr (std::is_void_v<RetT>) {
            std::forward<Func>(func)(std::forward<Args>(args)...);
            return Result<RetT, Exceptional>{};
        } else {
            return Result<RetT, Exceptional>{std::forward<Func>(func)(std::forward<Args>(args)...)};
        }
#if SYNC_HAS_EXCEPTIONS
    } catch (...) {
        return Result<RetT, Exceptional>{Error(internal::translateCurrentException())};
    }
#endif
}

} // namespace sy

#endif // SY_CORE_EXCEPTIONAL_HPP_
