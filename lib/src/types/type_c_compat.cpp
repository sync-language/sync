// The few global constants for the primitive types as Sync compatible RTTI are defined in
// `type.cpp`. The types conflict on some compilers naturally, so splitting the inclusion of
// `"type.h" is necessary. Most of the other extern "C" stuff elsewhere in the repo is just for
// functions and runtime reinterpret_cast, so those are fine, just the compile time global variables
// mess up.

#include "type.h"
#include "type.hpp"
#include <cstddef>

static_assert(sizeof(SyType) == sizeof(sy::Type));
static_assert(alignof(SyType) == alignof(sy::Type));
static_assert(offsetof(SyType, base) == offsetof(sy::Type, base));
static_assert(offsetof(SyType, indirection) == offsetof(sy::Type, indirection));
static_assert(offsetof(SyType, mutableBits) == offsetof(sy::Type, mutableBits));

static_assert(sizeof(SyTypeExtraTag) == sizeof(sy::TypeExtra::Tag));
static_assert(SyTypeExtraTagBool == static_cast<int>(sy::TypeExtra::Tag::Bool));
static_assert(SyTypeExtraTagInt == static_cast<int>(sy::TypeExtra::Tag::Int));
static_assert(SyTypeExtraTagFloat == static_cast<int>(sy::TypeExtra::Tag::Float));
static_assert(SyTypeExtraTagOpaquePointer == static_cast<int>(sy::TypeExtra::Tag::OpaquePointer));
static_assert(SyTypeExtraTagString == static_cast<int>(sy::TypeExtra::Tag::String));
static_assert(SyTypeExtraTagStringSlice == static_cast<int>(sy::TypeExtra::Tag::StringSlice));
static_assert(SyTypeExtraTagOrdering == static_cast<int>(sy::TypeExtra::Tag::Ordering));
static_assert(SyTypeExtraTagReference == static_cast<int>(sy::TypeExtra::Tag::Reference));
static_assert(SyTypeExtraTagFunction == static_cast<int>(sy::TypeExtra::Tag::Function));

static_assert(sizeof(SyTypeExtraInt) == sizeof(sy::TypeExtra::Int));
static_assert(alignof(SyTypeExtraInt) == alignof(sy::TypeExtra::Int));
static_assert(offsetof(SyTypeExtraInt, isSigned) == offsetof(sy::TypeExtra::Int, isSigned));
static_assert(offsetof(SyTypeExtraInt, bits) == offsetof(sy::TypeExtra::Int, bits));

static_assert(sizeof(SyTypeExtraFloat) == sizeof(sy::TypeExtra::Float));
static_assert(alignof(SyTypeExtraFloat) == alignof(sy::TypeExtra::Float));
static_assert(offsetof(SyTypeExtraFloat, bits) == offsetof(sy::TypeExtra::Float, bits));

static_assert(sizeof(SyTypeExtraReference) == sizeof(sy::TypeExtra::Reference));
static_assert(alignof(SyTypeExtraReference) == alignof(sy::TypeExtra::Reference));
static_assert(offsetof(SyTypeExtraReference, isMutable) ==
              offsetof(sy::TypeExtra::Reference, isMutable));
static_assert(offsetof(SyTypeExtraReference, childType) ==
              offsetof(sy::TypeExtra::Reference, childType));

static_assert(sizeof(SyTypeExtraFunction) == sizeof(sy::TypeExtra::Function));
static_assert(alignof(SyTypeExtraFunction) == alignof(sy::TypeExtra::Function));
static_assert(offsetof(SyTypeExtraFunction, retType) == offsetof(sy::TypeExtra::Function, retType));
static_assert(offsetof(SyTypeExtraFunction, argTypes) ==
              offsetof(sy::TypeExtra::Function, argTypes));
static_assert(offsetof(SyTypeExtraFunction, argLen) == offsetof(sy::TypeExtra::Function, argLen));

static_assert(sizeof(SyTypeExtraInfo) == sizeof(sy::TypeExtra::Info));
static_assert(alignof(SyTypeExtraInfo) == alignof(sy::TypeExtra::Info));

static_assert(sizeof(SyTypeExtra) == sizeof(sy::TypeExtra));
static_assert(alignof(SyTypeExtra) == alignof(sy::TypeExtra));
static_assert(offsetof(SyTypeExtra, tag) == offsetof(sy::TypeExtra, tag));
static_assert(offsetof(SyTypeExtra, info) == offsetof(sy::TypeExtra, info));

static_assert(sizeof(SyTypeMetadata) == sizeof(sy::TypeMetadata));
static_assert(alignof(SyTypeMetadata) == alignof(sy::TypeMetadata));
static_assert(offsetof(SyTypeMetadata, typeSize) == offsetof(sy::TypeMetadata, typeSize));
static_assert(offsetof(SyTypeMetadata, typeAlign) == offsetof(sy::TypeMetadata, typeAlign));
static_assert(offsetof(SyTypeMetadata, name) == offsetof(sy::TypeMetadata, name));
static_assert(offsetof(SyTypeMetadata, extra) == offsetof(sy::TypeMetadata, extra));
static_assert(offsetof(SyTypeMetadata, destructor) == offsetof(sy::TypeMetadata, destructor));
static_assert(offsetof(SyTypeMetadata, builtinTraits) == offsetof(sy::TypeMetadata, builtinTraits));

extern "C" {
SY_API bool sy_type_is_indirection_level_mutable(const SyType* self, uint32_t level) {
    return reinterpret_cast<const sy::Type*>(self)->isIndirectionLevelMutable(level);
}

SY_API SyType sy_type_reference(const SyType* self, bool isMutable) {
    auto res = reinterpret_cast<const sy::Type*>(self)->reference(isMutable);
    return SyType{.base = reinterpret_cast<const SyTypeMetadata*>(res.base),
                  .indirection = res.indirection,
                  .mutableBits = res.mutableBits};
}

SY_API SyType sy_type_dereference(const SyType* self) {
    auto res = reinterpret_cast<const sy::Type*>(self)->dereference();
    return SyType{.base = reinterpret_cast<const SyTypeMetadata*>(res.base),
                  .indirection = res.indirection,
                  .mutableBits = res.mutableBits};
}

SY_API bool sy_type_is_reference(const SyType* self) {
    return reinterpret_cast<const sy::Type*>(self)->isReference();
}

SY_API bool sy_type_eq(const SyType* self, const SyType* other) {
    return (*reinterpret_cast<const sy::Type*>(self)) ==
           (*reinterpret_cast<const sy::Type*>(other));
}
}
