//! API
#pragma once
#ifndef SY_UTIL_MOVE_AND_LEAK_HPP_
#define SY_UTIL_MOVE_AND_LEAK_HPP_

#include "../core/core.h"
#include <new>
#include <type_traits>
#include <utility>

namespace sy {
namespace internal {
/// Moves `objToLeak` into a byte buffer, not calling the destructor on it.
/// Assumes that the destructor is safe to run on a moved object (it should be).
/// @tparam T The type of the object to move
/// @param objToLeak The object that gets moved to "supress" it's destructor.
template <typename T> void moveAndLeak(T&& objToLeak) {
    using U = std::remove_reference_t<T>;
    static_assert(!std::is_lvalue_reference_v<U>,
                  "moveAndLeak requires an rvalue, so wrap with std::move(...)");
    alignas(U) uint8_t buf[sizeof(U)];
    (void)::new (static_cast<void*>(buf)) U(std::forward(objToLeak));
}
} // namespace internal
} // namespace sy

#endif // SY_UTIL_MOVE_AND_LEAK_HPP_
