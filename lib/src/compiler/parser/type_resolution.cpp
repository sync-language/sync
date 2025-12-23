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

static Result<void, ProgramError> parseSlice(ParseInfo* parseInfo, ParsedType* parsedType) {
    ParsedTypeNode node;
    node.tag = ParsedTypeTag::Slice;

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

static Result<void, ProgramError> parseLifetimeSlice(ParseInfo* parseInfo, ParsedType* parsedType) {
    ParsedTypeNode node;
    node.tag = ParsedTypeTag::Slice;

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
                                 "Expected identifier for lifetime annotated pointer");
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

static Result<void, ProgramError> parseStaticArray(ParseInfo* parseInfo, ParsedType* parsedType) {
    ParsedTypeNode node;
    node.tag = ParsedTypeTag::StaticArray;

    if (parseInfo->tokenIter.next().has_value() == false) {
        parseInfo->reportErr(ProgramError::CompileUnknownType, parseInfo->tokenIter.current().location(),
                             "Expected expression for static array size, not end of file");
        return Error(ProgramError::CompileUnknownType);
    }

    // This gets added BEFORE the static array node, but is a child of the static array node to maintain correct
    // ordering.
    uint16_t sizeNodeIndex = UINT16_MAX;

    { // size of the array
        const TokenType current = parseInfo->tokenIter.current().tag();
        switch (current) {
        case TokenType::NumberLiteral: {
            ParsedTypeNode sizeNode;
            sizeNode.tag = ParsedTypeTag::IntLiteral;
            sizeNode.expression = parseInfo->tokenIter.currentSlice();
            if (parsedType->nodes.push(std::move(sizeNode)).hasErr()) {
                return Error(ProgramError::OutOfMemory);
            }
            sizeNodeIndex = static_cast<uint16_t>(parsedType->nodes.len() - 1);
        } break;

        default:
            parseInfo->reportErr(ProgramError::CompileUnknownType, parseInfo->tokenIter.current().location(),
                                 "Expected number literal for array size");
            return Error(ProgramError::CompileUnknownType);
        }
    }

    if (node.childrenIndices.push(sizeNodeIndex).hasErr()) {
        return Error(ProgramError::OutOfMemory);
    }

    if (parsedType->nodes.push(std::move(node)).hasErr()) {
        return Error(ProgramError::OutOfMemory);
    }

    { // need ']' character
        if (parseInfo->tokenIter.next().has_value() == false) {
            parseInfo->reportErr(ProgramError::CompileUnknownType, parseInfo->tokenIter.current().location(),
                                 "Expected ']' for static array type, not end of file");
            return Error(ProgramError::CompileUnknownType);
        }
        const TokenType current = parseInfo->tokenIter.current().tag();
        if (current != TokenType::RightBracketSymbol) {
            parseInfo->reportErr(ProgramError::CompileUnknownType, parseInfo->tokenIter.current().location(),
                                 "Expected ']' for static array type");
            return Error(ProgramError::CompileUnknownType);
        }
    }

    return {};
}

static Result<void, ProgramError> parseConcreteLifetime(ParseInfo* parseInfo, ParsedType* parsedType) {
    // next must be identifier for valid lifetime pointer
    if (parseInfo->tokenIter.next().has_value() == false) {
        parseInfo->reportErr(ProgramError::CompileUnknownType, parseInfo->tokenIter.current().location(),
                             "Expected identifier for concrete lifetime, not end of file");
        return Error(ProgramError::CompileUnknownType);
    }

    TokenType token = parseInfo->tokenIter.current().tag();
    if (token != TokenType::Identifier) {
        parseInfo->reportErr(ProgramError::CompileUnknownType, parseInfo->tokenIter.current().location(),
                             "Expected identifier for concrete lifetime");
        return Error(ProgramError::CompileUnknownType);
    }

    parsedType->nodes[parsedType->nodes.len() - 1].lifetime = parseInfo->tokenIter.currentSlice();

    return {};
}

static Result<void, ProgramError> parseErrorUnion(ParseInfo* parseInfo, ParsedType* parsedType) {
    { // validate not already found error union
        const ParsedTypeNode& rootNode = parsedType->nodes[parsedType->rootNode];
        if (rootNode.tag == ParsedTypeTag::ErrorUnion) {
            parseInfo->reportErr(ProgramError::CompileUnknownType, parseInfo->tokenIter.current().location(),
                                 "Cannot chain error unions");
            return Error(ProgramError::CompileUnknownType);
        }
    }

    ParsedTypeNode node;
    node.tag = ParsedTypeTag::ErrorUnion;
    if (node.childrenIndices.push(parsedType->rootNode).hasErr()) {
        return Error(ProgramError::OutOfMemory);
    }
    if (parsedType->nodes.push(std::move(node)).hasErr()) {
        return Error(ProgramError::OutOfMemory);
    }
    parsedType->rootNode = static_cast<uint16_t>(parsedType->nodes.len() - 1);

    return {};
}

enum class TypeParsePhase : int32_t {
    /// Identifiers, primitives, or prefixes
    CollectPrefixOrGetNamed,
    /// Next token MUST be an identifier or other named thing
    GetNamedOnly,
    /// Concrete lifetime or generic parameters
    CollectPostfix,

    DoneParse,
};

struct GenericContext {
    uint16_t parentNode;
    uint16_t currentArgRoot;
};

static constexpr uint16_t MAX_GENERIC_TYPE_DEPTH = 32;

Result<ParsedType, ProgramError> sy::ParsedType::parse(ParseInfo* parseInfo) {
    ParsedType parsedType{DynArray<ParsedTypeNode>(parseInfo->alloc)};
    TypeParsePhase parsePhase = TypeParsePhase::CollectPrefixOrGetNamed;

    GenericContext genericStack[MAX_GENERIC_TYPE_DEPTH];
    uint16_t genericStackDepth = 0;

    while (parsePhase != TypeParsePhase::DoneParse) {
        size_t nodesLenAtStart = parsedType.nodes.len();

        Option<uint16_t> prevNodeIndex{};
        if (parsedType.nodes.len() > 0) {
            prevNodeIndex = static_cast<uint16_t>(parsedType.nodes.len() - 1);
        }

        GenericContext* currentGenericContext = nullptr;
        if (genericStackDepth > 0) {
            currentGenericContext = &genericStack[genericStackDepth - 1];
        }

        // Try to parse named types (identifiers / primitives) explicitly
        if (parsePhase == TypeParsePhase::CollectPrefixOrGetNamed || parsePhase == TypeParsePhase::GetNamedOnly) {
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

                if (genericStackDepth > 0) {
                    if (currentGenericContext->currentArgRoot == UINT16_MAX) {
                        currentGenericContext->currentArgRoot = static_cast<uint16_t>(parsedType.nodes.len() - 1);
                    } else {
                        if (prevNodeIndex.hasValue()) {
                            ParsedTypeNode& prevNode = parsedType.nodes[prevNodeIndex.value()];
                            if (prevNode.childrenIndices.push(static_cast<uint16_t>(parsedType.nodes.len() - 1))
                                    .hasErr()) {
                                return Error(ProgramError::OutOfMemory);
                            }
                        }
                    }
                }

                // step iter
                if (parseInfo->tokenIter.next().has_value() == false) {
                    parsePhase = TypeParsePhase::DoneParse;
                } else {
                    parsePhase = TypeParsePhase::CollectPostfix;
                }
                // add the new node as a child
                uint16_t newNodeIndex = static_cast<uint16_t>(parsedType.nodes.len() - 1);
                if (genericStackDepth == 0) {
                    if (prevNodeIndex.hasValue()) {
                        ParsedTypeNode& prevNode = parsedType.nodes[prevNodeIndex.value()];
                        if (prevNode.childrenIndices.push(newNodeIndex).hasErr()) {
                            return Error(ProgramError::OutOfMemory);
                        }
                    } else {
                        parsedType.rootNode = newNodeIndex;
                    }
                    continue; // loop again
                }

            } break;

            default:
                if (parsePhase == TypeParsePhase::GetNamedOnly) {
                    parseInfo->reportErr(ProgramError::CompileUnknownType, parseInfo->tokenIter.current().location(),
                                         "Expected identifier or primitive");
                    return Error(ProgramError::CompileUnknownType);
                }
            }
        }

        switch (parsePhase) {
        case TypeParsePhase::CollectPrefixOrGetNamed: {
            bool shouldStepIter = true;
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

            case TokenType::Slice: {
                if (auto res = parseSlice(parseInfo, &parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
            } break;

            case TokenType::SliceLifetime: {
                if (auto res = parseLifetimeSlice(parseInfo, &parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
            } break;

            case TokenType::DynKeyword: {
                // TODO constraint next to be identifier for trait no matter what
                if (auto res = parseDyn(parseInfo, &parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
                parsePhase = TypeParsePhase::GetNamedOnly; // only a trait name can come after dyn
            } break;

            case TokenType::LifetimeDynKeyword: {
                // TODO constraint next to be identifier for trait no matter what
                if (auto res = parseLifetimeDyn(parseInfo, &parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
                parsePhase = TypeParsePhase::GetNamedOnly; // only a trait name can come after dyn
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

            case TokenType::LeftBracketSymbol: {
                if (auto res = parseStaticArray(parseInfo, &parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
            } break;

            case TokenType::LeftParenthesesSymbol: {
                ParsedTypeNode node;
                node.tag = ParsedTypeTag::Tuple;
                if (parsedType.nodes.push(std::move(node)).hasErr()) {
                    return Error(ProgramError::OutOfMemory);
                }
                shouldStepIter = false;
                parsePhase = TypeParsePhase::CollectPostfix;
            } break;

            default:

                break;
            }

            if (shouldStepIter && parseInfo->tokenIter.next().has_value() == false) {
                parsePhase = TypeParsePhase::DoneParse;
            }
        } break;

        case TypeParsePhase::CollectPostfix: {
            TokenType currentToken = parseInfo->tokenIter.current().tag();
            switch (currentToken) {

            case TokenType::ConcreteLifetime: {
                if (auto res = parseConcreteLifetime(parseInfo, &parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
            } break;

            case TokenType::ExclamationSymbol: {
                if (auto res = parseErrorUnion(parseInfo, &parsedType); res.hasErr()) {
                    return Error(res.takeErr());
                }
                parsePhase = TypeParsePhase::CollectPrefixOrGetNamed;
            } break;

            case TokenType::LeftParenthesesSymbol: { // generic
                if (genericStackDepth >= MAX_GENERIC_TYPE_DEPTH) {
                    parseInfo->reportErr(ProgramError::CompileUnknownType, parseInfo->tokenIter.current().location(),
                                         "Generic nesting depth exceeds maximum (maybe malicious code)");
                    return Error(ProgramError::CompileUnknownType);
                }

                uint16_t namedNodeIndex = static_cast<uint16_t>(parsedType.nodes.len() - 1);
                GenericContext newContext{namedNodeIndex, UINT16_MAX};
                genericStack[genericStackDepth] = newContext;
                currentGenericContext = &genericStack[genericStackDepth];
                genericStackDepth += 1;

                if (parseInfo->tokenIter.next().has_value() == false) {
                    parseInfo->reportErr(ProgramError::CompileUnknownType, parseInfo->tokenIter.current().location(),
                                         "Expected generic argument or ')', not EOF");
                    return Error(ProgramError::CompileUnknownType);
                }

                TokenType token = parseInfo->tokenIter.current().tag();

                switch (token) {
                case TokenType::RightParenthesesSymbol: { // empty, just ()
                    genericStackDepth -= 1;
                    if (parseInfo->tokenIter.next().has_value() == false) {
                        parsePhase = TypeParsePhase::DoneParse;
                    } else {
                        parsePhase = TypeParsePhase::CollectPostfix;
                    }
                } break;

                case TokenType::NumberLiteral: {
                    if (parsedType.nodes[currentGenericContext->parentNode].tag == ParsedTypeTag::Tuple) {
                        parseInfo->reportErr(ProgramError::CompileUnknownType,
                                             parseInfo->tokenIter.current().location(),
                                             "Tuple may not contain number literals");
                        return Error(ProgramError::CompileUnknownType);
                    }

                    ParsedTypeNode argNode;
                    argNode.tag = ParsedTypeTag::IntLiteral;
                    argNode.expression = parseInfo->tokenIter.currentSlice();
                    uint16_t argIndex = static_cast<uint16_t>(parsedType.nodes.len());
                    if (parsedType.nodes.push(std::move(argNode)).hasErr()) {
                        return Error(ProgramError::OutOfMemory);
                    }
                    if (parsedType.nodes[namedNodeIndex].childrenIndices.push(argIndex).hasErr()) {
                        return Error(ProgramError::OutOfMemory);
                    }

                    if (parseInfo->tokenIter.next().has_value() == false) {
                        parseInfo->reportErr(ProgramError::CompileUnknownType,
                                             parseInfo->tokenIter.current().location(),
                                             "Expected ',' or ')' after generic argument, not end of file");
                        return Error(ProgramError::CompileUnknownType);
                    }
                    continue;
                } break;

                default:
                    parsePhase = TypeParsePhase::CollectPrefixOrGetNamed;
                }
            } break;

            default: {
                if (genericStackDepth > 0) {
                    TokenType token = parseInfo->tokenIter.current().tag();
                    if (token == TokenType::CommaSymbol || token == TokenType::RightParenthesesSymbol) {
                        // done generic arg
                        if (currentGenericContext->currentArgRoot != UINT16_MAX) {
                            if (parsedType.nodes[currentGenericContext->parentNode]
                                    .childrenIndices.push(currentGenericContext->currentArgRoot)
                                    .hasErr()) {
                                return Error(ProgramError::OutOfMemory);
                            }
                        }

                        if (token == TokenType::CommaSymbol) {
                            if (parseInfo->tokenIter.next().has_value() == false) {
                                parseInfo->reportErr(ProgramError::CompileUnknownType,
                                                     parseInfo->tokenIter.current().location(),
                                                     "Expected generic argument after ',', not end of file");
                                return Error(ProgramError::CompileUnknownType);
                            }

                            TokenType nextToken = parseInfo->tokenIter.current().tag();
                            currentGenericContext->currentArgRoot = UINT16_MAX; // reset

                            if (nextToken == TokenType::NumberLiteral) {
                                ParsedTypeNode argNode;
                                argNode.tag = ParsedTypeTag::IntLiteral;
                                argNode.expression = parseInfo->tokenIter.currentSlice();
                                uint16_t argIndex = static_cast<uint16_t>(parsedType.nodes.len());
                                if (parsedType.nodes.push(std::move(argNode)).hasErr()) {
                                    return Error(ProgramError::OutOfMemory);
                                }
                                if (parsedType.nodes[currentGenericContext->parentNode]
                                        .childrenIndices.push(argIndex)
                                        .hasErr()) {
                                    return Error(ProgramError::OutOfMemory);
                                }

                                if (parseInfo->tokenIter.next().has_value() == false) {
                                    parseInfo->reportErr(ProgramError::CompileUnknownType,
                                                         parseInfo->tokenIter.current().location(),
                                                         "Expected ',' or ')' after generic argument, not end of file");
                                    return Error(ProgramError::CompileUnknownType);
                                }
                            } else {
                                parsePhase = TypeParsePhase::CollectPrefixOrGetNamed;
                            }
                        } else { // right parenthesis
                            genericStackDepth -= 1;
                            if (parseInfo->tokenIter.next().has_value() == false) {
                                parsePhase = TypeParsePhase::DoneParse;
                            } else {
                                parsePhase = TypeParsePhase::CollectPostfix;
                            }
                        }
                        continue;
                    }
                } else {
                    parsePhase = TypeParsePhase::DoneParse;
                }

                // parseInfo->reportErr(ProgramError::CompileUnknownType, parseInfo->tokenIter.current().location(),
                //                      "Expected concrete lifetime, error union, or generic args");
                // return Error(ProgramError::CompileUnknownType);
            }
            }

        } break;

        default:
            break;
        }

        // add the new node as a child only if a node was created this iteration
        if (parsedType.nodes.len() > nodesLenAtStart) {
            uint16_t newNodeIndex = static_cast<uint16_t>(parsedType.nodes.len() - 1);
            if (genericStackDepth > 0 && currentGenericContext->currentArgRoot == UINT16_MAX) {
                // set generic root
                currentGenericContext->currentArgRoot = newNodeIndex;
            } else if (genericStackDepth == 0) {
                if (prevNodeIndex.hasValue()) {
                    ParsedTypeNode& prevNode = parsedType.nodes[prevNodeIndex.value()];
                    if (prevNode.childrenIndices.push(newNodeIndex).hasErr()) {
                        return Error(ProgramError::OutOfMemory);
                    }
                } else {
                    parsedType.rootNode = newNodeIndex;
                }
            } else { // is generic
                if (prevNodeIndex.hasValue()) {
                    ParsedTypeNode& prevNode = parsedType.nodes[prevNodeIndex.value()];
                    if (prevNode.childrenIndices.push(newNodeIndex).hasErr()) {
                        return Error(ProgramError::OutOfMemory);
                    }
                }
            }
        }
    }
    return parsedType;
}

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"

TEST_CASE("[ParsedType::parse] Parse normal i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    CHECK_EQ(type.getRootNode().tag, ParsedTypeTag::Named);
    CHECK_EQ(type.getRootNode().name, "i32");
    CHECK_EQ(type.getRootNode().lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse nullable i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "?i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Nullable);

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "i32");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse pointer i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "*i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Pointer);
    CHECK_FALSE(root.isMutable);

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "i32");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse mut pointer i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "*mut i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Pointer);
    CHECK(root.isMutable);

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "i32");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse lifetime pointer i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "*'a i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Pointer);
    CHECK_FALSE(root.isMutable);
    CHECK_EQ(root.lifetime, "a");

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "i32");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse mut lifetime pointer i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "*'a mut i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Pointer);
    CHECK(root.isMutable);
    CHECK_EQ(root.lifetime, "a");

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "i32");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse slice i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "[]i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Slice);
    CHECK_FALSE(root.isMutable);

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "i32");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse mut slice i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "[]mut i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Slice);
    CHECK(root.isMutable);

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "i32");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse lifetime slice i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "[]'a i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Slice);
    CHECK_FALSE(root.isMutable);
    CHECK_EQ(root.lifetime, "a");

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "i32");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse mut lifetime slice i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "[]'a mut i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Slice);
    CHECK(root.isMutable);
    CHECK_EQ(root.lifetime, "a");

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "i32");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse dyn trait") {
    Tokenizer tokenizer = Tokenizer::create({}, "dyn Example").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Dyn);
    CHECK_FALSE(root.isMutable);

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "Example");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse mut dyn trait") {
    Tokenizer tokenizer = Tokenizer::create({}, "dyn mut Example").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Dyn);
    CHECK(root.isMutable);

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "Example");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse lifetime dyn trait") {
    Tokenizer tokenizer = Tokenizer::create({}, "dyn'a Example").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Dyn);
    CHECK_FALSE(root.isMutable);
    CHECK_EQ(root.lifetime, "a");

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "Example");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse mut lifetime dyn trait") {
    Tokenizer tokenizer = Tokenizer::create({}, "dyn'a mut Example").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Dyn);
    CHECK(root.isMutable);
    CHECK_EQ(root.lifetime, "a");

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "Example");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse unique i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "Unique i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Unique);
    CHECK_FALSE(root.isMutable);

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "i32");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse shared i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "Shared i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Shared);
    CHECK_FALSE(root.isMutable);

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "i32");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse weak i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "Weak i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Weak);
    CHECK_FALSE(root.isMutable);

    const ParsedTypeNode& child = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(child.tag, ParsedTypeTag::Named);
    CHECK_EQ(child.name, "i32");
    CHECK_EQ(child.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse static array int literal i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "[8]i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::StaticArray);
    CHECK_FALSE(root.isMutable);

    const ParsedTypeNode& sizeChild = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(sizeChild.tag, ParsedTypeTag::IntLiteral);
    CHECK_EQ(sizeChild.expression, "8");

    const ParsedTypeNode& typeChild = type.nodes[root.childrenIndices[1]];
    CHECK_EQ(typeChild.tag, ParsedTypeTag::Named);
    CHECK_EQ(typeChild.name, "i32");
    CHECK_EQ(typeChild.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse nullable pointer i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "?*i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Nullable);

    const ParsedTypeNode& pointerChild = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(pointerChild.tag, ParsedTypeTag::Pointer);
    CHECK_FALSE(pointerChild.isMutable);
    CHECK_EQ(pointerChild.lifetime, ""); // no lifetime

    const ParsedTypeNode& typeChild = type.nodes[pointerChild.childrenIndices[0]];
    CHECK_EQ(typeChild.tag, ParsedTypeTag::Named);
    CHECK_EQ(typeChild.name, "i32");
    CHECK_EQ(typeChild.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse nullable mut lifetime pointer i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "?*'a mut i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Nullable);

    const ParsedTypeNode& pointerChild = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(pointerChild.tag, ParsedTypeTag::Pointer);
    CHECK(pointerChild.isMutable);
    CHECK_EQ(pointerChild.lifetime, "a"); // no lifetime

    const ParsedTypeNode& typeChild = type.nodes[pointerChild.childrenIndices[0]];
    CHECK_EQ(typeChild.tag, ParsedTypeTag::Named);
    CHECK_EQ(typeChild.name, "i32");
    CHECK_EQ(typeChild.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse error union err i32 ok i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "i32!i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::ErrorUnion);

    const ParsedTypeNode& errTypeChild = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(errTypeChild.tag, ParsedTypeTag::Named);
    CHECK_EQ(errTypeChild.name, "i32");
    CHECK_EQ(errTypeChild.lifetime, ""); // no lifetime

    const ParsedTypeNode& okTypeChild = type.nodes[root.childrenIndices[1]];
    CHECK_EQ(okTypeChild.tag, ParsedTypeTag::Named);
    CHECK_EQ(okTypeChild.name, "i32");
    CHECK_EQ(okTypeChild.lifetime, ""); // no lifetime
}

TEST_CASE("[ParsedType::parse] Parse error union mix of type prefixes") {
    Tokenizer tokenizer = Tokenizer::create({}, "?*i32![]'a mut i32").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::ErrorUnion);

    {
        const ParsedTypeNode& errNullableChild = type.nodes[root.childrenIndices[0]];
        CHECK_EQ(errNullableChild.tag, ParsedTypeTag::Nullable);

        const ParsedTypeNode& errPointerChild = type.nodes[errNullableChild.childrenIndices[0]];
        CHECK_EQ(errPointerChild.tag, ParsedTypeTag::Pointer);
        CHECK_FALSE(errPointerChild.isMutable);
        CHECK_EQ(errPointerChild.lifetime, "");

        const ParsedTypeNode& errTypeChild = type.nodes[errPointerChild.childrenIndices[0]];
        CHECK_EQ(errTypeChild.tag, ParsedTypeTag::Named);
        CHECK_EQ(errTypeChild.name, "i32");
        CHECK_EQ(errTypeChild.lifetime, ""); // no lifetime
    }

    {
        const ParsedTypeNode& okSliceChild = type.nodes[root.childrenIndices[1]];
        CHECK_EQ(okSliceChild.tag, ParsedTypeTag::Slice);
        CHECK(okSliceChild.isMutable);
        CHECK_EQ(okSliceChild.lifetime, "a");

        const ParsedTypeNode& okTypeChild = type.nodes[okSliceChild.childrenIndices[0]];
        CHECK_EQ(okTypeChild.tag, ParsedTypeTag::Named);
        CHECK_EQ(okTypeChild.name, "i32");
        CHECK_EQ(okTypeChild.lifetime, ""); // no lifetime
    }
}

TEST_CASE("[ParsedType::parse] Parse concrete lifetime i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "i32@'a").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    CHECK_EQ(type.getRootNode().tag, ParsedTypeTag::Named);
    CHECK_EQ(type.getRootNode().name, "i32");
    CHECK_EQ(type.getRootNode().lifetime, "a");
}

TEST_CASE("[ParsedType::parse] Parse List generic i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "List(i32)").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Named);
    CHECK_EQ(root.name, "List");
    CHECK_EQ(root.childrenIndices.len(), 1);

    const ParsedTypeNode& genericType = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(genericType.tag, ParsedTypeTag::Named);
    CHECK_EQ(genericType.name, "i32");
    CHECK_EQ(genericType.lifetime, "");
    CHECK_EQ(genericType.childrenIndices.len(), 0);
}

TEST_CASE("[ParsedType::parse] Parse Set generic i32") {
    Tokenizer tokenizer = Tokenizer::create({}, "Set(i32)").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Named);
    CHECK_EQ(root.name, "Set");
    CHECK_EQ(root.childrenIndices.len(), 1);

    const ParsedTypeNode& genericType = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(genericType.tag, ParsedTypeTag::Named);
    CHECK_EQ(genericType.name, "i32");
    CHECK_EQ(genericType.lifetime, "");
    CHECK_EQ(genericType.childrenIndices.len(), 0);
}

TEST_CASE("[ParsedType::parse] Parse Map generic i32, f32") {
    Tokenizer tokenizer = Tokenizer::create({}, "Map(i32, f32)").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Named);
    CHECK_EQ(root.name, "Map");
    CHECK_EQ(root.childrenIndices.len(), 2);

    const ParsedTypeNode& firstGeneric = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(firstGeneric.tag, ParsedTypeTag::Named);
    CHECK_EQ(firstGeneric.name, "i32");
    CHECK_EQ(firstGeneric.lifetime, "");
    CHECK_EQ(firstGeneric.childrenIndices.len(), 0);

    const ParsedTypeNode& secondGeneric = type.nodes[root.childrenIndices[1]];
    CHECK_EQ(secondGeneric.tag, ParsedTypeTag::Named);
    CHECK_EQ(secondGeneric.name, "f32");
    CHECK_EQ(secondGeneric.lifetime, "");
    CHECK_EQ(secondGeneric.childrenIndices.len(), 0);
}

