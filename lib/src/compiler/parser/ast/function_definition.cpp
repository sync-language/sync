#include "function_definition.hpp"
#include "../../../types/type_info.hpp"
#include "../../../util/assert.hpp"
#include "../parser.hpp"

using namespace sy;

static Result<DynArray<StackVariable>, ProgramError> parseFunctionArgs(ParseInfo* parseInfo) {
    sy_assert(parseInfo->tokenIter.current().tag() == TokenType::LeftParenthesesSymbol, "Expected left parentheses");

    DynArray<StackVariable> args(parseInfo->alloc);
    const StringSlice source = parseInfo->tokenIter.source();
    auto makeEndOfFileError = [source]() -> Error<ProgramError> {
        return Error(ProgramError(SourceFileLocation(source, static_cast<uint32_t>(source.len() - 1)),
                                  ProgramError::Kind::CompileFunctionSignature));
    };

    auto initialNext = parseInfo->tokenIter.next();
    if (initialNext.has_value() == false) {
        return makeEndOfFileError();
    }
    Token token = initialNext.value();

    while (token.tag() != TokenType::RightParenthesesSymbol) {
        StackVariable variable{};
        variable.isTemporary = false;

        if (token.tag() == TokenType::MutKeyword) {
            variable.isMutable = true;
            auto opt = parseInfo->tokenIter.next();
            if (opt.has_value() == false) {
                return makeEndOfFileError();
            }
            token = opt.value();
        }

        if (token.tag() != TokenType::Identifier) {
            return Error(ProgramError(SourceFileLocation(source, token.location()),
                                      ProgramError::Kind::CompileFunctionSignature));
        }

        StringSlice identifier = parseInfo->tokenIter.currentSlice();

        { // colon
            auto opt = parseInfo->tokenIter.next();
            if (opt.has_value() == false) {
                return makeEndOfFileError();
            }
            token = opt.value();
            if (token.tag() != TokenType::ColonSymbol) {
                return Error(ProgramError(SourceFileLocation(source, token.location()),
                                          ProgramError::Kind::CompileFunctionSignature));
            }
        }

        { // type
            auto opt = parseInfo->tokenIter.next();
            if (opt.has_value() == false) {
                return makeEndOfFileError();
            }
            token = opt.value();
            auto typeParseRes = TypeResolutionInfo::parse(parseInfo);
            if (typeParseRes.hasErr()) {
                return Error(ProgramError(SourceFileLocation(source, token.location()),
                                          ProgramError::Kind::CompileFunctionSignature));
            }
            variable.typeInfo = typeParseRes.takeValue();
        }

        { // actually make string from identifier
            auto strResult = String::copyConstructSlice(identifier, parseInfo->alloc);
            if (!strResult) {
                return Error(ProgramError(
                    SourceFileLocation(parseInfo->tokenIter.source(), parseInfo->tokenIter.current().location()),
                    ProgramError::Kind::OutOfMemory));
            }
            new (&variable.name) String(strResult.takeValue());
        }

        // push into array
        if (args.push(std::move(variable)).hasErr()) {
            return Error(ProgramError(
                SourceFileLocation(parseInfo->tokenIter.source(), parseInfo->tokenIter.current().location()),
                ProgramError::Kind::OutOfMemory));
        }

        { // move iterator forward to next argument
            auto opt = parseInfo->tokenIter.next();
            if (opt.has_value() == false) {
                return makeEndOfFileError();
            }
            token = opt.value();
            if (token.tag() != TokenType::CommaSymbol && token.tag() != TokenType::RightParenthesesSymbol) {
                return Error(ProgramError(SourceFileLocation(source, token.location()),
                                          ProgramError::Kind::CompileFunctionSignature));
            }
            if (token.tag() == TokenType::CommaSymbol) { // allow trailing commas
                auto commaStep = parseInfo->tokenIter.next();
                if (commaStep.has_value() == false) {
                    return makeEndOfFileError();
                }
                token = commaStep.value();
            }
        }
    }

    return args;
}

sy::FunctionDefinitionNode::~FunctionDefinitionNode() noexcept {
    for (size_t i = 0; i < this->statements.len(); i++) {
        delete statements[i];
    }
}

