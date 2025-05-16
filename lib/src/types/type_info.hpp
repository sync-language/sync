//! API
#pragma once
#ifndef SY_TYPES_TYPE_HPP_
#define SY_TYPES_TYPE_HPP_

#include "../core.h"
#include "string/string_slice.hpp"
#include <type_traits>

namespace sy {
    namespace c {
        #include "type_info.h"

        using SyTypeTag = SyTypeTag;
        using SyTypeInfoInt = SyTypeInfoInt;
        using SyTypeInfoFloat = SyTypeInfoFloat;
    }

    struct SY_API Type {
        enum class Tag : int32_t {
            Bool = c::SyTypeTag::SyTypeTagBool,
            Int = c::SyTypeTag::SyTypeTagInt,
            Float = c::SyTypeTag::SyTypeTagFloat,
        };

        union SY_API ExtraInfo {
            using unused = void*;
            using Int = c::SyTypeInfoInt;
            using Float = c::SyTypeInfoFloat;

            unused  _boolInfo;
            Int     intInfo;
            Float   floatInfo;

            constexpr ExtraInfo() : _boolInfo(nullptr) {}
            constexpr ExtraInfo(Int inIntInfo) : intInfo(inIntInfo) {}
            constexpr ExtraInfo(Float inFloatInfo) : floatInfo(inFloatInfo) {}
        };

        size_t      sizeType;
        uint16_t      alignType;
        StringSlice name;
        Tag         tag;
        ExtraInfo   extra;

        constexpr Type(size_t inSize, size_t inAlign, StringSlice inName, Tag inTag, ExtraInfo inExtra)
            : sizeType(inSize), alignType(static_cast<uint16_t>(inAlign)), name(inName), tag(inTag), extra(inExtra)
        {}

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
    };
}

#endif // SY_TYPES_TYPE_HPP_