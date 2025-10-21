#include "function_definition.hpp"
#include "../../../types/type_info.hpp"
#include "../../../util/assert.hpp"
#include "../parser.hpp"

using namespace sy;

static Result<DynArray<StackVariable>, ProgramError> parseFunctionArgs(ParseInfo* parseInfo) {
    sy_assert(parseInfo->tokenIter.current().tag() == TokenType::LeftParenthesesSymbol, "Expected left parentheses");

    DynArray<StackVariable> args(parseInfo->alloc);
    const StringSlice source = parseInfo->tokenIter.source();
    auto makeEndOfFileError = [source, parseInfo]() -> Error<ProgramError> {
        parseInfo->reportErr(ProgramError::CompileFunctionSignature, static_cast<uint32_t>(source.len() - 1),
                             "Unexpected end of file");
        return Error(ProgramError::CompileFunctionSignature);
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
            parseInfo->reportErr(ProgramError::CompileFunctionSignature, token.location(),
                                 "Expected identifier for argument name");
            return Error(ProgramError::CompileFunctionSignature);
        }

        StringSlice identifier = parseInfo->tokenIter.currentSlice();

        { // colon
            auto opt = parseInfo->tokenIter.next();
            if (opt.has_value() == false) {
                return makeEndOfFileError();
            }
            token = opt.value();
            if (token.tag() != TokenType::ColonSymbol) {
                parseInfo->reportErr(ProgramError::CompileFunctionSignature, token.location(),
                                     "Expected color for argument type");
                return Error(ProgramError::CompileFunctionSignature);
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
                parseInfo->reportErr(ProgramError::CompileFunctionSignature, token.location(),
                                     "Failed to parse argument type");
                return Error(ProgramError::CompileFunctionSignature);
            }
            variable.typeInfo = typeParseRes.takeValue();
        }

        { // actually make string from identifier
            auto strResult = String::copyConstructSlice(identifier, parseInfo->alloc);
            if (!strResult) {
                parseInfo->reportErr(ProgramError::OutOfMemory, parseInfo->tokenIter.current().location(),
                                     "Out of memory");
                return Error(ProgramError::OutOfMemory);
            }
            new (&variable.name) String(strResult.takeValue());
        }

        // push into array
        if (args.push(std::move(variable)).hasErr()) {
            parseInfo->reportErr(ProgramError::OutOfMemory, parseInfo->tokenIter.current().location(), "Out of memory");
            return Error(ProgramError::OutOfMemory);
        }

        { // move iterator forward to next argument
            auto opt = parseInfo->tokenIter.next();
            if (opt.has_value() == false) {
                return makeEndOfFileError();
            }
            token = opt.value();
            if (token.tag() != TokenType::CommaSymbol && token.tag() != TokenType::RightParenthesesSymbol) {
                parseInfo->reportErr(ProgramError::CompileFunctionSignature, token.location(),
                                     "Expected , or ) symbols");
                return Error(ProgramError::CompileFunctionSignature);
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
    auto makeEndOfFileError = [source, parseInfo]() -> Error<ProgramError> {
        parseInfo->reportErr(ProgramError::CompileFunctionSignature, static_cast<uint32_t>(source.len() - 1),
                             "Unexpected end of file");
        return Error(ProgramError::CompileFunctionSignature);
    };

    { // function name
        auto result = parseInfo->tokenIter.next();
        if (result.has_value() == false) {
            return makeEndOfFileError();
        }
        const Token next = result.value();
        if (next.tag() != TokenType::Identifier) {
            parseInfo->reportErr(ProgramError::CompileFunctionSignature, next.location(),
                                 "Expected identifier for argument name");
            return Error(ProgramError::CompileFunctionSignature);
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
            parseInfo->reportErr(ProgramError::CompileFunctionSignature, next.location(), "Expected ( symbol");
            return Error(ProgramError::CompileFunctionSignature);
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
                parseInfo->reportErr(ProgramError::CompileFunctionSignature, parseInfo->tokenIter.current().location(),
                                     "Failed to parse function return type");
                return Error(ProgramError::CompileFunctionSignature);
            }
            this->retType = Option<TypeResolutionInfo>(typeParseRes.takeValue());
            // step forward
            if (parseInfo->tokenIter.next().has_value() == false) {
                return makeEndOfFileError();
            }
        }
    }

    { // function body
        auto scopeRes = this->alloc().allocObject<Scope>();
        if (scopeRes.hasErr()) {
            parseInfo->reportErr(ProgramError::OutOfMemory, parseInfo->tokenIter.current().location(), "Out of memory");
            return Error(ProgramError::OutOfMemory);
        }
        this->scope = scopeRes.value();
        new (this->scope) Scope();
        this->scope->isInFunction = true;
        this->scope->isSync = false;
        this->scope->parent = outerScope;

        auto statementRes = parseStatement(parseInfo, &this->localVariables, this->scope);
        if (statementRes.hasErr()) {
            return Error(statementRes.takeErr());
        }
        auto statement = statementRes.takeValue();
        if (statement.hasValue() == false) {
            if (this->statements.push(statement.value()).hasErr()) {
                parseInfo->reportErr(ProgramError::OutOfMemory, parseInfo->tokenIter.current().location(),
                                     "Out of memory");
                return Error(ProgramError::OutOfMemory);
            }
        }
    }

    return {};
}

#if SYNC_LIB_WITH_TESTS

#include "../../../doctest.h"

TEST_CASE("parse function args") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "(arg1: i8, mut arg2: u64)").takeValue();
    ParseInfo parseInfo{tokenizer.iter(), alloc, nullptr, {}, nullptr, nullptr};
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
    ParseInfo parseInfo{tokenizer.iter(), alloc, nullptr, {}, nullptr, nullptr};
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