TEST_CASE("[ParsedType::parse] Parse generic 3 args u8, i32, f32") {
    Tokenizer tokenizer = Tokenizer::create({}, "Example(u8, i32, f32)").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Named);
    CHECK_EQ(root.name, "Example");
    CHECK_EQ(root.childrenIndices.len(), 3);

    const ParsedTypeNode& firstGeneric = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(firstGeneric.tag, ParsedTypeTag::Named);
    CHECK_EQ(firstGeneric.name, "u8");
    CHECK_EQ(firstGeneric.lifetime, "");
    CHECK_EQ(firstGeneric.childrenIndices.len(), 0);

    const ParsedTypeNode& secondGeneric = type.nodes[root.childrenIndices[1]];
    CHECK_EQ(secondGeneric.tag, ParsedTypeTag::Named);
    CHECK_EQ(secondGeneric.name, "i32");
    CHECK_EQ(secondGeneric.lifetime, "");
    CHECK_EQ(secondGeneric.childrenIndices.len(), 0);

    const ParsedTypeNode& thirdGeneric = type.nodes[root.childrenIndices[2]];
    CHECK_EQ(thirdGeneric.tag, ParsedTypeTag::Named);
    CHECK_EQ(thirdGeneric.name, "f32");
    CHECK_EQ(thirdGeneric.lifetime, "");
    CHECK_EQ(thirdGeneric.childrenIndices.len(), 0);
}

