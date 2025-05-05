#include "type_info.hpp"

using sy::Type;

static_assert(sizeof(sy::Type) == sizeof(sy::c::SyType));
static_assert(alignof(sy::Type) == alignof(sy::c::SyType));

namespace sy::detail {
    static const sy::Type boolTypeInfo = Type(sizeof(bool), alignof(bool), "bool", Type::Tag::Bool, Type::ExtraInfo());

    static const sy::Type i8TypeInfo = 
        Type(sizeof(int8_t), alignof(int8_t), "i8", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{true, 8}));
    static const sy::Type i16TypeInfo = 
        Type(sizeof(int16_t), alignof(int16_t), "i16", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{true, 16}));
    static const sy::Type i32TypeInfo = 
        Type(sizeof(int32_t), alignof(int32_t), "i32", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{true, 32}));
    static const sy::Type i64TypeInfo = 
        Type(sizeof(int64_t), alignof(int64_t), "i64", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{true, 64}));
    static const sy::Type u8TypeInfo = 
        Type(sizeof(uint8_t), alignof(uint8_t), "u8", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{false, 8}));
    static const sy::Type u16TypeInfo = 
        Type(sizeof(uint16_t), alignof(uint16_t), "u16", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{false, 16}));
    static const sy::Type u32TypeInfo = 
        Type(sizeof(uint32_t), alignof(uint32_t), "u32", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{false, 32}));
    static const sy::Type u64TypeInfo = 
        Type(sizeof(uint64_t), alignof(uint64_t), "u64", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{false, 64}));  
    static const sy::Type usizeTypeInfo = 
        Type(sizeof(size_t), alignof(size_t), "usize", Type::Tag::Int, Type::ExtraInfo(
            Type::ExtraInfo::Int{false, sizeof(size_t) * 8} // amount of bytes * 8 bits per byte
        ));

    static const sy::Type f32TypeInfo = 
        Type(sizeof(float), alignof(float), "f32", Type::Tag::Float, Type::ExtraInfo(Type::ExtraInfo::Float{32}));
    static const sy::Type f64TypeInfo = 
        Type(sizeof(double), alignof(double), "f64", Type::Tag::Float, Type::ExtraInfo(Type::ExtraInfo::Float{64})); 
}

const Type* const sy::Type::TYPE_BOOL = &sy::detail::boolTypeInfo;
const Type* const sy::Type::TYPE_I8 = &sy::detail::i8TypeInfo;
const Type* const sy::Type::TYPE_I16 = &sy::detail::i16TypeInfo;
const Type* const sy::Type::TYPE_I32 = &sy::detail::i32TypeInfo;
const Type* const sy::Type::TYPE_I64 = &sy::detail::i64TypeInfo;
const Type* const sy::Type::TYPE_U8 = &sy::detail::u8TypeInfo;
const Type* const sy::Type::TYPE_U16 = &sy::detail::u16TypeInfo;
const Type* const sy::Type::TYPE_U32 = &sy::detail::u32TypeInfo;
const Type* const sy::Type::TYPE_U64 = &sy::detail::u64TypeInfo;
const Type* const sy::Type::TYPE_USIZE = &sy::detail::usizeTypeInfo;
const Type* const sy::Type::TYPE_F32 = &sy::detail::f32TypeInfo;
const Type* const sy::Type::TYPE_F64 = &sy::detail::f64TypeInfo;

extern "C" {
    using sy::c::SyType;

    SY_API const SyType* SY_TYPE_BOOL = reinterpret_cast<const SyType*>(Type::TYPE_BOOL);
    SY_API const SyType* SY_TYPE_I8 = reinterpret_cast<const SyType*>(Type::TYPE_I8);
    SY_API const SyType* SY_TYPE_I16 = reinterpret_cast<const SyType*>(Type::TYPE_I16);
    SY_API const SyType* SY_TYPE_I32 = reinterpret_cast<const SyType*>(Type::TYPE_I32);
    SY_API const SyType* SY_TYPE_I64 = reinterpret_cast<const SyType*>(Type::TYPE_I64);
    SY_API const SyType* SY_TYPE_U8 = reinterpret_cast<const SyType*>(Type::TYPE_U8);
    SY_API const SyType* SY_TYPE_U16 = reinterpret_cast<const SyType*>(Type::TYPE_U16);
    SY_API const SyType* SY_TYPE_U32 = reinterpret_cast<const SyType*>(Type::TYPE_U32);
    SY_API const SyType* SY_TYPE_U64 = reinterpret_cast<const SyType*>(Type::TYPE_U64);
    SY_API const SyType* SY_TYPE_USIZE = reinterpret_cast<const SyType*>(Type::TYPE_USIZE);
    SY_API const SyType* SY_TYPE_F32 = reinterpret_cast<const SyType*>(Type::TYPE_F32);
    SY_API const SyType* SY_TYPE_F64 = reinterpret_cast<const SyType*>(Type::TYPE_F64);
}

#ifdef SYNC_LIB_TEST

#include "../doctest.h"

TEST_CASE("same object") {
    const size_t cppBoolPtr = reinterpret_cast<size_t>(sy::Type::TYPE_BOOL);
    const size_t cBoolPtr = reinterpret_cast<size_t>(sy::c::SY_TYPE_BOOL);
    CHECK_EQ(cppBoolPtr, cBoolPtr);
}

#endif
