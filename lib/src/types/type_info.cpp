#include "type_info.h"
#include "function/function.h"
#include "type_info.hpp"
#include "function/function.hpp"
#include "../util/assert.hpp"
#include "string/string_slice.hpp"
#include "string/string.hpp"
#include "../program/program.hpp"

using sy::Type;
using sy::Function;
using sy::StringSlice;
using sy::String;

static_assert(sizeof(float) == 4); // f32
static_assert(sizeof(double) == 8); // f64

static_assert(sizeof(Type::Tag) == sizeof(SyTypeTag));
static_assert(alignof(Type::Tag) == alignof(SyTypeTag));
static_assert(sizeof(Type::Tag) == sizeof(int));
static_assert(alignof(Type::Tag) == alignof(int));
static_assert(static_cast<int>(Type::Tag::Bool) == static_cast<int>(SyTypeTagBool));
static_assert(static_cast<int>(Type::Tag::Int) == static_cast<int>(SyTypeTagInt));
static_assert(static_cast<int>(Type::Tag::Float) == static_cast<int>(SyTypeTagFloat));
static_assert(static_cast<int>(Type::Tag::StringSlice) == static_cast<int>(SyTypeTagStringSlice));
static_assert(static_cast<int>(Type::Tag::String) == static_cast<int>(SyTypeTagString));
static_assert(static_cast<int>(Type::Tag::Reference) == static_cast<int>(SyTypeTagReference));
static_assert(static_cast<int>(Type::Tag::Function) == static_cast<int>(SyTypeTagFunction));

static_assert(sizeof(Type::ExtraInfo) == sizeof(SyTypeExtraInfo));
static_assert(alignof(Type::ExtraInfo) == alignof(SyTypeExtraInfo));

static_assert(sizeof(Type::ExtraInfo::Int) == sizeof(SyTypeInfoInt));
static_assert(alignof(Type::ExtraInfo::Int) == alignof(SyTypeInfoInt));
static_assert(offsetof(Type::ExtraInfo::Int, isSigned) == offsetof(SyTypeInfoInt, isSigned));
static_assert(offsetof(Type::ExtraInfo::Int, bits) == offsetof(SyTypeInfoInt, bits));

static_assert(sizeof(Type::ExtraInfo::Float) == sizeof(SyTypeInfoFloat));
static_assert(alignof(Type::ExtraInfo::Float) == alignof(SyTypeInfoFloat));
static_assert(offsetof(Type::ExtraInfo::Float, bits) == offsetof(SyTypeInfoFloat, bits));

static_assert(sizeof(Type::ExtraInfo::Reference) == sizeof(SyTypeInfoReference));
static_assert(alignof(Type::ExtraInfo::Reference) == alignof(SyTypeInfoReference));
static_assert(offsetof(Type::ExtraInfo::Reference, isMutable) == offsetof(SyTypeInfoReference, isMutable));
static_assert(offsetof(Type::ExtraInfo::Reference, childType) == offsetof(SyTypeInfoReference, childType));

static_assert(sizeof(Type::ExtraInfo::Function) == sizeof(SyTypeInfoFunction));
static_assert(alignof(Type::ExtraInfo::Function) == alignof(SyTypeInfoFunction));
static_assert(offsetof(Type::ExtraInfo::Function, retType) == offsetof(SyTypeInfoFunction, retType));
static_assert(offsetof(Type::ExtraInfo::Function, argTypes) == offsetof(SyTypeInfoFunction, argTypes));
static_assert(offsetof(Type::ExtraInfo::Function, argLen) == offsetof(SyTypeInfoFunction, argLen));

static_assert(sizeof(Type) == sizeof(SyType));
static_assert(alignof(Type) == alignof(SyType));
static_assert(offsetof(Type, sizeType) == offsetof(SyType, sizeType));
static_assert(offsetof(Type, alignType) == offsetof(SyType, alignType));
static_assert(offsetof(Type, name) == offsetof(SyType, name));
static_assert(offsetof(Type, optionalDestructor) == offsetof(SyType, optionalDestructor));
static_assert(offsetof(Type, tag) == offsetof(SyType, tag));
static_assert(offsetof(Type, extra) == offsetof(SyType, extra));
static_assert(offsetof(Type, constRef) == offsetof(SyType, constRef));
static_assert(offsetof(Type, mutRef) == offsetof(SyType, mutRef));