TEST_CASE("[ParsedType::parse] Parse generic int literal arg") {
    Tokenizer tokenizer = Tokenizer::create({}, "Example(15)").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Named);
    CHECK_EQ(root.name, "Example");
    CHECK_EQ(root.childrenIndices.len(), 1);

    const ParsedTypeNode& firstGeneric = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(firstGeneric.tag, ParsedTypeTag::IntLiteral);
    CHECK_EQ(firstGeneric.expression, "15");
    CHECK_EQ(firstGeneric.lifetime, "");
    CHECK_EQ(firstGeneric.childrenIndices.len(), 0);
}

TEST_CASE("[ParsedType::parse] Parse Vec 3, f32") {
    Tokenizer tokenizer = Tokenizer::create({}, "Vec(3, f32)").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Named);
    CHECK_EQ(root.name, "Vec");
    CHECK_EQ(root.childrenIndices.len(), 2);

    const ParsedTypeNode& firstGeneric = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(firstGeneric.tag, ParsedTypeTag::IntLiteral);
    CHECK_EQ(firstGeneric.expression, "3");
    CHECK_EQ(firstGeneric.childrenIndices.len(), 0);

    const ParsedTypeNode& secondGeneric = type.nodes[root.childrenIndices[1]];
    CHECK_EQ(secondGeneric.tag, ParsedTypeTag::Named);
    CHECK_EQ(secondGeneric.name, "f32");
    CHECK_EQ(secondGeneric.lifetime, "");
    CHECK_EQ(secondGeneric.childrenIndices.len(), 0);
}

