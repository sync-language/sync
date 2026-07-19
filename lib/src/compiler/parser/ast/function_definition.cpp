#include "function_definition.hpp"
#include "../../../core/core_internal.h"
#include "../../../interpreter/function_builder.hpp"
#include "../../../types/type_info.hpp"
#include "../../source_tree/source_tree.hpp"
#include "../parser.hpp"

using namespace sy;

static Result<DynArray<StackVariable>, CompileError> parseFunctionArgs(ParseInfo* parseInfo) {
    sy_assert(parseInfo->tokenIter.current().tag() == TokenType::LeftParenthesesSymbol,
              "Expected left parentheses");

    DynArray<StackVariable> args(parseInfo->alloc);
    const StringSlice source = parseInfo->tokenIter.source();
    auto makeEndOfFileError = [source, parseInfo]() -> Error<CompileError> {
        parseInfo->reportErr(CompileError::CompileFunctionSignature,
                             static_cast<uint32_t>(source.len() - 1), "Unexpected end of file");
        return Error(CompileError::CompileFunctionSignature);
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
            parseInfo->reportErr(CompileError::CompileFunctionSignature, token.location(),
                                 "Expected identifier for argument name");
            return Error(CompileError::CompileFunctionSignature);
        }

        StringSlice identifier = parseInfo->tokenIter.currentSlice();

        { // colon
            auto opt = parseInfo->tokenIter.next();
            if (opt.has_value() == false) {
                return makeEndOfFileError();
            }
            token = opt.value();
            if (token.tag() != TokenType::ColonSymbol) {
                parseInfo->reportErr(CompileError::CompileFunctionSignature, token.location(),
                                     "Expected color for argument type");
                return Error(CompileError::CompileFunctionSignature);
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
                parseInfo->reportErr(CompileError::CompileFunctionSignature, token.location(),
                                     "Failed to parse argument type");
                return Error(CompileError::CompileFunctionSignature);
            }
            variable.typeInfo = typeParseRes.takeValue();
        }

        { // actually make string from identifier
            auto strResult = String::init(identifier, parseInfo->alloc);
            if (!strResult) {
                parseInfo->reportErr(CompileError::OutOfMemory,
                                     parseInfo->tokenIter.current().location(), "Out of memory");
                return Error(CompileError::OutOfMemory);
            }
            new (&variable.name) String(strResult.takeValue());
        }

        // push into array
        if (args.push(std::move(variable)).hasErr()) {
            parseInfo->reportErr(CompileError::OutOfMemory,
                                 parseInfo->tokenIter.current().location(), "Out of memory");
            return Error(CompileError::OutOfMemory);
        }

        { // move iterator forward to next argument
            auto opt = parseInfo->tokenIter.next();
            if (opt.has_value() == false) {
                return makeEndOfFileError();
            }
            token = opt.value();
            if (token.tag() != TokenType::CommaSymbol &&
                token.tag() != TokenType::RightParenthesesSymbol) {
                parseInfo->reportErr(CompileError::CompileFunctionSignature, token.location(),
                                     "Expected , or ) symbols");
                return Error(CompileError::CompileFunctionSignature);
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
    if (this->scope != nullptr) {
        this->alloc().freeObject(this->scope);
        this->scope = nullptr;
    }
}

Result<void, CompileError> sy::FunctionDefinitionNode::init(ParseInfo* parseInfo,
                                                            Scope* outerScope) noexcept {
    sy_assert(parseInfo->tokenIter.current().tag() == TokenType::FnKeyword,
              "Function definition node must be initialized from a fn keyword token");

    const StringSlice source = parseInfo->tokenIter.source();
    auto makeEndOfFileError = [source, parseInfo]() -> Error<CompileError> {
        parseInfo->reportErr(CompileError::CompileFunctionSignature,
                             static_cast<uint32_t>(source.len() - 1), "Unexpected end of file");
        return Error(CompileError::CompileFunctionSignature);
    };

    { // function name
        auto result = parseInfo->tokenIter.next();
        if (result.has_value() == false) {
            return makeEndOfFileError();
        }
        const Token next = result.value();
        if (next.tag() != TokenType::Identifier) {
            parseInfo->reportErr(CompileError::CompileFunctionSignature, next.location(),
                                 "Expected identifier for argument name");
            return Error(CompileError::CompileFunctionSignature);
        }

        this->functionName = parseInfo->tokenIter.currentSlice();

        // TODO scoped functions
        auto fileNameRes = String::init(parseInfo->moduleName, parseInfo->alloc);
        if (fileNameRes.hasErr()) {
            parseInfo->reportErr(CompileError::OutOfMemory,
                                 parseInfo->tokenIter.current().location(), "Out of memory");
            return Error(CompileError::OutOfMemory);
        }

        new (&this->functionQualifiedName) String(std::move(fileNameRes.takeValue()));
        if (auto qualifiedConcatRes = this->functionQualifiedName.concat(".");
            qualifiedConcatRes.hasErr()) {
            parseInfo->reportErr(CompileError::OutOfMemory,
                                 parseInfo->tokenIter.current().location(), "Out of memory");
            return Error(CompileError::OutOfMemory);
        } else {
            this->functionQualifiedName = qualifiedConcatRes.takeValue();
        }

        if (auto qualifiedConcatRes = this->functionQualifiedName.concat(this->functionName);
            qualifiedConcatRes.hasErr()) {
            parseInfo->reportErr(CompileError::OutOfMemory,
                                 parseInfo->tokenIter.current().location(), "Out of memory");
            return Error(CompileError::OutOfMemory);
        } else {
            this->functionQualifiedName = qualifiedConcatRes.takeValue();
        }
    }

    { // arguments
        auto nextResult = parseInfo->tokenIter.next();
        if (nextResult.has_value() == false) {
            return makeEndOfFileError();
        }
        const Token next = nextResult.value();
        if (next.tag() != TokenType::LeftParenthesesSymbol) {
            parseInfo->reportErr(CompileError::CompileFunctionSignature, next.location(),
                                 "Expected ( symbol");
            return Error(CompileError::CompileFunctionSignature);
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
        if (token.tag() == TokenType::LeftBraceSymbol) {
            this->retType = Option<TypeResolutionInfo>();
            if (parseInfo->tokenIter.next().has_value() == false) {
                return makeEndOfFileError();
            }
        } else {
            auto typeParseRes = TypeResolutionInfo::parse(parseInfo);
            if (typeParseRes.hasErr()) {
                parseInfo->reportErr(CompileError::CompileFunctionSignature,
                                     parseInfo->tokenIter.current().location(),
                                     "Failed to parse function return type");
                return Error(CompileError::CompileFunctionSignature);
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
            parseInfo->reportErr(CompileError::OutOfMemory,
                                 parseInfo->tokenIter.current().location(), "Out of memory");
            return Error(CompileError::OutOfMemory);
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
        if (statement.hasValue()) {
            if (this->statements.push(statement.value()).hasErr()) {
                parseInfo->reportErr(CompileError::OutOfMemory,
                                     parseInfo->tokenIter.current().location(), "Out of memory");
                return Error(CompileError::OutOfMemory);
            }
        }
    }

    return {};
}

Result<FunctionBuilder, CompileError> sy::FunctionDefinitionNode::compile() const noexcept {
    FunctionBuilder builder(this->alloc());
    if (this->retType.hasValue()) {
        const auto& optKnown = this->retType.value().knownType;
        if (optKnown.hasValue() == false) {
            return Error(CompileError::CompileUnknownType);
        }
        builder.retType = optKnown;
    }

    for (const auto& arg : this->args) {
        const auto& optKnown = arg.typeInfo.knownType;
        if (optKnown.hasValue() == false) {
            return Error(CompileError::CompileUnknownType);
        }
        if (auto res = builder.addArg(optKnown.value()); res.hasErr()) {
            return Error(CompileError::OutOfMemory);
        }
    }

    for (const auto& statement : this->statements) {
        if (auto res = statement->compileStatement(&builder); res.hasErr()) {
            return Error(res.takeErr());
        }
    }

    return builder;
}

#if SYNC_LIB_WITH_TESTS

#include "../../../doctest.h"

// TEST_CASE("[FunctionDefintion] parse function args") {
//     Allocator alloc;
//     Tokenizer tokenizer = Tokenizer::create(alloc, "(arg1: i8, mut arg2: u64)").takeValue();
//     ParseInfo parseInfo = ParseInfo(tokenizer.iter(), alloc, {}, nullptr, nullptr);
//     (void)parseInfo.tokenIter.next();
//     DynArray<StackVariable> variables = parseFunctionArgs(&parseInfo).takeValue();
//     {
//         StackVariable& variable = variables[0];
//         CHECK_FALSE(variable.isTemporary);
//         CHECK_FALSE(variable.isMutable);
//         CHECK_EQ(variable.name.asSlice(), "arg1");
//         CHECK(variable.typeInfo.knownType.hasValue());
//         CHECK_EQ(variable.typeInfo.knownType.value(), Type::TYPE_I8);
//     }
//     {
//         StackVariable& variable = variables[1];
//         CHECK_FALSE(variable.isTemporary);
//         CHECK(variable.isMutable);
//         CHECK_EQ(variable.name.asSlice(), "arg2");
//         CHECK(variable.typeInfo.knownType.hasValue());
//         CHECK_EQ(variable.typeInfo.knownType.value(), Type::TYPE_U64);
//     }
// }

// TEST_CASE("[FunctionDefintion] parse function args trailing comma") {
//     Allocator alloc;
//     Tokenizer tokenizer = Tokenizer::create(alloc, "(arg1: i8, mut arg2: u64,)").takeValue();
//     ParseInfo parseInfo = ParseInfo(tokenizer.iter(), alloc, {}, nullptr, nullptr);
//     (void)parseInfo.tokenIter.next();
//     DynArray<StackVariable> variables = parseFunctionArgs(&parseInfo).takeValue();
//     {
//         StackVariable& variable = variables[0];
//         CHECK_FALSE(variable.isTemporary);
//         CHECK_FALSE(variable.isMutable);
//         CHECK_EQ(variable.name.asSlice(), "arg1");
//         CHECK(variable.typeInfo.knownType.hasValue());
//         CHECK_EQ(variable.typeInfo.knownType.value(), Type::TYPE_I8);
//     }
//     {
//         StackVariable& variable = variables[1];
//         CHECK_FALSE(variable.isTemporary);
//         CHECK(variable.isMutable);
//         CHECK_EQ(variable.name.asSlice(), "arg2");
//         CHECK(variable.typeInfo.knownType.hasValue());
//         CHECK_EQ(variable.typeInfo.knownType.value(), Type::TYPE_U64);
//     }
// }

TEST_CASE("[FunctionDefintion] parse no args") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "()").takeValue();
    ParseInfo parseInfo = ParseInfo(tokenizer.iter(), alloc, {}, nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    DynArray<StackVariable> variables = parseFunctionArgs(&parseInfo).takeValue();
    CHECK_EQ(variables.len(), 0);
}

TEST_CASE("[FunctionDefintion] parse function return no value statement") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "fn example() { return; }").takeValue();
    ParseInfo parseInfo = ParseInfo(tokenizer.iter(), alloc, "hello", nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    FunctionDefinitionNode funcDef = FunctionDefinitionNode(alloc);
    Scope outerScope{};
    CHECK(funcDef.init(&parseInfo, &outerScope));
    CHECK_EQ(funcDef.unqualifiedName(), "example");
    CHECK_EQ(funcDef.qualifiedName(), "hello.example");
    CHECK_EQ(funcDef.args.len(), 0);
    CHECK_FALSE(funcDef.retType.hasValue());
    CHECK_EQ(funcDef.localVariables.len(), 0);
    CHECK_EQ(funcDef.statements.len(), 1);
}

#endif // SYNC_LIB_WITH_TESTS
