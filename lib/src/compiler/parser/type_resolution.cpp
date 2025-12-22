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

static Result<void, ProgramError> parseOptionalSymbol(ParsedType* parsedType) {
    ParsedTypeNode node;
    node.tag = ParsedTypeTag::Nullable;
    if (parsedType->nodes.push(std::move(node)).hasErr()) {
        return Error(ProgramError::OutOfMemory);
    }

    return {};
}

static Result<void, ProgramError> parsePointer(ParseInfo* parseInfo, ParsedType* parsedType) {
    ParsedTypeNode node;
    node.tag = ParsedTypeTag::Pointer;

    if (auto next = parseInfo->tokenIter.peek(); next.has_value()) {
        TokenType nextToken = next.value().tag();
        if (nextToken == TokenType::MutKeyword) {
            (void)parseInfo->tokenIter.next();
            node.isMutable = true;
        }
    }

    if (parsedType->nodes.push(std::move(node)).hasErr()) {
        return Error(ProgramError::OutOfMemory);
    }

    return {};
}

static Result<void, ProgramError> parseLifetimePointer(ParseInfo* parseInfo, ParsedType* parsedType) {
    ParsedTypeNode node;
    node.tag = ParsedTypeTag::Pointer;

    // next must be identifier for valid lifetime pointer
    if (auto next = parseInfo->tokenIter.peek(); next.has_value()) {
        TokenType nextToken = next.value().tag();
        if (nextToken != TokenType::Identifier) {
            parseInfo->reportErr(ProgramError::CompileUnknownType, parseInfo->tokenIter.current().location(),
                                 "Expected identifier for lifetime annotated pointer"); //
            return Error(ProgramError::CompileUnknownType);
        }

        (void)parseInfo->tokenIter.next();
        node.lifetime = parseInfo->tokenIter.currentSlice();
    }

    if (auto next = parseInfo->tokenIter.peek(); next.has_value()) {
        TokenType nextToken = next.value().tag();
        if (nextToken == TokenType::MutKeyword) {
            (void)parseInfo->tokenIter.next();
            node.isMutable = true;
        }
    }

    if (parsedType->nodes.push(std::move(node)).hasErr()) {
        return Error(ProgramError::OutOfMemory);
    }

    return {};
}

static Result<void, ProgramError> parseDyn(ParseInfo* parseInfo, ParsedType* parsedType) {
    ParsedTypeNode node;
    node.tag = ParsedTypeTag::Dyn;

    if (auto next = parseInfo->tokenIter.peek(); next.has_value()) {
        TokenType nextToken = next.value().tag();
        if (nextToken == TokenType::MutKeyword) {
            (void)parseInfo->tokenIter.next();
            node.isMutable = true;
        }
    }

    if (parsedType->nodes.push(std::move(node)).hasErr()) {
        return Error(ProgramError::OutOfMemory);
    }

    return {};
}

static Result<void, ProgramError> parseLifetimeDyn(ParseInfo* parseInfo, ParsedType* parsedType) {
    ParsedTypeNode node;
    node.tag = ParsedTypeTag::Dyn;

    // next must be identifier for valid lifetime pointer
    if (auto next = parseInfo->tokenIter.peek(); next.has_value()) {
        TokenType nextToken = next.value().tag();
        if (nextToken != TokenType::Identifier) {
            parseInfo->reportErr(ProgramError::CompileUnknownType, parseInfo->tokenIter.current().location(),
                                 "Expected identifier for lifetime annotated pointer"); //
            return Error(ProgramError::CompileUnknownType);
        }

        (void)parseInfo->tokenIter.next();
        node.lifetime = parseInfo->tokenIter.currentSlice();
    }

    if (auto next = parseInfo->tokenIter.peek(); next.has_value()) {
        TokenType nextToken = next.value().tag();
        if (nextToken == TokenType::MutKeyword) {
            (void)parseInfo->tokenIter.next();
            node.isMutable = true;
        }
    }

    if (parsedType->nodes.push(std::move(node)).hasErr()) {
        return Error(ProgramError::OutOfMemory);
    }

    return {};
}

static Result<void, ProgramError> parseUnique(ParsedType* parsedType) {
    ParsedTypeNode node;
    node.tag = ParsedTypeTag::Unique;
    if (parsedType->nodes.push(std::move(node)).hasErr()) {
        return Error(ProgramError::OutOfMemory);
    }

    return {};
}