Result<void, ProgramError> sy::FunctionDefinitionNode::init(ParseInfo* parseInfo, Scope* outerScope) noexcept {
    sy_assert(parseInfo->tokenIter.current().tag() == TokenType::FnKeyword,
              "Function definition node must be initialized from a fn keyword token");

    const StringSlice source = parseInfo->tokenIter.source();
    auto makeEndOfFileError = [source]() -> Error<ProgramError> {
        return Error(ProgramError(SourceFileLocation(source, static_cast<uint32_t>(source.len() - 1)),
                                  ProgramError::Kind::CompileFunctionSignature));
    };

    { // function name
        auto result = parseInfo->tokenIter.next();
        if (result.has_value() == false) {
            return makeEndOfFileError();
        }
        const Token next = result.value();
        if (next.tag() != TokenType::Identifier) {
            return Error(ProgramError(SourceFileLocation(source, next.location()),
                                      ProgramError::Kind::CompileFunctionSignature));
        }

        this->functionName = parseInfo->tokenIter.currentSlice();
    }

    { // arguments
        auto nextResult = parseInfo->tokenIter.next();
        if (nextResult.has_value() == false) {
            return makeEndOfFileError();
        }
        const Token next = nextResult.value();
        if (next.tag() != TokenType::LeftParenthesesSymbol) {
            return Error(ProgramError(SourceFileLocation(source, next.location()),
                                      ProgramError::Kind::CompileFunctionSignature));
        }

        auto parseResult = parseFunctionArgs(parseInfo);
        if (parseResult.hasErr()) {
            return Error(parseResult.takeErr());
        }
        new (&this->args) DynArray<StackVariable>(parseResult.takeValue());
        sy_assert(parseInfo->tokenIter.current().tag() == TokenType::RightParenthesesSymbol,
                  "Expected to end with right parentheses");
    }

    { // return type
        auto nextResult = parseInfo->tokenIter.next();
        if (nextResult.has_value() == false) {
            return makeEndOfFileError();
        }
        const Token token = nextResult.value();
        if (token.tag() == TokenType::RightBraceSymbol) {
            this->retType = Option<TypeResolutionInfo>();
        } else {
            auto typeParseRes = TypeResolutionInfo::parse(parseInfo);
            if (typeParseRes.hasErr()) {
                return Error(ProgramError(SourceFileLocation(source, parseInfo->tokenIter.current().location()),
                                          ProgramError::Kind::CompileFunctionSignature));
            }
            this->retType = Option<TypeResolutionInfo>(typeParseRes.takeValue());
            // step forward
            if (parseInfo->tokenIter.next().has_value() == false) {
                return makeEndOfFileError();
            }
        }
    }

    { // function body
        if (parseInfo->tokenIter.current().tag() != TokenType::RightBraceSymbol) {
            return Error(ProgramError(SourceFileLocation(source, parseInfo->tokenIter.current().location()),
                                      ProgramError::Kind::CompileFunctionSignature));
        }
    }

    (void)outerScope;

    return {};
}

#if SYNC_LIB_WITH_TESTS

#include "../../../doctest.h"

TEST_CASE("parse function args") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "(arg1: i8, mut arg2: u64)").takeValue();
    ParseInfo parseInfo{tokenizer.iter(), alloc, nullptr, {}};
    (void)parseInfo.tokenIter.next();
    DynArray<StackVariable> variables = parseFunctionArgs(&parseInfo).takeValue();
    {
        StackVariable& variable = variables[0];
        CHECK_FALSE(variable.isTemporary);
        CHECK_FALSE(variable.isMutable);
        CHECK_EQ(variable.name.asSlice(), "arg1");
        CHECK(variable.typeInfo.knownType.hasValue());
        CHECK_EQ(variable.typeInfo.knownType.value(), Type::TYPE_I8);
    }
    {
        StackVariable& variable = variables[1];
        CHECK_FALSE(variable.isTemporary);
        CHECK(variable.isMutable);
        CHECK_EQ(variable.name.asSlice(), "arg2");
        CHECK(variable.typeInfo.knownType.hasValue());
        CHECK_EQ(variable.typeInfo.knownType.value(), Type::TYPE_U64);
    }
}

TEST_CASE("parse function args trailing comma") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "(arg1: i8, mut arg2: u64,)").takeValue();
    ParseInfo parseInfo{tokenizer.iter(), alloc, nullptr, {}};
    (void)parseInfo.tokenIter.next();
    DynArray<StackVariable> variables = parseFunctionArgs(&parseInfo).takeValue();
    {
        StackVariable& variable = variables[0];
        CHECK_FALSE(variable.isTemporary);
        CHECK_FALSE(variable.isMutable);
        CHECK_EQ(variable.name.asSlice(), "arg1");
        CHECK(variable.typeInfo.knownType.hasValue());
        CHECK_EQ(variable.typeInfo.knownType.value(), Type::TYPE_I8);
    }
    {
        StackVariable& variable = variables[1];
        CHECK_FALSE(variable.isTemporary);
        CHECK(variable.isMutable);
        CHECK_EQ(variable.name.asSlice(), "arg2");
        CHECK(variable.typeInfo.knownType.hasValue());
        CHECK_EQ(variable.typeInfo.knownType.value(), Type::TYPE_U64);
    }
}

#endif // SYNC_LIB_WITH_TESTS
