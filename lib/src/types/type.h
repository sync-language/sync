//! API
#pragma once
// #ifndef SY_TYPES_TYPE_H_
// #define SY_TYPES_TYPE_H_

#include "../core/core.h"
#include "string/string_slice.h"

struct SyTypeMetadata;
struct SyFunction;
struct SyBuiltInCoherentTraits;

typedef struct SyType {
    const struct SyTypeMetadata* base;
    uint32_t indirection;
    uint32_t mutableBits;
} SyType;

typedef enum SyTypeExtraTag {
    SyTypeExtraTagBool = 0,
    SyTypeExtraTagInt = 1,
    SyTypeExtraTagFloat = 2,
    SyTypeExtraTagOpaquePointer = 3,
    SyTypeExtraTagString = 4,
    SyTypeExtraTagStringSlice = 5,
    SyTypeExtraTagOrdering = 6,
    SyTypeExtraTagReference = 7,
    SyTypeExtraTagFunction = 18,

    _SY_TYPE_EXTRA_TAG_MAX_ENUM = 0x7FFFFFFF
} SyTypeExtraTag;

typedef struct SyTypeExtraInt {
    bool isSigned;
    uint8_t bits;
} SyTypeExtraInt;

typedef struct SyTypeExtraFloat {
    uint8_t bits;
} SyTypeExtraFloat;

typedef struct SyTypeExtraReference {
    bool isMutable;
    SyType childType;
} SyTypeExtraReference;

typedef struct SyTypeExtraFunction {
    SyType retType;
    const SyType* argTypes;
    uint16_t argLen;
} SyTypeExtraFunction;

typedef union SyTypeExtraInfo {
    void* _boolInfo;
    SyTypeExtraInt intInfo;
    SyTypeExtraFloat floatInfo;
    void* _stringSliceInfo;
    void* _stringInfo;
    SyTypeExtraReference referenceInfo;
    SyTypeExtraFunction functionInfo;
} SyTypeExtraInfo;

typedef struct SyTypeExtra {
    SyTypeExtraTag tag;
    SyTypeExtraInfo info;
} SyTypeExtra;

typedef struct SyTypeMetadata {
    size_t typeSize;
    size_t typeAlign;
    SyStringSlice name;
    SyTypeExtra extra;
    const SyFunction* destructor;
    const SyBuiltInCoherentTraits* builtinTraits;
} SyTypeMetadata;

#ifdef __cplusplus
extern "C" {
#endif

SY_API bool sy_type_is_indirection_level_mutable(const SyType* self, uint32_t level);

SY_API SyType sy_type_reference(const SyType* self, bool isMutable);

SY_API SyType sy_type_dereference(const SyType* self);

SY_API bool sy_type_is_reference(const SyType* self);

SY_API bool sy_type_eq(const SyType* self, const SyType* other);

SY_API extern const SyType SY_TYPE_BOOL;
SY_API extern const SyType SY_TYPE_I8;
SY_API extern const SyType SY_TYPE_I16;
SY_API extern const SyType SY_TYPE_I32;
SY_API extern const SyType SY_TYPE_I64;
SY_API extern const SyType SY_TYPE_U8;
SY_API extern const SyType SY_TYPE_U16;
SY_API extern const SyType SY_TYPE_U32;
SY_API extern const SyType SY_TYPE_U64;
SY_API extern const SyType SY_TYPE_USIZE;
SY_API extern const SyType SY_TYPE_F32;
SY_API extern const SyType SY_TYPE_F64;
SY_API extern const SyType SY_TYPE_STRING_SLICE;
SY_API extern const SyType SY_TYPE_STRING;
SY_API extern const SyType SY_TYPE_ORDERING;
SY_API extern const SyType SY_TYPE_OPAQUE_PTR;

#ifdef __cplusplus
} // extern "C"
#endif

// #endif // SY_TYPES_TYPE_H_