TEST_CASE("[ParsedType::parse] Parse Mat 4, 3, f32") {
    Tokenizer tokenizer = Tokenizer::create({}, "Mat(4, 3, f32)").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Named);
    CHECK_EQ(root.name, "Mat");
    CHECK_EQ(root.childrenIndices.len(), 3);

    const ParsedTypeNode& firstGeneric = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(firstGeneric.tag, ParsedTypeTag::IntLiteral);
    CHECK_EQ(firstGeneric.expression, "4");
    CHECK_EQ(firstGeneric.childrenIndices.len(), 0);

    const ParsedTypeNode& secondGeneric = type.nodes[root.childrenIndices[1]];
    CHECK_EQ(secondGeneric.tag, ParsedTypeTag::IntLiteral);
    CHECK_EQ(secondGeneric.expression, "3");
    CHECK_EQ(secondGeneric.childrenIndices.len(), 0);

    const ParsedTypeNode& thirdGeneric = type.nodes[root.childrenIndices[2]];
    CHECK_EQ(thirdGeneric.tag, ParsedTypeTag::Named);
    CHECK_EQ(thirdGeneric.name, "f32");
    CHECK_EQ(thirdGeneric.lifetime, "");
    CHECK_EQ(thirdGeneric.childrenIndices.len(), 0);
}

