//! API
#pragma once
#ifndef SY_TYPES_TYPE_HPP_
#define SY_TYPES_TYPE_HPP_

#include "../core.h"
#include "string/string_slice.hpp"
#include <type_traits>

namespace sy {
    class Function;
    
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
            Function = 17,
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

            struct Function {
                /// Can be null, meaning has no return type.
                const Type*         retType;
                /// Can be null, meaning takes no arguments.
                const Type* const*  argTypes;
                /// Amount of arguments. Is the length of `argTypes`.
                uint16_t            argLen;
            };
            
            struct Reference {
                bool        isMutable;
                const Type* childType;
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
            return createType<T>(inName, inTag, inExtra, inOptionalDestructor);      
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

            return &concreteType;
        }
    
        void assertTypeSizeAlignMatch(size_t sizeOfType, size_t alignOfType) const;
        void destroyObjectImpl(void* obj) const;
    };
}

#endif // SY_TYPES_TYPE_HPP_