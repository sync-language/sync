//! API
#pragma once
// #ifndef SY_TYPES_TYPE_HPP_
// #define SY_TYPES_TYPE_HPP_

#include "../core/builtin_traits/builtin_traits.hpp"
#include "../core/core.h"
#include "anyerror/anyerror.hpp"
#include "option/option.hpp"
#include "ordering/ordering.hpp"
#include "result/result.hpp"
#include "string/string.hpp"
#include "string/string_slice.hpp"
#include <type_traits>

namespace sy {
class TypeMetadata;

class SY_API Type final {
  public:
    /// Must be non-null.
    const sy::TypeMetadata* base;
    /// The amount of pointer indirection to get to `base`. For example if it was a C type,
    /// `int**` would have `indirection == 2`. If `indirection == 0`, this is the concrete type.
    /// @warning This value must always be `indirection <= 32`.
    uint32_t indirection;
    /// Bitmask of which levels in `indirection` are mutable or not. If `indirection == 0`, this
    /// field is ignored.
    /// For example:
    /// - `indirection == 1` and it's a pointer to a mutable `int*`, then `mutableBits == 0b01`.
    /// - `indirection == 2` and both levels of pointers are mutable `int**`, then `mutableBits ==
    /// 0b011`.
    /// - `indirection == 2` and the higher level pointer is mutable while the lower is immutable
    /// such as `const int**`, then `mutableBits == 0b010`.
    /// - `indirection == 32` and none of the pointer levels are mutable, then `mutableBits ==
    /// 0b00000000000000000000000000000000` (all bits unset).
    /// - `indirection == 32` and all of the pointer levels are mutable, then `mutableBits ==
    /// 0b11111111111111111111111111111111` (all bits set).
    uint32_t mutableBits;

    constexpr Type(const TypeMetadata* inBase, uint32_t inIndirection, uint32_t inMutableBits)
        : base(inBase), indirection(inIndirection), mutableBits(inMutableBits) {}

    constexpr Type(const Type&) = default;

    constexpr Type& operator=(const Type&) = default;

    constexpr Type(Type&&) noexcept = default;

    constexpr Type& operator=(Type&&) noexcept = default;

    /// Check if the indirection level at `level` is mutable or not. For example, if the type is
    /// `int*`, then `isIndirectionLevelMutable(0)` would yield `true`. If the type is `int**`, then
    /// `level` of `0` or `1` would also be `true`. If the type is `const int**`, then `level` of
    /// `0` is `true`, and `level` of `1` is `false`.
    /// @param level The pointer indirection level to check.
    /// @return `true` if the pointer indirection at `level` is mutable, otherwise `false`
    /// indicating it is immutable.
    /// @warning `level >= indirection` is a fatal condition.
    [[nodiscard]] bool isIndirectionLevelMutable(uint32_t level) const noexcept;

    /// @param isMutable Whether the new reference to this type should be mutable or not to the
    /// level below.
    /// @return a new `Type` instance with increased pointer indirection.
    [[nodiscard]] Type reference(bool isMutable) const noexcept;

    /// Does not carry down the  mutability of the `this` type information at the level that `this`
    /// is at. So if `this` is a type of `const int**`, and you dereference, you will get a new
    /// `Type` for `const int*`, without carrying the fact that the above reference was mutable.
    /// @return a new `Type` instance with one less pointer indirection.
    /// @warning `indirection == 0` is a fatal condition.
    [[nodiscard]] Type dereference() const noexcept;

    [[nodiscard]] bool isReference() const noexcept { return this->indirection > 0; }

    [[nodiscard]] bool operator==(const Type& other) const noexcept;

    [[nodiscard]] bool operator!=(const Type& other) const noexcept {
        return ((*this) == other) == false;
    }

    /// @return If `isReference()`, `sizeof(void*)` otherwise `base->typeSize`.
    [[nodiscard]] size_t byteSize() const noexcept;

    /// @return If `isReference()`, `alignof(void*)` otherwise `base->typeAlign`.
    [[nodiscard]] size_t byteAlign() const noexcept;

