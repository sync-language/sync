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
#include "string/string_slice.hpp"

namespace sy {
class TypeMetadata;

class SY_API Type {
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

    /// Invoke the destructor of `T` on `obj`, whether as a native type or a script type. If `this`
    /// is a reference / pointer type (`indirection > 0`), will return without invoking anything. If
    /// `base->tag == sy::TypeExtra::Tag::Reference`, will also return without invoking anything.
    template <typename T> [[nodiscard]] Result<void, AnyError> destroy(T* obj) const noexcept;

    /// Invoke this type's destructor on `obj`. Cannot check if `obj` is an object of this type. If
    /// `this` is a reference / pointer type (`indirection > 0`), will return without invoking
    /// anything. If `base->tag == sy::TypeExtra::Tag::Reference`, will also return without
    /// invoking anything.
    [[nodiscard]] Result<void, AnyError> destroyUnchecked(void* obj) const noexcept;
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

} // namespace sy

// #endif // SY_TYPES_TYPE_HPP_
