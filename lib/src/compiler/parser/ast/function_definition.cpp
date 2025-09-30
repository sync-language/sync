#include "function_definition.hpp"
#include "../../../types/type_info.hpp"
#include "../../../util/assert.hpp"
#include "../parser.hpp"

using namespace sy;

static Result<DynArray<StackVariable>, CompileError> parseFunctionArgs(ParseInfo* parseInfo) {
    sy_assert(parseInfo->tokenIter.current().tag() == TokenType::LeftParenthesesSymbol, "Expected left parentheses");

    DynArray<StackVariable> args(parseInfo->alloc);
    const StringSlice source = parseInfo->tokenizer.source();
    auto makeEndOfFileError = [source]() -> Error<CompileError> {
        return Error(CompileError::createInvalidFunctionSignature(
            detail::sourceLocationFromFileLocation(source, static_cast<uint32_t>(source.len() - 1))));
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
            return Error(CompileError::createInvalidFunctionSignature(
                detail::sourceLocationFromFileLocation(source, token.location())));
        }

        StringSlice identifier = parseInfo->tokenIter.currentSlice();

        { // colon
            auto opt = parseInfo->tokenIter.next();
            if (opt.has_value() == false) {
                return makeEndOfFileError();
            }
            token = opt.value();
            if (token.tag() != TokenType::ColonSymbol) {
                return Error(CompileError::createInvalidFunctionSignature(
                    detail::sourceLocationFromFileLocation(source, token.location())));
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
                return Error(CompileError::createInvalidFunctionSignature(
                    detail::sourceLocationFromFileLocation(source, token.location())));
            }
            variable.typeInfo = typeParseRes.takeValue();
        }

        { // actually make string from identifier
            auto strResult = String::copyConstructSlice(identifier, parseInfo->alloc);
            if (!strResult) {
                return Error(CompileError::createOutOfMemory());
            }
            new (&variable.name) String(strResult.takeValue());
        }

        // push into array
        if (args.push(std::move(variable)).hasErr()) {
            return Error(CompileError::createOutOfMemory());
        }

        { // move iterator forward to next argument
            auto opt = parseInfo->tokenIter.next();
            if (opt.has_value() == false) {
                return makeEndOfFileError();
            }
            token = opt.value();
            if (token.tag() != TokenType::CommaSymbol && token.tag() != TokenType::RightParenthesesSymbol) {
                return Error(CompileError::createInvalidFunctionSignature(
                    detail::sourceLocationFromFileLocation(source, token.location())));
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

Result<void, CompileError> sy::FunctionDefinitionNode::init(ParseInfo* parseInfo, Scope* outerScope) {
    sy_assert(parseInfo->tokenIter.current().tag() == TokenType::FnKeyword,
              "Function definition node must be initialized from a fn keyword token");

    const StringSlice source = parseInfo->tokenizer.source();
    auto makeEndOfFileError = [source]() -> Error<CompileError> {
        return Error(CompileError::createInvalidFunctionSignature(
            detail::sourceLocationFromFileLocation(source, static_cast<uint32_t>(source.len() - 1))));
    };

    { // function name
        auto result = parseInfo->tokenIter.next();
        if (result.has_value() == false) {
            return makeEndOfFileError();
        }
        const Token next = result.value();
        if (next.tag() != TokenType::Identifier) {
            return Error(CompileError::createInvalidFunctionSignature(
                detail::sourceLocationFromFileLocation(source, next.location())));
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
            return Error(CompileError::createInvalidFunctionSignature(
                detail::sourceLocationFromFileLocation(source, next.location())));
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
                return Error(CompileError::createInvalidFunctionSignature(
                    detail::sourceLocationFromFileLocation(source, parseInfo->tokenIter.current().location())));
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
            return Error(CompileError::createInvalidFunctionSignature(
                detail::sourceLocationFromFileLocation(source, parseInfo->tokenIter.current().location())));
        }
    }

    (void)outerScope;

    return {};
}

#if SYNC_LIB_WITH_TESTS

#include "../../../doctest.h"

TEST_CASE("parse function args") {
    Allocator alloc;
    ParseInfo parseInfo{{}, alloc, Tokenizer::create(alloc, "(arg1: i8, mut arg2: u64)").takeValue(), nullptr, {}};
    parseInfo.tokenIter = parseInfo.tokenizer.iter();
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
    ParseInfo parseInfo{{}, alloc, Tokenizer::create(alloc, "(arg1: i8, mut arg2: u64,)").takeValue(), nullptr, {}};
    parseInfo.tokenIter = parseInfo.tokenizer.iter();
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