    /// Invoke the destructor of `T` on `obj`, whether as a native type or a script type. If
    /// `this->isReference()` will return without invoking anything. If `base->extra.tag ==
    /// sy::TypeExtra::Tag::Reference`, will also return without invoking anything.
    /// @param obj Non-null pointer to the object's memory.
    template <typename T> [[nodiscard]] Result<void, AnyError> destroy(T* obj) const noexcept;

    /// Invoke this type's destructor on `obj`. Cannot check if `obj` is an object of this type. If
    /// `this->isReference()`will return without invoking anything. If `base->extra.tag ==
    /// sy::TypeExtra::Tag::Reference`, will also return without invoking anything.
    /// @param obj Non-null pointer to the object's memory.
    [[nodiscard]] Result<void, AnyError> destroyUnchecked(void* obj) const noexcept;

    template <typename T>
    [[nodiscard]] Result<void, AnyError> clone(T* out, const T* srcObj) const noexcept;

    [[nodiscard]] Result<void, AnyError> cloneUnchecked(void* out,
                                                        const void* srcObj) const noexcept;

    template <typename T>
    [[nodiscard]] Result<bool, AnyError> equal(const T* lhs, const T* rhs) const noexcept;

    [[nodiscard]] Result<bool, AnyError> equalUnchecked(const void* lhs,
                                                        const void* rhs) const noexcept;

    template <typename T> [[nodiscard]] Result<size_t, AnyError> hash(const T* obj) const noexcept;

    [[nodiscard]] Result<size_t, AnyError> hashUnchecked(const void* obj) const noexcept;

    template <typename T>
    [[nodiscard]] Result<Ordering, AnyError> compare(const T* lhs, const T* rhs) const noexcept;

    [[nodiscard]] Result<Ordering, AnyError> compareUnchecked(const void* lhs,
                                                              const void* rhs) const noexcept;

    template <typename T>
    [[nodiscard]] Result<void, AnyError> elementWiseAtomicDestroy(T* obj) const noexcept;

    [[nodiscard]] Result<void, AnyError>
    elementWiseAtomicDestroyUnchecked(void* obj) const noexcept;

    template <typename T>
    [[nodiscard]] Result<void, AnyError> elementWiseAtomicLoad(T* out,
                                                               const T* srcObj) const noexcept;

    [[nodiscard]] Result<void, AnyError>
    elementWiseAtomicLoadUnchecked(void* out, const void* srcObj) const noexcept;

    template <typename T>
    [[nodiscard]] Result<void, AnyError> elementWiseAtomicStore(T* out,
                                                                const T* srcObj) const noexcept;

    [[nodiscard]] Result<void, AnyError>
    elementWiseAtomicStoreUnchecked(void* out, const void* srcObj) const noexcept;
}; // class Type

class TypeExtra {
  public:
    enum class Tag : int {
        Bool = 0,
        Int = 1,
        Float = 2,
        OpaquePointer = 3,
        String = 4,
        StringSlice = 5,
        Ordering = 6,
        Reference = 7,
        Function = 18,
    };

    struct Int {
        /// If `true`, this is a signed integer, otherwise unsigned.
        bool isSigned;
        /// @warning Must be one of `8`, `16`, `32`, or `64`.
        uint8_t bits;
    };

    struct Float {
        /// @warning Must be `32` or `64`.
        uint8_t bits;
    };

    struct Reference {
        bool isMutable;
        sy::Type childType;
    };

    struct Function {
        /// Can be null, meaning has no return type.
        sy::Type retType;
        /// Can be null, meaning takes no arguments.
        const sy::Type* argTypes;
        /// Amount of arguments. Is the length of `argTypes`.
        uint16_t argLen;
    };

    union SY_API Info {
        /// Unused, reserved internally only.
        void* _boolInfo;
        /// Only active when `TypeExtra::tag == TypeExtra::Tag::Int`.
        Int intInfo;
        /// Only active when `TypeExtra::tag == TypeExtra::Tag::Float`.
        Float floatInfo;
        /// Unused, reserved internally only.
        // void* charInfo;
        /// Unused, reserved internally only.
        void* _stringSliceInfo;
        /// Unused, reserved internally only.
        void* _stringInfo;
        /// Only active when `TypeExtra::tag == TypeExtra::Tag::Reference`.
        Reference referenceInfo;
        /// Only active when `TypeExtra::tag == TypeExtra::Tag::Function`.
        Function functionInfo;