static Result<void, ProgramError> parseShared(ParsedType* parsedType) {
    ParsedTypeNode node;
    node.tag = ParsedTypeTag::Shared;
    if (parsedType->nodes.push(std::move(node)).hasErr()) {
        return Error(ProgramError::OutOfMemory);
    }

    return {};
}

static Result<void, ProgramError> parseWeak(ParsedType* parsedType) {
    ParsedTypeNode node;
    node.tag = ParsedTypeTag::Weak;
    if (parsedType->nodes.push(std::move(node)).hasErr()) {
        return Error(ProgramError::OutOfMemory);
    }

    return {};
}

enum class TypeParsePhase : int32_t {
    /// Identifiers, primitives, or prefixes
    CollectPrefixOrGetBase,
    /// Next token MUST be an identifier or other named thing
    GetBaseOnly,
    /// Concrete lifetime or generic parameters
    CollectPostfix,

    DoneParse,
};

Result<ParsedType, ProgramError> sy::ParsedType::parse(ParseInfo* parseInfo) {
    ParsedType parsedType{DynArray<ParsedTypeNode>(parseInfo->alloc)};
    TypeParsePhase parsePhase = TypeParsePhase::CollectPrefixOrGetBase;

    while (parsePhase != TypeParsePhase::DoneParse) {
        // Try to parse named types (identifiers / primitives) explicitly
        if (parsePhase == TypeParsePhase::CollectPrefixOrGetBase || parsePhase == TypeParsePhase::GetBaseOnly) {
            TokenType currentToken = parseInfo->tokenIter.current().tag();
            switch (currentToken) {
            case TokenType::Identifier:
            case TokenType::BoolPrimitive:
            case TokenType::I8Primitive:
            case TokenType::I16Primitive:
            case TokenType::I32Primitive:
            case TokenType::I64Primitive:
            case TokenType::U8Primitive:
            case TokenType::U16Primitive:
            case TokenType::U32Primitive:
            case TokenType::U64Primitive:
            case TokenType::USizePrimitive:
            case TokenType::F32Primitive:
            case TokenType::F64Primitive:
            case TokenType::CharPrimitive:
            case TokenType::StrPrimitive:
            case TokenType::StringPrimitive:
            case TokenType::TypePrimitive:
            case TokenType::ListPrimitive:
            case TokenType::MapPrimitive:
            case TokenType::SetPrimitive: {
                ParsedTypeNode node;
                node.tag = ParsedTypeTag::Named;
                node.name = parseInfo->tokenIter.currentSlice();
                if (parsedType.nodes.push(std::move(node)).hasErr()) {
                    return Error(ProgramError::OutOfMemory);
                }
            } break;

            default:
                if (parsePhase == TypeParsePhase::GetBaseOnly) {
                    parseInfo->reportErr(ProgramError::CompileUnknownType, parseInfo->tokenIter.current().location(),
                                         "Expected identifier or primitive");
                    return Error(ProgramError::CompileUnknownType);
                }
            }
        }

        switch (parsePhase) {
        case TypeParsePhase::CollectPrefixOrGetBase: {
            TokenType currentToken = parseInfo->tokenIter.current().tag();
            switch (currentToken) {

            case TokenType::OptionalSymbol: {
                if (auto res = parseOptionalSymbol(&parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
            } break;

            case TokenType::AsteriskSymbol: {
                // TODO function pointer
                if (auto res = parsePointer(parseInfo, &parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
            } break;

            case TokenType::LifetimePointer: {
                if (auto res = parseLifetimePointer(parseInfo, &parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
            } break;

            case TokenType::DynKeyword: {
                // TODO constraint next to be identifier for trait no matter what
                if (auto res = parseDyn(parseInfo, &parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
            } break;

            case TokenType::LifetimeDynKeyword: {
                // TODO constraint next to be identifier for trait no matter what
                if (auto res = parseLifetimeDyn(parseInfo, &parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
            } break;

            case TokenType::UniqueKeyword: {
                if (auto res = parseUnique(&parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
            } break;

            case TokenType::SharedKeyword: {
                if (auto res = parseShared(&parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
            } break;

            case TokenType::WeakKeyword: {
                if (auto res = parseWeak(&parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
            } break;

            default:

                break;
            }
        }

        default:
            break;
        }
    }

    return parsedType;
}
