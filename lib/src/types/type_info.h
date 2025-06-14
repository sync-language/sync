//! API
#pragma once
#ifndef SY_TYPES_TYPE_H_
#define SY_TYPES_TYPE_H_

#include "../core.h"
#include "string/string_slice.h"

struct SyType;
struct SyFunction;

typedef enum SyTypeTag {
    /// Maps to a singular `SyType` instance.
    SyTypeTagBool = 0,
    /// Maps to a few `SyType` instances, depending on bit width and signed-ness.
    /// The options are signed or unsigned, as well as bit widths of 8, 16, 32, and 64.
    SyTypeTagInt = 1,
    /// Maps to 2 `SyType` instances, depending on bit width. The options are 32 or 64 bits.
    SyTypeTagFloat = 2,
    /// Maps to a singular `SyType` instance.
    //SyTypeTagChar = 3,
    /// Maps to a singular `SyType` instance. Is an owned string. For string references, see `SyTypeTagStringSlice`.
    SyTypeTagString = 4,
    /// Maps to a singular `SyType` instance.
    SyTypeTagStringSlice = 5,
    /// Maps to a singular `SyType` instance.
    /// TODO figure out how to handle unordered data, such as float NaN.
    SyTypeTagOrdering = 6,
    /// Maps to effectively infinite `SyType` instances, depending on the references type and mutability.
    SyTypeTagReference = 7,
    /// Maps to effectively infinite `SyType` instances, depending on the type of each array element,
    /// as well as if it's a dynamic or static array (and size of array for static). Holds ownership over the values. 
    /// For array references, see `SyTypeTagSlice`.
    SyTypeTagArray = 8,
    /// Maps to effectively infinite `SyType` instances, depending on the type of each set element.
    /// Holds ownership over the values.
    SyTypeTagSet = 9,
    /// Maps to effectively infinite `SyType` instances, depending on the type of keys and values.
    /// Holds ownership over the keys and values.
    SyTypeTagMap = 10,
    /// Maps to effectively infinite `SyType` instances, depending on the type of each array element, and mutability.
    SyTypeTagSlice = 11,
    /// Maps to effectively infinite `SyType` instances, depending on the type of the optional value.
    /// Can perform special optimizations for types with an "invalid" state. These types are references, slices,
    /// function pointers, and the sync pointer types,which aren't allowed to be nulled. Weak sync pointers may
    /// appear null when invalidated, but they still hold a valid pointer to the freed object.
    SyTypeTagOption = 12,
    /// Maps to effectively infinite `SyType` instances, depending on the optional type of the error value.
    /// TODO For the error type, look into other really good error types, such as anyhow and thiserror for Rust. 
    /// Having stack traces in error may be really smart
    /// https://docs.rs/anyhow/latest/anyhow/ https://github.com/dtolnay/thiserror
    SyTypeTagError = 13,
    /// Maps to effectively infinite `SyType` instances, depending on the type of the ok value and the error value.
    /// Holds ownership over the values.
    SyTypeTagResult = 14,
    /// Maps to a few `SyType` instances, depending on the type of each component for the integer and float types, 
    /// along with the dimensions, being `2`, `3`, or `4`. These can map to GLSL types.
    SyTypeTagVector = 15,
    /// Maps to a few `SyType` instances, depending on the `32` or `64` bit floats, along with the X and Y dimensions,
    /// each of which can be `2`, `3`, or `4`. These can map to GLSL types.
    /// TODO figure out column vs row major
    SyTypeTagMatrix = 16,
    /// Maps to effectively infinite `SyType` instances, depending on the return type and argument types.
    /// Is a function pointer. Doesn't handle member binding.
    SyTypeTagFunction = 17,
    /// Maps to effectively infinite `SyType` instances, depending on the object type, and whether it's
    /// single ownership, shared ownership, or weak referencing.
    SyTypeTagSync = 18,
    /// Maps to effectively infinite `SyType` instances, depending on the struct name (and relevant namespacing),
    /// it's members, and various other factors.
    SyTypeTagStruct = 19,

    // Enforce 32 bit size
    _SY_TYPE_TAG_MAX_ENUM = 0x7FFFFFFF
} SyTypeTag;

typedef struct SyTypeInfoInt {
    /// If `true`, this is a signed integer, otherwise unsigned.
    bool    isSigned;
    /// Must be one of `8`, `16`, `32`, or `64`.
    uint8_t bits;
} SyTypeInfoInt;

typedef struct SyTypeInfoFloat {
    /// Must be `32` or `64`.
    uint8_t bits;
} SyTypeInfoFloat;

typedef struct SyTypeInfoReference {
    bool            isMutable;
    const SyType*   childType;
} SyTypeInfoReference;

typedef struct SyTypeInfoFunction {
    /// Can be null, meaning has no return type.
    const struct SyType* retType;
    /// Can be null, meaning takes no arguments.
    const struct SyType* const* argTypes;
    /// Amount of arguments. Is the length of `argTypes`.
    uint16_t argLen;
} SyTypeInfoFunction;

typedef union SyTypeExtraInfo {
    void*               _boolInfo;
    SyTypeInfoInt       intInfo;
    SyTypeInfoFloat     floatInfo;
    //void*    _charInfo;
    void*               _stringSliceInfo;
    void*               _stringInfo;
    SyTypeInfoFunction  functionInfo;
    SyTypeInfoReference referenceInfo;
} SyTypeExtraInfo;

typedef struct SyType {
    /// Actual size of the type in bytes.
    size_t              sizeType;
    /// Alignment of the type in bytes. Alignment beyond UINT16_MAX is unsupported. 
    uint16_t            alignType;
    SyStringSlice       name;
    const SyFunction*   optionalDestructor;
    /// Used as a tagged union with the payload being `extra`.
    SyTypeTag           tag;
    /// Used as a tagged union, with the tags being `tag`.
    SyTypeExtraInfo     extra;
    const SyType*       constRef;
    const SyType*       mutRef;
} SyType;


#ifdef __cplusplus
extern "C" {
#endif

SY_API extern const SyType* SY_TYPE_BOOL;
SY_API extern const SyType* SY_TYPE_I8;
SY_API extern const SyType* SY_TYPE_I16; 
SY_API extern const SyType* SY_TYPE_I32;
SY_API extern const SyType* SY_TYPE_I64;
SY_API extern const SyType* SY_TYPE_U8;
SY_API extern const SyType* SY_TYPE_U16;
SY_API extern const SyType* SY_TYPE_U32;
SY_API extern const SyType* SY_TYPE_U64;
SY_API extern const SyType* SY_TYPE_USIZE;
SY_API extern const SyType* SY_TYPE_F32;
SY_API extern const SyType* SY_TYPE_F64;
//SY_API extern const SyType* SY_TYPE_CHAR;
SY_API extern const SyType* SY_TYPE_STRING_SLICE;
SY_API extern const SyType* SY_TYPE_STRING;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SY_TYPES_TYPE_H_