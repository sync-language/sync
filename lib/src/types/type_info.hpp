//! API
#pragma once
#ifndef SY_TYPES_TYPE_HPP_
#define SY_TYPES_TYPE_HPP_

#include "../core/builtin_traits/builtin_traits.hpp"
#include "../core/core.h"
#include "../program/program_error.hpp"
#include "function/function.hpp"
#include "option/option.hpp"
#include "ordering/ordering.hpp"
#include "reflect_fwd.hpp"
#include "string/string.hpp"
#include "string/string_slice.hpp"
#include <atomic>
#include <new>
#include <string_view>
#include <type_traits>
#include <utility>

namespace sy {
class Type;

namespace detail {
// https://stackoverflow.com/a/35207812
template <class T, class EqualTo> struct has_operator_equal_impl {
    template <class U, class V>
    static auto test(U*) -> decltype(std::declval<U>() == std::declval<V>());
    template <typename, typename> static auto test(...) -> std::false_type;

    using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
};

template <typename T> struct has_operator_equal : has_operator_equal_impl<T, T>::type {};

// https://stackoverflow.com/a/51915825
template <typename T, typename = std::void_t<>> struct is_std_hashable : std::false_type {};

template <typename T>
struct is_std_hashable<T, std::void_t<decltype(std::declval<std::hash<T>>()(std::declval<T>()))>>
    : std::true_type {};

template <typename T> struct has_less_than {
  private:
    template <typename U, typename = decltype(std::declval<U>() < std::declval<U>())>
    static std::true_type test(U*);

    template <typename> static std::false_type test(...);

  public:
    static const bool value = decltype(test<T>(nullptr))::value;
};

} // namespace detail

class SY_API Type {
  public:
    enum class Tag : int32_t {
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

    union SY_API ExtraInfo {
        struct Int {
            /// If `true`, this is a signed integer, otherwise unsigned.
            bool isSigned;
            /// Must be one of `8`, `16`, `32`, or `64`.
            uint8_t bits;
        };

        struct Float {
            /// Must be `32` or `64`.
            uint8_t bits;
        };

        struct Reference {
            bool isMutable;
            const Type* childType;
        };

        struct Function {
            /// Can be null, meaning has no return type.
            const Type* retType;
            /// Can be null, meaning takes no arguments.
            const Type* const* argTypes;
            /// Amount of arguments. Is the length of `argTypes`.
            uint16_t argLen;
        };

        void* _boolInfo;
        Int intInfo;
        Float floatInfo;
        //_unused     charInfo;
        void* _strinSliceInfo;
        void* _stringInfo;
        Reference referenceInfo;
        Function functionInfo;

        constexpr ExtraInfo() : _boolInfo(nullptr) {}
        constexpr ExtraInfo(Int inIntInfo) : intInfo(inIntInfo) {}
        constexpr ExtraInfo(Float inFloatInfo) : floatInfo(inFloatInfo) {}
        constexpr ExtraInfo(Reference inReferenceInfo) : referenceInfo(inReferenceInfo) {}
        constexpr ExtraInfo(Function inFunctionInfo) : functionInfo(inFunctionInfo) {}
    };

    size_t sizeType;
    /// Alignment of the type in bytes. Alignment beyond UINT16_MAX is unsupported.
    uint16_t alignType;
    StringSlice name;
    Tag tag;
    ExtraInfo extra;
    const Function<void(void*)>* destructor;
    const BuiltInCoherentTraits* builtinTraits;
    const Type* constRef;
    const Type* mutRef;

    template <typename T>
    static constexpr Type makeRefType(StringSlice refName, bool isMutable,
                                      const Type* underlyingType);

    template <typename T> Result<void, ProgramError> destroyObject(T* obj) const {
        if (!std::is_constant_evaluated()) {
            if constexpr (!std::is_same<T, void>::value) {
                this->assertTypeSizeAlignMatch(sizeof(T), alignof(T));
            }
        }
        return this->destroyObjectImpl(reinterpret_cast<void*>(obj));
    }

