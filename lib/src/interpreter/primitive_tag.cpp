#include "primitive_tag.hpp"
#include "../types/type_info.hpp"
#include "../util/assert.hpp"
#include "../util/unreachable.hpp"

const sy::Type *primitiveTagToType(PrimitiveTag tag)
{
    using sy::Type;

    sy_assert(tag != PrimitiveTag::NonPrimitive, "Cannot only get type for actual primitive");

    switch(tag) {
        case PrimitiveTag::Bool: return Type::TYPE_BOOL;
        case PrimitiveTag::I8: return Type::TYPE_I8;
        case PrimitiveTag::I16: return Type::TYPE_I16;
        case PrimitiveTag::I32: return Type::TYPE_I32;
        case PrimitiveTag::I64: return Type::TYPE_I64;
        case PrimitiveTag::U8: return Type::TYPE_U8;
        case PrimitiveTag::U16: return Type::TYPE_U16;
        case PrimitiveTag::U32: return Type::TYPE_U32;
        case PrimitiveTag::U64: return Type::TYPE_U64;
        case PrimitiveTag::USize: return Type::TYPE_USIZE;
        case PrimitiveTag::F32: return Type::TYPE_F32;
        case PrimitiveTag::F64: return Type::TYPE_F64;
        default: unreachable();
    }
}
