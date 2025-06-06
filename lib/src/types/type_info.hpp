//! API
#pragma once
#ifndef SY_TYPES_TYPE_HPP_
#define SY_TYPES_TYPE_HPP_

#include "../core.h"
#include "string/string_slice.hpp"
#include <type_traits>

namespace sy {
    class Function;

    namespace c {
        #include "type_info.h"

        using SyTypeTag = SyTypeTag;
        using SyTypeInfoInt = SyTypeInfoInt;
        using SyTypeInfoFloat = SyTypeInfoFloat;
        using SyTypeInfoReference = SyTypeInfoReference;
        using SyTypeInfoFunction = SyTypeInfoFunction;
        using SyType = SyType;
    }

    struct SY_API Type {
        enum class Tag : int32_t {
            Bool = c::SyTypeTag::SyTypeTagBool,
            Int = c::SyTypeTag::SyTypeTagInt,
            Float = c::SyTypeTag::SyTypeTagFloat,
            //Char = c::SyTypeTag::SyTypeTagChar,
            StringSlice = c::SyTypeTag::SyTypeTagStringSlice,
            String = c::SyTypeTag::SyTypeTagString,
            Reference = c::SyTypeTag::SyTypeTagReference,
            Function = c::SyTypeTag::SyTypeTagFunction,
        };

        union SY_API ExtraInfo {
            using _unused = void*;
            using Int = c::SyTypeInfoInt;
            using Float = c::SyTypeInfoFloat;
            using Function = c::SyTypeInfoFunction;
            
            struct Reference {
                bool        isMutable;
                const Type* childType;
            };

            _unused     _boolInfo;
            Int         intInfo;
            Float       floatInfo;
            //_unused     charInfo;
            _unused     _strinSliceInfo;
            _unused     _stringInfo;
            Reference   referenceInfo;
            Function    functionInfo;

            constexpr ExtraInfo() : _boolInfo(nullptr) {}
            constexpr ExtraInfo(Int inIntInfo) : intInfo(inIntInfo) {}
            constexpr ExtraInfo(Float inFloatInfo) : floatInfo(inFloatInfo) {}
            constexpr ExtraInfo(Reference inReferenceInfo) : referenceInfo(inReferenceInfo) {}
            constexpr ExtraInfo(Function inFunctionInfo) : functionInfo(inFunctionInfo) {}
        };

        size_t      sizeType;
        /// Alignment of the type in bytes. Alignment beyond UINT16_MAX is unsupported. 
        uint16_t    alignType;
        StringSlice name;
        const Function* optionalDestructor = nullptr;
        Tag         tag;
        ExtraInfo   extra;
        const Type* constRef;
        const Type* mutRef;

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