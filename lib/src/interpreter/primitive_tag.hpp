#pragma once
#ifndef SY_INTERPRETER_PRIMITIVE_TAG_HPP_
#define SY_INTERPRETER_PRIMITIVE_TAG_HPP_

#include "../core.h"

enum class PrimitiveTag : uint8_t {
    NonPrimitive = 0,
    Bool = 1,
    I8 = 2,
    I16 = 3,
    I32 = 4,
    I64 = 5,
    U8 = 6,
    U16 = 7,
    U32 = 8,
    U64 = 9,
    USize = 10,
    F32 = 11,
    F64 = 12,
};

static constexpr size_t PRIMITIVE_TAG_USED_BITS = 6;

namespace sy {
    struct Type;
}

const sy::Type* primitiveTagToType(PrimitiveTag tag);

#endif // SY_INTERPRETER_PRIMITIVE_TAG_HPP_