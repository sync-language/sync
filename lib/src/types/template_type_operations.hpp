//! API

#ifndef SY_TYPES_TEMPLATE_TYPE_OPERATIONS_HPP_
#define SY_TYPES_TEMPLATE_TYPE_OPERATIONS_HPP_

#include "../core/core.h"
#include <functional>
#include <type_traits>

namespace sy {
namespace detail {
using DestructFn = void (*)(void* ptr);
using HashKeyFn = size_t (*)(const void* key);
using EqualKeyFn = bool (*)(const void* searchKey, const void* found);
using MoveConstructFn = void (*)(void* dst, void* src);
using CopyConstructFn = void (*)(void* dst, const void* src);

template <typename T> constexpr DestructFn makeDestructor() {
    if constexpr (!std::is_destructible_v<T> && std::is_trivially_destructible_v<T>) {
        return nullptr;
    }

    return [](void* ptr) {
        T* asT = reinterpret_cast<T*>(ptr);
        asT->~T();
    };
}

template <typename T> constexpr HashKeyFn makeHashKey() {
    return [](const void* key) -> size_t {
        const T* asT = reinterpret_cast<const T*>(key);
        std::hash<T> h;
        return h(*asT);
    };
}

template <typename T> constexpr EqualKeyFn makeEqualKey() {
    return [](const void* searchKey, const void* found) -> bool {
        const T* searchAsT = reinterpret_cast<const T*>(searchKey);
        const T* foundAsT = reinterpret_cast<const T*>(found);
        return (*searchAsT) == (*foundAsT);
    };
}

template <typename T> constexpr MoveConstructFn makeMoveConstructor() {
    return [](void* dst, void* src) {
        T* dstAsT = reinterpret_cast<T*>(dst);
        T* srcAsT = reinterpret_cast<T*>(src);
        if constexpr (std::is_trivially_copyable_v<T>) {
            *dstAsT = *srcAsT;
        } else {
            T* _ = new (dstAsT) T(std::move(*srcAsT));
            (void)_;
        }
    };
}

template <typename T> constexpr CopyConstructFn makeCopyConstructor() {
    return [](void* dst, const void* src) {
        T* dstAsT = reinterpret_cast<T*>(dst);
        const T* srcAsT = reinterpret_cast<const T*>(src);
        if constexpr (std::is_trivially_copyable_v<T>) {
            *dstAsT = *srcAsT;
        } else {
            T* _ = new (dstAsT) T(*srcAsT);
            (void)_;
        }
    };
}
} // namespace detail
} // namespace sy

#endif // SY_TYPES_TEMPLATE_TYPE_OPERATIONS_HPP_
