//! API
#pragma once
#ifndef SY_TYPES_TYPE_HPP_
#define SY_TYPES_TYPE_HPP_

#include "../core.h"
#include "string/string_slice.hpp"
#include <type_traits>
#include <utility>
#include "function/function.hpp"
#include "../program/program.hpp"

namespace sy {
    namespace detail {
        /// https://stackoverflow.com/a/35207812
        template<class T, class EqualTo>
        struct has_operator_equal_impl
        {
            template<class U, class V>
            static auto test(U*) -> decltype(std::declval<U>() == std::declval<V>());
            template<typename, typename>
            static auto test(...) -> std::false_type;

            using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
        };

        template<typename T>
        struct has_operator_equal : has_operator_equal_impl<T, T>::type {};
    }
    
    class SY_API Type {
    public:
        enum class Tag : int32_t {
            Bool = 0,
            Int = 1,
            Float = 2,
            //Char = c::SyTypeTag::SyTypeTagChar,
            String = 4,
            StringSlice = 5,
            Reference = 7,
            Function = 18,
        };

        union SY_API ExtraInfo {
            struct Int {
                /// If `true`, this is a signed integer, otherwise unsigned.
                bool    isSigned;
                /// Must be one of `8`, `16`, `32`, or `64`.
                uint8_t bits;
            };

            struct Float {
                /// Must be `32` or `64`.
                uint8_t bits;
            };

            struct Reference {
                bool        isMutable;
                const Type* childType;
            };

            struct Function {
                /// Can be null, meaning has no return type.
                const Type*         retType;
                /// Can be null, meaning takes no arguments.
                const Type* const*  argTypes;
                /// Amount of arguments. Is the length of `argTypes`.
                uint16_t            argLen;
            };  

            void*       _boolInfo;
            Int         intInfo;
            Float       floatInfo;
            //_unused     charInfo;
            void*       _strinSliceInfo;
            void*       _stringInfo;
            Reference   referenceInfo;
            Function    functionInfo;

            constexpr ExtraInfo() : _boolInfo(nullptr) {}
            constexpr ExtraInfo(Int inIntInfo) : intInfo(inIntInfo) {}
            constexpr ExtraInfo(Float inFloatInfo) : floatInfo(inFloatInfo) {}
            constexpr ExtraInfo(Reference inReferenceInfo) : referenceInfo(inReferenceInfo) {}
            constexpr ExtraInfo(Function inFunctionInfo) : functionInfo(inFunctionInfo) {}
        };

        size_t          sizeType;
        /// Alignment of the type in bytes. Alignment beyond UINT16_MAX is unsupported. 
        uint16_t        alignType;
        StringSlice     name;
        const Function* optionalDestructor = nullptr;
        const Function* optionalEquality = nullptr;
        Tag             tag;
        ExtraInfo       extra;
        const Type*     constRef;
        const Type*     mutRef;

        template<typename T>
        static const Type* makeType(
            StringSlice inName, 
            Tag inTag, 
            ExtraInfo inExtra, 
            const Function* inOptionalDestructor = nullptr
        ) {
            static const Type* actualType = createType<T>(inName, inTag, inExtra, inOptionalDestructor);
            return  actualType;
        }

        template<typename T>
        void destroyObject(T* obj) const {
            if constexpr(!std::is_same<T, void>::value) {
                this->assertTypeSizeAlignMatch(sizeof(T), alignof(T));
            }
            this->destroyObjectImpl(reinterpret_cast<void*>(obj));
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
        //static const Type* const TYPE_CHAR;
        static const Type* const TYPE_STRING_SLICE;
        static const Type* const TYPE_STRING;

    private:

        template<typename T>
        static Function::c_function_t makeEqualityFunction() {
            Function::c_function_t func = [](Function::CHandler handler) -> ProgramRuntimeError {
                const T* lhs = handler.takeArg<const T*>(0);
                const T* rhs = handler.takeArg<const T*>(1);
                bool equal = (*lhs) == (*rhs);
                handler.setReturn(std::move(equal));
                return ProgramRuntimeError();
            };
            return func;
        }

        template<typename T>
        static const Type* createType(
            StringSlice inName, 
            Tag inTag, 
            ExtraInfo inExtra, 
            const Function* inOptionalDestructor
        ) {
            static Type concreteType = {
                sizeof(T),                          // sizeType
                static_cast<uint16_t>(alignof(T)),  // alignType
                inName,                             // name
                inOptionalDestructor,               // optionalDestructor
                nullptr,                            // optionalEquality
                inTag,                              // tag
                inExtra,                            // extra
                nullptr,                            // constRef
                nullptr                             // mutRef
            };

            const ExtraInfo::Reference constRefInfo = {false, &concreteType};
            const ExtraInfo::Reference mutRefInfo = {true, &concreteType};

            const ExtraInfo constRefExtra{constRefInfo};
            const ExtraInfo mutRefExtra{mutRefInfo};

            static Type constRefType = {
                sizeof(const T*),
                static_cast<uint16_t>(alignof(const T*)),
                "ConstRef", // TODO proper naming
                nullptr,
                nullptr,
                Tag::Reference,
                constRefExtra,
                nullptr,
                nullptr
            };

            static Type mutRefType = {
                sizeof(T*),
                static_cast<uint16_t>(alignof(T*)),
                "MutRef", // TODO proper naming
                nullptr,
                nullptr,
                Tag::Reference,
                mutRefExtra,
                nullptr,
                nullptr
            };

            concreteType.constRef = &constRefType;
            concreteType.mutRef = &mutRefType;

            // TODO is this smart?
            constRefType.constRef = &constRefType;
            constRefType.mutRef = &mutRefType;
            mutRefType.constRef = &constRefType;
            mutRefType.mutRef = &mutRefType;

            // Equality
            if constexpr(detail::has_operator_equal<T>::value) {
                Function::c_function_t func = makeEqualityFunction<T>();
                static const Type* argsTypes[2] = {&constRefType, &constRefType};
                static Function cfunc = {
                    "==", // name
                    "==", // identifier name
                    Type::TYPE_BOOL, // return type
                    argsTypes,
                    2, // number of args
                    SY_FUNCTION_MIN_ALIGN, // alignment
                    Function::CallType::C,
                    reinterpret_cast<const void*>(func)
                };
                concreteType.optionalEquality = &cfunc;
            }

            return &concreteType;
        }
    
        void assertTypeSizeAlignMatch(size_t sizeOfType, size_t alignOfType) const;
        void destroyObjectImpl(void* obj) const;
    };
}

#endif // SY_TYPES_TYPE_HPP_