        constexpr Info() : _boolInfo(nullptr) {}
        constexpr Info(Int inIntInfo) : intInfo(inIntInfo) {}
        constexpr Info(Float inFloatInfo) : floatInfo(inFloatInfo) {}
        constexpr Info(Reference inReferenceInfo) : referenceInfo(inReferenceInfo) {}
        constexpr Info(Function inFunctionInfo) : functionInfo(inFunctionInfo) {}
    };

    Tag tag;
    Info info;
}; // class TypeExtra

class SY_API TypeMetadata {
  public:
    /// Size in bytes.
    size_t typeSize;
    /// Alignment in bytes.
    /// @warning Alignment beyond `UINT16_MAX` is unsupported.
    size_t typeAlign;
    sy::StringSlice name;
    sy::TypeExtra extra;
    const sy::BuiltInDestructorFn* destructor;
    const sy::BuiltInCoherentTraits* builtinTraits;
}; // class TypeMetadata

/// Generic way to get type information. This is used across Sync internally through C++ API
/// boundaries. Specialize for your specific type:
///
/// ```cpp
/// template <> struct SY_API ::sy::Reflect<T> {
///    static constexpr ::sy::Type get() noexcept { return YOUR_TYPE_GLOBAL_OBJ; }
/// };
/// ```
/// @tparam T the type to specialize for.
template <typename T, typename Enable = void> struct Reflect {
    static constexpr Type get() noexcept {
        static_assert(sizeof(T) == 0, "sy::Reflect must be specialized for this type");
        return Type{nullptr, 0, 0};
    }
};

namespace internal { // primitives :)
SY_API extern const Type TYPE_BOOL;
SY_API extern const Type TYPE_I8;
SY_API extern const Type TYPE_U8;
SY_API extern const Type TYPE_I16;
SY_API extern const Type TYPE_U16;
SY_API extern const Type TYPE_I32;
SY_API extern const Type TYPE_U32;
SY_API extern const Type TYPE_I64;
SY_API extern const Type TYPE_U64;
SY_API extern const Type TYPE_USIZE;
SY_API extern const Type TYPE_F32;
SY_API extern const Type TYPE_F64;
SY_API extern const Type TYPE_ORDERING;
SY_API extern const Type TYPE_STRING_SLICE;
SY_API extern const Type TYPE_STRING;
SY_API extern const Type TYPE_OPAQUE_PTR;
} // namespace internal

template <> struct SY_API Reflect<bool> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_BOOL; }
};

template <> struct SY_API Reflect<int8_t> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_I8; }
};

template <> struct SY_API Reflect<uint8_t> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_U8; }
};

template <> struct SY_API Reflect<int16_t> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_I16; }
};

template <> struct SY_API Reflect<uint16_t> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_U16; }
};

template <> struct SY_API Reflect<int32_t> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_I32; }
};

template <> struct SY_API Reflect<uint32_t> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_U32; }
};

template <> struct SY_API Reflect<int32_t> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_I64; }
};

template <> struct SY_API Reflect<uint32_t> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_U64; }
};

template <typename T>
struct SY_API Reflect<T, std::enable_if_t<std::is_same_v<T, size_t> &&
                                          !std::is_same_v<size_t, uint64_t>>> { // Windows :(
    static constexpr Type get() noexcept { return sy::internal::TYPE_USIZE; }
};

template <> struct SY_API Reflect<float> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_F32; }
};

template <> struct SY_API Reflect<double> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_F64; }
};

template <> struct SY_API Reflect<Ordering> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_ORDERING; }
};

template <> struct SY_API Reflect<StringSlice> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_STRING_SLICE; }
};

template <> struct SY_API Reflect<String> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_STRING; }
};

template <> struct SY_API Reflect<void*> {
    static constexpr Type get() noexcept { return sy::internal::TYPE_OPAQUE_PTR; }
};

template <> struct SY_API Reflect<const void*> { // TODO is this right
    static constexpr Type get() noexcept { return sy::internal::TYPE_OPAQUE_PTR; }
};

namespace internal {
SY_API void sy_type_debug_assert_same_size(size_t objSize, size_t expectedSize) noexcept;
SY_API void sy_type_debug_assert_same_align(size_t objAlign, size_t expectedAlign) noexcept;
} // namespace internal