    template <typename T> Result<void, ProgramError> cloneObj(T* dst, const T* src) const {
        if (!std::is_constant_evaluated()) {
            if constexpr (!std::is_same<T, void>::value) {
                this->assertTypeSizeAlignMatch(sizeof(T), alignof(T));
            }
        }
        return this->cloneObjectImpl(reinterpret_cast<void*>(dst),
                                     reinterpret_cast<const void*>(src));
    }

    template <typename T> Result<bool, ProgramError> equalObj(const T* lhs, const T* rhs) const {
        if (!std::is_constant_evaluated()) {
            if constexpr (!std::is_same<T, void>::value) {
                this->assertTypeSizeAlignMatch(sizeof(T), alignof(T));
            }
        }
        return this->equalObjectsImpl(reinterpret_cast<const void*>(lhs),
                                      reinterpret_cast<const void*>(rhs));
    }

    template <typename T> Result<size_t, ProgramError> hashObj(const T* obj) const {
        if (!std::is_constant_evaluated()) {
            if constexpr (!std::is_same<T, void>::value) {
                this->assertTypeSizeAlignMatch(sizeof(T), alignof(T));
            }
        }
        return this->hashObjectImpl(reinterpret_cast<const void*>(obj));
    }

    template <typename T>
    Result<Ordering, ProgramError> compareObj(const T* lhs, const T* rhs) const {
        if (!std::is_constant_evaluated()) {
            if constexpr (!std::is_same<T, void>::value) {
                this->assertTypeSizeAlignMatch(sizeof(T), alignof(T));
            }
        }
        return this->compareObjectImpl(reinterpret_cast<const void*>(lhs),
                                       reinterpret_cast<const void*>(rhs));
    }

    template <typename T> Result<void, ProgramError> elementWiseAtomicDestroyObj(T* dst) const {
        if (!std::is_constant_evaluated()) {
            if constexpr (!std::is_same<T, void>::value) {
                this->assertTypeSizeAlignMatch(sizeof(T), alignof(T));
            }
        }
        return this->elementWiseAtomicDestroyObjImpl(reinterpret_cast<void*>(dst));
    }

    template <typename T>
    Result<void, ProgramError> elementWiseAtomicLoadObj(T* dst, const T* src) const {
        if (!std::is_constant_evaluated()) {
            if constexpr (!std::is_same<T, void>::value) {
                this->assertTypeSizeAlignMatch(sizeof(T), alignof(T));
            }
        }
        return this->elementWiseAtomicLoadObjImpl(reinterpret_cast<void*>(dst),
                                                  reinterpret_cast<const void*>(src));
    }

    template <typename T>
    Result<void, ProgramError> elementWiseAtomicStoreObj(T* dst, const T* src) const {
        if (!std::is_constant_evaluated()) {
            if constexpr (!std::is_same<T, void>::value) {
                this->assertTypeSizeAlignMatch(sizeof(T), alignof(T));
            }
        }
        return this->elementWiseAtomicStoreObjImpl(reinterpret_cast<void*>(dst),
                                                   reinterpret_cast<const void*>(src));
    }