TEST_CASE("[ParsedType::parse] Parse generic many int literal") {
    Tokenizer tokenizer = Tokenizer::create({}, "Example(0, 1, 2, 3, 4)").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Named);
    CHECK_EQ(root.name, "Example");
    CHECK_EQ(root.childrenIndices.len(), 5);

    for (int i = 0; i < 5; i++) {
        const ParsedTypeNode& generic = type.nodes[root.childrenIndices[i]];
        CHECK_EQ(generic.tag, ParsedTypeTag::IntLiteral);
        char buf[2] = {static_cast<char>(i + static_cast<int>('0')), '\0'};
        CHECK_EQ(generic.expression, StringSlice(buf));
        CHECK_EQ(generic.childrenIndices.len(), 0);
    }
}

TEST_CASE("[ParsedType::parse] Parse tuple one type") {
    Tokenizer tokenizer = Tokenizer::create({}, "(i32)").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Tuple);
    CHECK_EQ(root.childrenIndices.len(), 1);

    const ParsedTypeNode& firstGeneric = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(firstGeneric.tag, ParsedTypeTag::Named);
    CHECK_EQ(firstGeneric.name, "i32");
    CHECK_EQ(firstGeneric.lifetime, "");
    CHECK_EQ(firstGeneric.childrenIndices.len(), 0);
}

