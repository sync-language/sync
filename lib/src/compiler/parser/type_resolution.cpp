#include "type_resolution.hpp"
#include "../../types/type_info.hpp"
#include "parser.hpp"

using namespace sy;

static Option<TypeResolutionInfo> tryParseNormalType(const TokenIter* tokenIter) noexcept {
    const Token current = tokenIter->current();
    switch (current.tag()) {
    case TokenType::BoolPrimitive: {
        return TypeResolutionInfo{Type::TYPE_BOOL->name, Type::TYPE_BOOL};
    } break;
    case TokenType::I8Primitive: {
        return TypeResolutionInfo{Type::TYPE_I8->name, Type::TYPE_I8};
    } break;
    case TokenType::I16Primitive: {
        return TypeResolutionInfo{Type::TYPE_I16->name, Type::TYPE_I16};
    } break;
    case TokenType::I32Primitive: {
        return TypeResolutionInfo{Type::TYPE_I32->name, Type::TYPE_I32};
    } break;
    case TokenType::I64Primitive: {
        return TypeResolutionInfo{Type::TYPE_I64->name, Type::TYPE_I64};
    } break;
    case TokenType::U8Primitive: {
        return TypeResolutionInfo{Type::TYPE_U8->name, Type::TYPE_U8};
    } break;
    case TokenType::U16Primitive: {
        return TypeResolutionInfo{Type::TYPE_U16->name, Type::TYPE_U16};
    } break;
    case TokenType::U32Primitive: {
        return TypeResolutionInfo{Type::TYPE_U32->name, Type::TYPE_U32};
    } break;
    case TokenType::U64Primitive: {
        return TypeResolutionInfo{Type::TYPE_U64->name, Type::TYPE_U64};
    } break;
    case TokenType::USizePrimitive: {
        return TypeResolutionInfo{Type::TYPE_USIZE->name, Type::TYPE_USIZE};
    } break;
    case TokenType::F32Primitive: {
        return TypeResolutionInfo{Type::TYPE_F32->name, Type::TYPE_F32};
    } break;
    case TokenType::F64Primitive: {
        return TypeResolutionInfo{Type::TYPE_F64->name, Type::TYPE_F64};
    } break;
    // TODO char and str? str is reference type though?
    case TokenType::StringPrimitive: {
        return TypeResolutionInfo{Type::TYPE_STRING->name, Type::TYPE_STRING};
    } break;
    default: {
        return {};
    }
    }
}

Result<TypeResolutionInfo, TypeResolutionInfo::Err> sy::TypeResolutionInfo::parse(ParseInfo* parseInfo) noexcept {
    if (auto normalParse = tryParseNormalType(&parseInfo->tokenIter); normalParse.hasValue()) {
        return normalParse.take();
    }

    return Error(TypeResolutionInfo::Err::NotAType);
}