  private:
    void assertTypeSizeAlignMatch(size_t sizeOfType, size_t alignOfType) const;
    Result<void, ProgramError> destroyObjectImpl(void* obj) const;
    Result<void, ProgramError> cloneObjectImpl(void* dst, const void* src) const;
    Result<bool, ProgramError> equalObjectsImpl(const void* self, const void* other) const;
    Result<size_t, ProgramError> hashObjectImpl(const void* self) const;
    Result<Ordering, ProgramError> compareObjectImpl(const void* self, const void* other) const;
    Result<void, ProgramError> elementWiseAtomicDestroyObjImpl(void* obj) const;
    Result<void, ProgramError> elementWiseAtomicLoadObjImpl(void* dst, const void* src) const;
    Result<void, ProgramError> elementWiseAtomicStoreObjImpl(void* dst, const void* src) const;
};

namespace internal {
extern constinit sy::Type TYPE_BOOL;
extern constinit sy::Type TYPE_BOOL_CONST_REF;
extern constinit sy::Type TYPE_BOOL_MUT_REF;

extern constinit sy::Type TYPE_I8;
extern constinit sy::Type TYPE_I8_CONST_REF;
extern constinit sy::Type TYPE_I8_MUT_REF;

extern constinit sy::Type TYPE_I16;
extern constinit sy::Type TYPE_I16_CONST_REF;
extern constinit sy::Type TYPE_I16_MUT_REF;

extern constinit sy::Type TYPE_I32;
extern constinit sy::Type TYPE_I32_CONST_REF;
extern constinit sy::Type TYPE_I32_MUT_REF;

extern constinit sy::Type TYPE_I64;
extern constinit sy::Type TYPE_I64_CONST_REF;
extern constinit sy::Type TYPE_I64_MUT_REF;

extern constinit sy::Type TYPE_U8;
extern constinit sy::Type TYPE_U8_CONST_REF;
extern constinit sy::Type TYPE_U8_MUT_REF;

extern constinit sy::Type TYPE_U16;
extern constinit sy::Type TYPE_U16_CONST_REF;
extern constinit sy::Type TYPE_U16_MUT_REF;

extern constinit sy::Type TYPE_U32;
extern constinit sy::Type TYPE_U32_CONST_REF;
extern constinit sy::Type TYPE_U32_MUT_REF;

extern constinit sy::Type TYPE_U64;
extern constinit sy::Type TYPE_U64_CONST_REF;
extern constinit sy::Type TYPE_U64_MUT_REF;

extern constinit sy::Type TYPE_USIZE;
extern constinit sy::Type TYPE_USIZE_CONST_REF;
extern constinit sy::Type TYPE_USIZE_MUT_REF;

extern constinit sy::Type TYPE_F32;
extern constinit sy::Type TYPE_F32_CONST_REF;
extern constinit sy::Type TYPE_F32_MUT_REF;

extern constinit sy::Type TYPE_F64;
extern constinit sy::Type TYPE_F64_CONST_REF;
extern constinit sy::Type TYPE_F64_MUT_REF;

extern constinit sy::Type TYPE_ORDERING;
extern constinit sy::Type TYPE_ORDERING_CONST_REF;
extern constinit sy::Type TYPE_ORDERING_MUT_REF;

extern constinit sy::Type TYPE_STRING_SLICE;
extern constinit sy::Type TYPE_STRING_SLICE_CONST_REF;
extern constinit sy::Type TYPE_STRING_SLICE_MUT_REF;

extern constinit sy::Type TYPE_STRING;
extern constinit sy::Type TYPE_STRING_CONST_REF;
extern constinit sy::Type TYPE_STRING_MUT_REF;

extern constinit sy::Type TYPE_OPAQUE_PTR;
} // namespace internal

template <> struct SY_API Reflect<bool> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_BOOL; }
    static constexpr const Type* constRef() noexcept { return &sy::internal::TYPE_BOOL_CONST_REF; }
    static constexpr const Type* mutRef() noexcept { return &sy::internal::TYPE_BOOL_MUT_REF; }
};

template <> struct SY_API Reflect<int8_t> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_I8; }
    static constexpr const Type* constRef() noexcept { return &sy::internal::TYPE_I8_CONST_REF; }
    static constexpr const Type* mutRef() noexcept { return &sy::internal::TYPE_I8_MUT_REF; }
};

template <> struct SY_API Reflect<int16_t> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_I16; }
    static constexpr const Type* constRef() noexcept { return &sy::internal::TYPE_I16_CONST_REF; }
    static constexpr const Type* mutRef() noexcept { return &sy::internal::TYPE_I16_MUT_REF; }
};

template <> struct SY_API Reflect<int32_t> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_I32; }
    static constexpr const Type* constRef() noexcept { return &sy::internal::TYPE_I32_CONST_REF; }
    static constexpr const Type* mutRef() noexcept { return &sy::internal::TYPE_I32_MUT_REF; }
};

template <> struct SY_API Reflect<int64_t> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_I64; }
    static constexpr const Type* constRef() noexcept { return &sy::internal::TYPE_I64_CONST_REF; }
    static constexpr const Type* mutRef() noexcept { return &sy::internal::TYPE_I64_MUT_REF; }
};

