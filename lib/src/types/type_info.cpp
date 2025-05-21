#include "type_info.h"
#include "function/function.h"
#include "type_info.hpp"
#include "../util/assert.hpp"
#include "function/function.hpp"
#include "../program/program.hpp"

using sy::Type;

static_assert(sizeof(sy::Type) == sizeof(sy::c::SyType));
static_assert(alignof(sy::Type) == alignof(sy::c::SyType));
static_assert(sizeof(float) == 4); // f32
static_assert(sizeof(double) == 8); // f64

static_assert(sizeof(Type::ExtraInfo::Reference) == sizeof(SyTypeInfoReference));
static_assert(alignof(Type::ExtraInfo::Reference) == alignof(SyTypeInfoReference));

const Type* const sy::Type::TYPE_BOOL = Type::makeType<bool>("bool", Type::Tag::Bool, Type::ExtraInfo());
const Type* const sy::Type::TYPE_I8 =
    Type::makeType<int8_t>("i8", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{true, 8}));
const Type* const sy::Type::TYPE_I16 = 
    Type::makeType<int16_t>("i16", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{true, 16}));
const Type* const sy::Type::TYPE_I32 = 
    Type::makeType<int32_t>("i32", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{true, 32}));
const Type* const sy::Type::TYPE_I64 = 
    Type::makeType<int64_t>("i64", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{true, 64}));
const Type* const sy::Type::TYPE_U8 = 
    Type::makeType<uint8_t>("u8", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{false, 8}));
const Type* const sy::Type::TYPE_U16 =
    Type::makeType<uint16_t>("u16", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{false, 16}));
const Type* const sy::Type::TYPE_U32 = 
    Type::makeType<uint32_t>("u32", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{false, 32}));
const Type* const sy::Type::TYPE_U64 = 
    Type::makeType<uint64_t>("u64", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{false, 64}));
const Type* const sy::Type::TYPE_USIZE = 
    Type::makeType<size_t>("usize", Type::Tag::Int, Type::ExtraInfo(
            Type::ExtraInfo::Int{false, sizeof(size_t) * 8} // amount of bytes * 8 bits per byte
        ));
const Type* const sy::Type::TYPE_F32 = 
    Type::makeType<float>("f32", Type::Tag::Float, Type::ExtraInfo(Type::ExtraInfo::Float{32}));
const Type* const sy::Type::TYPE_F64 = 
    Type::makeType<double>("f64", Type::Tag::Float, Type::ExtraInfo(Type::ExtraInfo::Float{64}));

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

void sy::Type::assertTypeSizeAlignMatch(size_t sizeOfType, size_t alignOfType) const
{
    sy_assert(this->sizeType == sizeOfType, "Type size mismatch");
    sy_assert(this->alignType == alignOfType, "Type align mismatch");
}

void sy::Type::destroyObjectImpl(void *obj) const
{
    sy_assert(obj != nullptr, "Cannot destroy null object");
    if(this->optionalDestructor == nullptr) return;

    switch(this->tag) {
        case Tag::Bool:
        case Tag::Int:
        case Tag::Float:
        case Tag::Reference: return;
        default: break;
    }

    sy_assert(this->mutRef != nullptr, "Destructors take mutable references");
    sy_assert(this->mutRef->sizeType == sizeof(void*), 
        "Mutable reference type should have the same size as void*");
    sy_assert(this->mutRef->alignType == alignof(void*), 
        "Mutable reference type should have the same align as void*");

    Function::CallArgs callArgs = this->optionalDestructor->startCall();
    const bool pushSuccess = callArgs.push(&obj, this->mutRef);
    sy_assert(pushSuccess, "TODO overflow when destructor call"); // TODO decide what to do if destructor overflows

    const ProgramRuntimeError err = callArgs.call(nullptr);
    sy_assert(err.ok(), "Destructors may not throw/cause errors"); // TODO what do to if error?
}

#ifdef SYNC_LIB_TEST

#include "../doctest.h"

TEST_CASE("same object") {
    const size_t cppBoolPtr = reinterpret_cast<size_t>(sy::Type::TYPE_BOOL);
    const size_t cBoolPtr = reinterpret_cast<size_t>(SY_TYPE_BOOL);
    CHECK_EQ(cppBoolPtr, cBoolPtr);
}

TEST_CASE("destroy object") {
    size_t n1 = 10;
    size_t n2 = 40;

    sy::Type::TYPE_USIZE->destroyObject(reinterpret_cast<void*>(&n1));
    sy::Type::TYPE_USIZE->destroyObject(&n2);
}

#endif