TEST_CASE("[ParsedType::parse] Parse tuple one type") {
    Tokenizer tokenizer = Tokenizer::create({}, "(i32)").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Tuple);
    CHECK_EQ(root.childrenIndices.len(), 1);

    const ParsedTypeNode& firstGeneric = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(firstGeneric.tag, ParsedTypeTag::Named);
    CHECK_EQ(firstGeneric.name, "i32");
    CHECK_EQ(firstGeneric.lifetime, "");
    CHECK_EQ(firstGeneric.childrenIndices.len(), 0);
}

TEST_CASE("[ParsedType::parse] Parse tuple two types") {
    Tokenizer tokenizer = Tokenizer::create({}, "(f32, i32)").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Tuple);
    CHECK_EQ(root.childrenIndices.len(), 2);

    const ParsedTypeNode& firstGeneric = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(firstGeneric.tag, ParsedTypeTag::Named);
    CHECK_EQ(firstGeneric.name, "f32");
    CHECK_EQ(firstGeneric.lifetime, "");
    CHECK_EQ(firstGeneric.childrenIndices.len(), 0);

    const ParsedTypeNode& secondGeneric = type.nodes[root.childrenIndices[1]];
    CHECK_EQ(secondGeneric.tag, ParsedTypeTag::Named);
    CHECK_EQ(secondGeneric.name, "i32");
    CHECK_EQ(secondGeneric.lifetime, "");
    CHECK_EQ(secondGeneric.childrenIndices.len(), 0);
}