template <> struct SY_API Reflect<uint8_t> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_U8; }
    static constexpr const Type* constRef() noexcept { return &sy::internal::TYPE_U8_CONST_REF; }
    static constexpr const Type* mutRef() noexcept { return &sy::internal::TYPE_U8_MUT_REF; }
};

template <> struct SY_API Reflect<uint16_t> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_U16; }
    static constexpr const Type* constRef() noexcept { return &sy::internal::TYPE_U16_CONST_REF; }
    static constexpr const Type* mutRef() noexcept { return &sy::internal::TYPE_U16_MUT_REF; }
};

template <> struct SY_API Reflect<uint32_t> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_U32; }
    static constexpr const Type* constRef() noexcept { return &sy::internal::TYPE_U32_CONST_REF; }
    static constexpr const Type* mutRef() noexcept { return &sy::internal::TYPE_U32_MUT_REF; }
};

template <> struct SY_API Reflect<uint64_t> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_U64; }
    static constexpr const Type* constRef() noexcept { return &sy::internal::TYPE_U64_CONST_REF; }
    static constexpr const Type* mutRef() noexcept { return &sy::internal::TYPE_U64_MUT_REF; }
};

template <> struct SY_API Reflect<size_t> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_USIZE; }
    static constexpr const Type* constRef() noexcept { return &sy::internal::TYPE_USIZE_CONST_REF; }
    static constexpr const Type* mutRef() noexcept { return &sy::internal::TYPE_USIZE_MUT_REF; }
};

template <> struct SY_API Reflect<float> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_F32; }
    static constexpr const Type* constRef() noexcept { return &sy::internal::TYPE_F32_CONST_REF; }
    static constexpr const Type* mutRef() noexcept { return &sy::internal::TYPE_F32_MUT_REF; }
};

template <> struct SY_API Reflect<double> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_F64; }
    static constexpr const Type* constRef() noexcept { return &sy::internal::TYPE_F64_CONST_REF; }
    static constexpr const Type* mutRef() noexcept { return &sy::internal::TYPE_F64_MUT_REF; }
};

template <> struct SY_API Reflect<Ordering> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_ORDERING; }
    static constexpr const Type* constRef() noexcept {
        return &sy::internal::TYPE_ORDERING_CONST_REF;
    }
    static constexpr const Type* mutRef() noexcept { return &sy::internal::TYPE_ORDERING_MUT_REF; }
};

template <> struct SY_API Reflect<StringSlice> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_STRING_SLICE; }
    static constexpr const Type* constRef() noexcept {
        return &sy::internal::TYPE_STRING_SLICE_CONST_REF;
    }
    static constexpr const Type* mutRef() noexcept {
        return &sy::internal::TYPE_STRING_SLICE_MUT_REF;
    }
};

template <> struct SY_API Reflect<String> {
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_STRING; }
    static constexpr const Type* constRef() noexcept {
        return &sy::internal::TYPE_STRING_CONST_REF;
    }
    static constexpr const Type* mutRef() noexcept { return &sy::internal::TYPE_STRING_MUT_REF; }
};

template <> struct SY_API Reflect<std::string_view> : Reflect<StringSlice> {};

template <> struct SY_API Reflect<void*> {
    // should have ref types?
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_OPAQUE_PTR; }
    static constexpr const Type* constRef() noexcept { return nullptr; }
    static constexpr const Type* mutRef() noexcept { return nullptr; }
};

template <> struct SY_API Reflect<const void*> {
    // should have ref types?
    static constexpr const Type* get() noexcept { return &sy::internal::TYPE_OPAQUE_PTR; }
    static constexpr const Type* constRef() noexcept { return nullptr; }
    static constexpr const Type* mutRef() noexcept { return nullptr; }
};