namespace detail {
    template<typename T>
    void destructorCall(Function::CHandler handler) {
        T obj = handler.takeArg<T>(0);
        // destructor automatically called
    }
}

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

#pragma region Float

const Type* const sy::Type::TYPE_F32 = 
    Type::makeType<float>("f32", Type::Tag::Float, Type::ExtraInfo(Type::ExtraInfo::Float{32}));
const Type* const sy::Type::TYPE_F64 = 
    Type::makeType<double>("f64", Type::Tag::Float, Type::ExtraInfo(Type::ExtraInfo::Float{64}));

#pragma endregion

#pragma region String

//const Type* const sy::Type::TYPE_CHAR = nullptr;
const Type* const sy::Type::TYPE_STRING_SLICE =
    Type::makeType<sy::StringSlice>("str", Type::Tag::StringSlice, Type::ExtraInfo());

static const Type* stringDestructorArgs[1] = {Type::TYPE_STRING};
static const Function stringDestructorCall = {
    StringSlice("sy::String::~String"),
    StringSlice("~String"),
    nullptr,
    stringDestructorArgs,
    1,
    SY_FUNCTION_MIN_ALIGN, // fine for now
    Function::CallType::C,
    reinterpret_cast<const void*>(&detail::destructorCall<String>)
};

const Type* const sy::Type::TYPE_STRING = 
    Type::makeType<sy::String>("String", Type::Tag::String, Type::ExtraInfo(), &stringDestructorCall);

#pragma endregion

extern "C" {
    SY_API const SyType* SY_TYPE_BOOL = reinterpret_cast<const SyType*>(Type::TYPE_BOOL);

    SY_API const SyType* SY_TYPE_I8     = reinterpret_cast<const SyType*>(Type::TYPE_I8);
    SY_API const SyType* SY_TYPE_I16    = reinterpret_cast<const SyType*>(Type::TYPE_I16);
    SY_API const SyType* SY_TYPE_I32    = reinterpret_cast<const SyType*>(Type::TYPE_I32);
    SY_API const SyType* SY_TYPE_I64    = reinterpret_cast<const SyType*>(Type::TYPE_I64);
    SY_API const SyType* SY_TYPE_U8     = reinterpret_cast<const SyType*>(Type::TYPE_U8);
    SY_API const SyType* SY_TYPE_U16    = reinterpret_cast<const SyType*>(Type::TYPE_U16);
    SY_API const SyType* SY_TYPE_U32    = reinterpret_cast<const SyType*>(Type::TYPE_U32);
    SY_API const SyType* SY_TYPE_U64    = reinterpret_cast<const SyType*>(Type::TYPE_U64);
    SY_API const SyType* SY_TYPE_USIZE  = reinterpret_cast<const SyType*>(Type::TYPE_USIZE);

    SY_API const SyType* SY_TYPE_F32 = reinterpret_cast<const SyType*>(Type::TYPE_F32);
    SY_API const SyType* SY_TYPE_F64 = reinterpret_cast<const SyType*>(Type::TYPE_F64);

    //SY_API const SyType* SY_TYPE_CHAR           = reinterpret_cast<const SyType*>(Type::TYPE_CHAR);
    SY_API const SyType* SY_TYPE_STRING         = reinterpret_cast<const SyType*>(Type::TYPE_STRING);
    SY_API const SyType* SY_TYPE_STRING_SLICE   = reinterpret_cast<const SyType*>(Type::TYPE_STRING_SLICE);
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
        //case Tag::Char:
        case Tag::StringSlice:
        case Tag::Reference: 
        case Tag::Function: return;
        
        case Tag::String: {
            String* objAsString = reinterpret_cast<String*>(obj);
            objAsString->~String();
        } return;
            
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

TEST_CASE("string destructor") {
    // create with new so that destructor doesn't automatically get called 
    String* s = new String("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    Type::TYPE_STRING->destroyObject(s);
}

#endif