TEST_CASE("[ParsedType::parse] Parse tuple many types") {
    Tokenizer tokenizer = Tokenizer::create({}, "(u8, f32, i32)").takeValue();
    ParseInfo parseInfo(tokenizer.iter(), {}, "", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ParsedType type = ParsedType::parse(&parseInfo).takeValue();
    const ParsedTypeNode& root = type.getRootNode();
    CHECK_EQ(root.tag, ParsedTypeTag::Tuple);
    CHECK_EQ(root.childrenIndices.len(), 3);

    const ParsedTypeNode& firstGeneric = type.nodes[root.childrenIndices[0]];
    CHECK_EQ(firstGeneric.tag, ParsedTypeTag::Named);
    CHECK_EQ(firstGeneric.name, "u8");
    CHECK_EQ(firstGeneric.lifetime, "");
    CHECK_EQ(firstGeneric.childrenIndices.len(), 0);

    const ParsedTypeNode& secondGeneric = type.nodes[root.childrenIndices[1]];
    CHECK_EQ(secondGeneric.tag, ParsedTypeTag::Named);
    CHECK_EQ(secondGeneric.name, "f32");
    CHECK_EQ(secondGeneric.lifetime, "");
    CHECK_EQ(secondGeneric.childrenIndices.len(), 0);

    const ParsedTypeNode& thirdGeneric = type.nodes[root.childrenIndices[2]];
    CHECK_EQ(thirdGeneric.tag, ParsedTypeTag::Named);
    CHECK_EQ(thirdGeneric.name, "i32");
    CHECK_EQ(thirdGeneric.lifetime, "");
    CHECK_EQ(thirdGeneric.childrenIndices.len(), 0);
}

#endif // SYNC_LIB_WITH_TESTS