template <typename T> inline Result<void, AnyError> Type::destroy(T* obj) const noexcept {
    if constexpr (std::is_pointer<T>::value) {
        return {};
    }
    // if constexpr (std::is_reference<T>::value) { // TODO what to do for references?
    //     return {};
    // }
    sy_type_debug_assert_same_size(sizeof(T), this->base->typeSize);
    sy_type_debug_assert_same_size(alignof(T), this->base->typeAlign);
    return this->destroyUnchecked(static_cast<void*>(obj));
}

template <typename T>
inline Result<void, AnyError> Type::clone(T* out, const T* srcObj) const noexcept {
    if constexpr (!std::is_pointer<T>::value) {
        sy_type_debug_assert_same_size(sizeof(T), this->base->typeSize);
        sy_type_debug_assert_same_size(alignof(T), this->base->typeAlign);
    } // TODO what to do for references?

    return this->cloneUnchecked(static_cast<void*>(out), static_cast<const void*>(srcObj));
}

template <typename T>
inline Result<bool, AnyError> Type::equal(const T* lhs, const T* rhs) const noexcept {
    if constexpr (!std::is_pointer<T>::value) {
        sy_type_debug_assert_same_size(sizeof(T), this->base->typeSize);
        sy_type_debug_assert_same_size(alignof(T), this->base->typeAlign);
    } // TODO what to do for references?

    return this->equalUnchecked(static_cast<const void*>(lhs), static_cast<const void*>(rhs))
}

template <typename T> inline Result<size_t, AnyError> Type::hash(const T* obj) const noexcept {
    if constexpr (!std::is_pointer<T>::value) {
        sy_type_debug_assert_same_size(sizeof(T), this->base->typeSize);
        sy_type_debug_assert_same_size(alignof(T), this->base->typeAlign);
    } // TODO what to do for references?

    return this->hashUnchecked(static_cast<const void*>(obj))
}

template <typename T>
inline Result<Ordering, AnyError> Type::compare(const T* lhs, const T* rhs) const noexcept {
    if constexpr (!std::is_pointer<T>::value) {
        sy_type_debug_assert_same_size(sizeof(T), this->base->typeSize);
        sy_type_debug_assert_same_size(alignof(T), this->base->typeAlign);
    } // TODO what to do for references?

    return this->compareUnchecked(static_cast<const void*>(lhs), static_cast<const void*>(rhs))
}

template <typename T>
inline Result<void, AnyError> Type::elementWiseAtomicDestroy(T* obj) const noexcept {
    if constexpr (std::is_pointer<T>::value) {
        return {};
    }
    // if constexpr (std::is_reference<T>::value) { // TODO what to do for references?
    //     return {};
    // }
    sy_type_debug_assert_same_size(sizeof(T), this->base->typeSize);
    sy_type_debug_assert_same_size(alignof(T), this->base->typeAlign);
    return this->elementWiseAtomicDestroyUnchecked(static_cast<void*>(obj));
}

template <typename T>
inline Result<void, AnyError> Type::elementWiseAtomicLoad(T* out, const T* srcObj) const noexcept {
    if constexpr (!std::is_pointer<T>::value) {
        sy_type_debug_assert_same_size(sizeof(T), this->base->typeSize);
        sy_type_debug_assert_same_size(alignof(T), this->base->typeAlign);
    } // TODO what to do for references?

    return this->elementWiseAtomicLoadUnchecked(static_cast<void*>(out),
                                                static_cast<const void*>(srcObj));
}

template <typename T>
inline Result<void, AnyError> Type::elementWiseAtomicStore(T* out, const T* srcObj) const noexcept {
    if constexpr (!std::is_pointer<T>::value) {
        sy_type_debug_assert_same_size(sizeof(T), this->base->typeSize);
        sy_type_debug_assert_same_size(alignof(T), this->base->typeAlign);
    } // TODO what to do for references?

    return this->elementWiseAtomicStoreUnchecked(static_cast<void*>(out),
                                                 static_cast<const void*>(srcObj));
}
} // namespace sy

// #endif // SY_TYPES_TYPE_HPP_
