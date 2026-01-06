//! API
#pragma once
#ifndef SY_TYPES_TYPE_HPP_
#define SY_TYPES_TYPE_HPP_

#include "../core/core.h"
#include "../program/program_error.hpp"
#include "function/function.hpp"
#include "option/option.hpp"
#include "ordering/ordering.hpp"
#include "string/string_slice.hpp"
#include <new>
#include <type_traits>
#include <utility>

namespace sy {
namespace detail {
// https://stackoverflow.com/a/35207812
template <class T, class EqualTo> struct has_operator_equal_impl {
    template <class U, class V> static auto test(U*) -> decltype(std::declval<U>() == std::declval<V>());
    template <typename, typename> static auto test(...) -> std::false_type;

    using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
};

template <typename T> struct has_operator_equal : has_operator_equal_impl<T, T>::type {};

// https://stackoverflow.com/a/51915825
template <typename T, typename = std::void_t<>> struct is_std_hashable : std::false_type {};

template <typename T>
struct is_std_hashable<T, std::void_t<decltype(std::declval<std::hash<T>>()(std::declval<T>()))>> : std::true_type {};

template <typename T> struct has_less_than {
  private:
    template <typename U, typename = decltype(std::declval<U>() < std::declval<U>())> static std::true_type test(U*);

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
        // Char = c::SyTypeTag::SyTypeTagChar,
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
    Option<const RawFunction*> destructor;
    Option<const RawFunction*> copyConstructor;
    Option<const RawFunction*> equality;
    Option<const RawFunction*> hash;
    Option<const RawFunction*> compare;
    Tag tag;
    ExtraInfo extra;
    const Type* constRef;
    const Type* mutRef;

    template <typename T> static const Type* makeType(StringSlice inName, Tag inTag, ExtraInfo inExtra) {
        static const Type* actualType = createType<T>(inName, inTag, inExtra);
        return actualType;
    }

    template <typename T> void destroyObject(T* obj) const {
        if constexpr (!std::is_same<T, void>::value) {
            this->assertTypeSizeAlignMatch(sizeof(T), alignof(T));
        }
        this->destroyObjectImpl(reinterpret_cast<void*>(obj));
    }

    template <typename T> void copyConstructObj(T* dst, const T* src) const {
        if constexpr (!std::is_same<T, void>::value) {
            this->assertTypeSizeAlignMatch(sizeof(T), alignof(T));
        }
        this->copyConstructObjectImpl(reinterpret_cast<void*>(dst), reinterpret_cast<const void*>(src));
    }

    template <typename T> bool equalObj(const T* lhs, const T* rhs) const {
        if constexpr (!std::is_same<T, void>::value) {
            this->assertTypeSizeAlignMatch(sizeof(T), alignof(T));
        }
        return this->equalObjectsImpl(reinterpret_cast<const void*>(lhs), reinterpret_cast<const void*>(rhs));
    }

    template <typename T> size_t hashObj(const T* obj) const {
        if constexpr (!std::is_same<T, void>::value) {
            this->assertTypeSizeAlignMatch(sizeof(T), alignof(T));
        }
        return this->hashObjectImpl(reinterpret_cast<const void*>(obj));
    }

    template <typename T> sy::Ordering compareObj(const T* lhs, const T* rhs) const {
        if constexpr (!std::is_same<T, void>::value) {
            this->assertTypeSizeAlignMatch(sizeof(T), alignof(T));
        }
        return this->compareObjectImpl(reinterpret_cast<const void*>(lhs), reinterpret_cast<const void*>(rhs));
    }

    static const Type* const TYPE_BOOL;
    static const Type* const TYPE_I8;
    static const Type* const TYPE_I16;
    static const Type* const TYPE_I32;
    static const Type* const TYPE_I64;
    static const Type* const TYPE_U8;
    static const Type* const TYPE_U16;
    static const Type* const TYPE_U32;
    static const Type* const TYPE_U64;
    static const Type* const TYPE_USIZE;
    static const Type* const TYPE_F32;
    static const Type* const TYPE_F64;
    // static const Type* const TYPE_CHAR;
    static const Type* const TYPE_STRING_SLICE;
    static const Type* const TYPE_STRING;
    static const Type* const TYPE_ORDERING;

  private:
    template <typename T> static c_function_t makeDestructor() {
        c_function_t func = [](FunctionHandler handler) -> Result<void, ProgramError> {
            T* obj = handler.takeArg<T*>(0);
            obj->~T();
            return {};
        };
        return func;
    }

    template <typename T> static c_function_t makeCopyConstruct() {
        c_function_t func = [](FunctionHandler handler) -> Result<void, ProgramError> {
            T* dst = handler.takeArg<T*>(0);
            const T* src = handler.takeArg<const T*>(1);
            T* _ = new (dst) T(*src);
            (void)_;
            return {};
        };
        return func;
    }

    template <typename T> static c_function_t makeEqualityFunction() {
        c_function_t func = [](FunctionHandler handler) -> Result<void, ProgramError> {
            const T* lhs = handler.takeArg<const T*>(0);
            const T* rhs = handler.takeArg<const T*>(1);
            bool equal = (*lhs) == (*rhs);
            handler.setReturn(std::move(equal));
            return {};
        };
        return func;
    }

    template <typename T> static c_function_t makeHashFunction() {
        c_function_t func = [](FunctionHandler handler) -> Result<void, ProgramError> {
            const T* obj = handler.takeArg<const T*>(0);
            std::hash<T> h;
            size_t hashed = h(*obj);
            handler.setReturn(std::move(hashed));
            return {};
        };
        return func;
    }

    template <typename T> static c_function_t makeCompareFunction() {
        c_function_t func = [](FunctionHandler handler) -> Result<void, ProgramError> {
            const T* lhs = handler.takeArg<const T*>(0);
            const T* rhs = handler.takeArg<const T*>(1);
            bool equal = (*lhs) == (*rhs);
            if (equal) {
                handler.setReturn(Ordering::Equal);
            } else if ((*lhs) < (*rhs)) {
                handler.setReturn(Ordering::Less);
            } else {

                handler.setReturn(Ordering::Greater);
            }
            return {};
        };
        return func;
    }

    template <typename T> static const Type* createType(StringSlice inName, Tag inTag, ExtraInfo inExtra) {
        static Type concreteType = {
            sizeof(T),                         // sizeType
            static_cast<uint16_t>(alignof(T)), // alignType
            inName,                            // name
            nullptr,                           // destructor
            nullptr,                           // copyConstructor
            nullptr,                           // equality
            nullptr,                           // hash
            inTag,                             // tag
            inExtra,                           // extra
            nullptr,                           // constRef
            nullptr                            // mutRef
        };

        const ExtraInfo::Reference constRefInfo = {false, &concreteType};
        const ExtraInfo::Reference mutRefInfo = {true, &concreteType};

        const ExtraInfo constRefExtra{constRefInfo};
        const ExtraInfo mutRefExtra{mutRefInfo};

        static Type constRefType = {sizeof(const T*), static_cast<uint16_t>(alignof(const T*)),
                                    "ConstRef", // TODO proper naming
                                    nullptr,          nullptr,
                                    nullptr,          nullptr,
                                    Tag::Reference,   constRefExtra,
                                    nullptr,          nullptr};

        static Type mutRefType = {sizeof(T*),     static_cast<uint16_t>(alignof(T*)),
                                  "MutRef", // TODO proper naming
                                  nullptr,        nullptr,
                                  nullptr,        nullptr,
                                  Tag::Reference, mutRefExtra,
                                  nullptr,        nullptr};

        concreteType.constRef = &constRefType;
        concreteType.mutRef = &mutRefType;

        // TODO is this smart?
        constRefType.constRef = &constRefType;
        constRefType.mutRef = &mutRefType;
        mutRefType.constRef = &constRefType;
        mutRefType.mutRef = &mutRefType;

        // Destructor
        if constexpr (std::is_destructible_v<T> || !std::is_trivially_destructible_v<T>) {
            c_function_t func = makeDestructor<T>();
            static const Type* argsTypes[1] = {&mutRefType};
            static RawFunction cEqualFunc = {"Destructor", // name
                                             "Destructor", // identifier name
                                             nullptr,      // no return type
                                             argsTypes,
                                             1,                     // number of args
                                             SY_FUNCTION_MIN_ALIGN, // alignment
                                             true,
                                             FunctionType::C,
                                             reinterpret_cast<const void*>(func)};
            concreteType.destructor = &cEqualFunc;
        }

        // Copy Constructor
        if constexpr (std::is_copy_constructible_v<T>) {
            c_function_t func = makeCopyConstruct<T>();
            static const Type* argsTypes[2] = {&mutRefType, &constRefType};
            static RawFunction cEqualFunc = {"CopyConstructor", // name
                                             "CopyConstructor", // identifier name
                                             nullptr,           // no return type
                                             argsTypes,
                                             2,                     // number of args
                                             SY_FUNCTION_MIN_ALIGN, // alignment
                                             true,
                                             FunctionType::C,
                                             reinterpret_cast<const void*>(func)};
            concreteType.copyConstructor = &cEqualFunc;
        }

        // Equality
        if constexpr (detail::has_operator_equal<T>::value) {
            c_function_t func = makeEqualityFunction<T>();
            static const Type* argsTypes[2] = {&constRefType, &constRefType};
            static RawFunction cEqualFunc = {"eq",            // name
                                             "eq",            // identifier name
                                             Type::TYPE_BOOL, // return type
                                             argsTypes,
                                             2,                     // number of args
                                             SY_FUNCTION_MIN_ALIGN, // alignment
                                             true,
                                             FunctionType::C,
                                             reinterpret_cast<const void*>(func)};
            concreteType.equality = &cEqualFunc;
        }

        // Hash
        if constexpr (detail::is_std_hashable<T>::value) {
            c_function_t func = makeHashFunction<T>();
            static const Type* argsTypes[1] = {&constRefType};
            static RawFunction cHashFunc = {"hash",           // name
                                            "hash",           // identifier name
                                            Type::TYPE_USIZE, // return type
                                            argsTypes,
                                            1,                     // number of args
                                            SY_FUNCTION_MIN_ALIGN, // alignment
                                            true,
                                            FunctionType::C,
                                            reinterpret_cast<const void*>(func)};
            concreteType.hash = &cHashFunc;
        }

        // Ordering
        if constexpr (detail::has_less_than<T>::value) {
            c_function_t func = makeCompareFunction<T>();
            static const Type* argsTypes[2] = {&constRefType, &constRefType};
            static RawFunction cCompareFunc = {"cmp",               // name
                                               "cmp",               // identifier name
                                               Type::TYPE_ORDERING, // return type
                                               argsTypes,
                                               2,                     // number of args
                                               SY_FUNCTION_MIN_ALIGN, // alignment
                                               true,
                                               FunctionType::C,
                                               reinterpret_cast<const void*>(func)};
            concreteType.compare = &cCompareFunc;
        }

        return &concreteType;
    }

    void assertTypeSizeAlignMatch(size_t sizeOfType, size_t alignOfType) const;
    void destroyObjectImpl(void* obj) const;
    void copyConstructObjectImpl(void* dst, const void* src) const;
    bool equalObjectsImpl(const void* self, const void* other) const;
    size_t hashObjectImpl(const void* self) const;
    Ordering compareObjectImpl(const void* self, const void* other) const;
};
} // namespace sy

#endif // SY_TYPES_TYPE_HPP_