namespace internal {
constexpr static sy::Function<void(void*)> EMPTY_DESTRUCTOR = +[](void*) {};
constexpr static decltype(sy::BuiltInCoherentTraits::clone) PTR_BUILTIN_COHERENT_CLONE =
    sy::BuiltInCoherentTraits::makeClone<void*>();
constexpr static decltype(sy::BuiltInCoherentTraits::equal) PTR_BUILTIN_COHERENT_EQUALITY =
    sy::BuiltInCoherentTraits::makeEqualityFunction<void*>();
constexpr static decltype(sy::BuiltInCoherentTraits::hash) PTR_BUILTIN_COHERENT_HASH =
    sy::BuiltInCoherentTraits::makeHashFunction<void*>();
constexpr static decltype(sy::BuiltInCoherentTraits::compare) PTR_BUILTIN_COHERENT_COMPARE =
    sy::BuiltInCoherentTraits::makeCompareFunction<void*>();
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicDestroy)
    PTR_ELEMENT_WISE_ATOMIC_DESTROY = &EMPTY_DESTRUCTOR;
constexpr static Function<void(void* dst, const void* src)>
    PTR_ELEMENT_WISE_ATOMIC_SHALLOW_CLONE_IMPL = +[](void* dst, const void* src) {
        std::atomic<void*>* atomicDst = reinterpret_cast<std::atomic<void*>*>(dst);
        const std::atomic<void*>* atomicSrc = reinterpret_cast<const std::atomic<void*>*>(src);
        void* temp = atomicSrc->load(std::memory_order_relaxed);
        atomicDst->store(temp, std::memory_order_relaxed);
    };

constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicLoad)
    PTR_ELEMENT_WISE_ATOMIC_LOAD = &PTR_ELEMENT_WISE_ATOMIC_SHALLOW_CLONE_IMPL;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicStore)
    PTR_ELEMENT_WISE_ATOMIC_STORE = &PTR_ELEMENT_WISE_ATOMIC_SHALLOW_CLONE_IMPL;
constexpr static sy::BuiltInCoherentTraits PTR_BUILTIN_TRAITS = {
    .clone = PTR_BUILTIN_COHERENT_CLONE,
    .equal = PTR_BUILTIN_COHERENT_EQUALITY,
    .hash = PTR_BUILTIN_COHERENT_HASH,
    .compare = PTR_BUILTIN_COHERENT_COMPARE,
    .elementWiseAtomicDestroy = PTR_ELEMENT_WISE_ATOMIC_DESTROY,
    .elementWiseAtomicLoad = PTR_ELEMENT_WISE_ATOMIC_LOAD,
    .elementWiseAtomicStore = PTR_ELEMENT_WISE_ATOMIC_STORE};
} // namespace internal

template <typename T>
constexpr Type Type::makeRefType(StringSlice refName, bool isMutable, const Type* childType) {
    const Type t = {
        .sizeType = sizeof(void*),
        .alignType = static_cast<uint16_t>(alignof(void*)),
        .name = refName,
        .tag = Type::Tag::Reference,
        .extra = Type::ExtraInfo(Type::ExtraInfo::Reference{isMutable, childType}),
        .destructor = +[](void*) {},
        .builtinTraits = &internal::PTR_BUILTIN_TRAITS,
        .constRef = nullptr,
        .mutRef = nullptr,
    };
    return t;
}

namespace internal {
template <typename T> struct ReflectImpl {
    static constexpr const Type* get() noexcept { return sy::Reflect<T>::get(); }

    static const Type* constRef() noexcept {
        if constexpr (requires { sy::Reflect<T>::constRef(); }) {
            return sy::Reflect<T>::constRef();
        } else {
            static sy::Type t = sy::Type::makeRefType<T>(StringSlice("Sync_Reflect_ConstRef"),
                                                         false, sy::Reflect<T>::get());
            return &t;
        }
    }

    static const Type* mutRef() noexcept {
        if constexpr (requires { sy::Reflect<T>::mutRef(); }) {
            return sy::Reflect<T>::mutRef();
        } else {
            static sy::Type t = sy::Type::makeRefType<T>(StringSlice("Sync_Reflect_MutRef"), true,
                                                         sy::Reflect<T>::get());
            return &t;
        }
    }
};
} // namespace internal

} // namespace sy

#endif // SY_TYPES_TYPE_HPP_
