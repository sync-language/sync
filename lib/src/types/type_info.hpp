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
        using SyType = SyType;
    }

    struct SY_API Type {
        enum class Tag : int32_t {
            Bool = c::SyTypeTag::SyTypeTagBool,
            Int = c::SyTypeTag::SyTypeTagInt,
            Float = c::SyTypeTag::SyTypeTagFloat,
            Reference = c::SyTypeTag::SyTypeTagReference,
        };

        union SY_API ExtraInfo {
            using unused = void*;
            using Int = c::SyTypeInfoInt;
            using Float = c::SyTypeInfoFloat;
            
            struct Reference {
                bool        isMutable;
                const Type* childType;
            };

            unused      _boolInfo;
            Int         intInfo;
            Float       floatInfo;
            Reference   referenceInfo;

            constexpr ExtraInfo() : _boolInfo(nullptr) {}
            constexpr ExtraInfo(Int inIntInfo) : intInfo(inIntInfo) {}
            constexpr ExtraInfo(Float inFloatInfo) : floatInfo(inFloatInfo) {}
            constexpr ExtraInfo(Reference inReferenceInfo) : referenceInfo(inReferenceInfo) {}
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
        static constexpr const Type* makeType(
            StringSlice inName, 
            Tag inTag, 
            ExtraInfo inExtra, 
            const Function* inOptionalDestructor = nullptr
        ) {
            return createType<T>(inName, inTag, inExtra, inOptionalDestructor);      
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

    private:

        template<typename T>
        static constexpr const Type* createType(
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
    };
}

#endif // SY_TYPES_TYPE_HPP_