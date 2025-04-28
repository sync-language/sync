//! API
#pragma once
#ifndef SY_TYPES_TYPE_H_
#define SY_TYPES_TYPE_H_

#include "../core.h"

struct SyType;

typedef enum SyTypeTag {
    /// Maps to a singular `SyType` instance.
    SyTypeTagBool = 0,
    /// Maps to a few `SyType` instances, depending on bit width and signed-ness.
    /// The options are signed or unsigned, as well as bit widths of 8, 16, 32, and 64.
    SyTypeTagInt = 1,
    /// Maps to 2 `SyType` instances, depending on bit width. The options are 32 or 64 bits.
    SyTypeTagFloat = 2,
    /// Maps to a singular `SyType` instance.
    SyTypeTagChar = 3,
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

typedef struct SyBaseTypeInfo {
    /// Actual size of the type in bytes.
    size_t      sizeType;
    /// Alignment of the type. For now, `alignType <= 8` is required.
    /// TODO support alignments greater than 8 for 64 bit
    uint8_t     alignType;
    /// Is a non-null utf8 character array up to `nameLength` in size in bytes. Does not need to be null terminated.
    const char* name;
    /// Amount of bytes of `name`.
    size_t      nameLength;
    
    // TODO add functions
} SyBaseTypeInfo;

/// Internal use only. Used for type requirements on C unions.
typedef void* _sy_type_extra_info_unused_t;

typedef struct SyTypeInfoInt {
    /// If `true`, this is a signed integer, otherwise unsigned.
    bool    isSigned;
    /// Must be one of `8`, `16`, `32`, or `64`.
    uint8_t bits;
} SyTypeInfoInt;

typedef union SyTypeExtraInfo {
    _sy_type_extra_info_unused_t    _boolInfo;
    SyTypeInfoInt                   intInfo;
} SyTypeExtraInfo;

typedef struct SyType {
    SyBaseTypeInfo  baseInfo;
    /// Used as a tagged union with the payload being `extra`.
    SyTypeTag       tag;
    /// Used as a tagged union, with the tags being `tag`.
    SyTypeExtraInfo extra;
} SyType;

#endif // SY_TYPES_TYPE_H_