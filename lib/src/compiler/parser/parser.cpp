#include "parser.hpp"
#include "../../util/assert.hpp"
#include "../source_tree/source_tree.hpp"
#include "ast/function_definition.hpp"
#include "ast/return.hpp"

using namespace sy;

sy::FileAst::~FileAst() noexcept {
    for (size_t i = 0; i < this->functions.len(); i++) {
        delete this->functions[i];
    }
    for (size_t i = 0; i < this->structs.len(); i++) {
        delete this->structs[i];
    }

    this->functions.destroy(this->alloc);
    this->structs.destroy(this->alloc);
}

Result<Option<IFunctionStatement*>, ProgramError>
sy::parseStatement(ParseInfo* parseInfo, DynArray<StackVariable>* localVariables, Scope* currentScope) noexcept {
    const TokenType tokenType = parseInfo->tokenIter.current().tag();
    if (tokenType == TokenType::RightBraceSymbol) {
        return Option<IFunctionStatement*>(); // no more nodes
    }

    auto makeOutOfMemory = [parseInfo]() {
        return Error(
            ProgramError(SourceFileLocation(parseInfo->tokenIter.source(), parseInfo->tokenIter.current().location()),
                         ProgramError::Kind::OutOfMemory));
    };

    switch (tokenType) {
    case TokenType::ReturnKeyword: {
        ReturnNode* node = new (parseInfo->alloc) ReturnNode(parseInfo->alloc);
        if (node == nullptr)
            return makeOutOfMemory();

        if (auto res = node->init(parseInfo, localVariables, currentScope); res.hasErr()) {
            delete node;
            return Error(res.takeErr());
        }
        return node;
    } break;
    default:
        return Error(
            ProgramError(SourceFileLocation(parseInfo->tokenIter.source(), parseInfo->tokenIter.current().location()),
                         ProgramError::Kind::CompileFunctionStatement));
    }
}

Result<FileAst, ProgramError> sy::parseFile(Allocator alloc, const SourceTreeNode* fileSource) noexcept {
    sy_assert(fileSource->kind == SourceFileKind::SyncSourceFile, "Expected Sync source code file");

    FileAst ast{};
    ast.alloc = alloc;
    const StringSlice fileSlice = fileSource->elem.syncSourceFile.value().asSlice();
    auto tokenizerRes = Tokenizer::create(alloc, fileSlice);
    if (tokenizerRes.hasErr()) {
        return Error(tokenizerRes.takeErr());
    }
    Tokenizer tokenizer = tokenizerRes.takeValue();
    ParseInfo parseInfo{tokenizer.iter(), alloc, fileSource, {}};

    {
        auto nextOpt = parseInfo.tokenIter.next();
        while (nextOpt.has_value()) {
            Token next = nextOpt.value();
            switch (next.tag()) {
            case TokenType::EndOfFile:
                break;
            case TokenType::FnKeyword: {
                FunctionDefinitionNode* func = new (alloc) FunctionDefinitionNode(alloc);
                if (func == nullptr) {
                    return Error(ProgramError(
                        SourceFileLocation(parseInfo.tokenIter.source(), parseInfo.tokenIter.current().location()),
                        ProgramError::Kind::OutOfMemory));
                }

                if (auto res = func->init(&parseInfo, &ast.scope); res.hasErr()) {
                    return Error(res.takeErr());
                }

                if (ast.functions.push(func, alloc).hasErr()) {
                    return Error(ProgramError(
                        SourceFileLocation(parseInfo.tokenIter.source(), parseInfo.tokenIter.current().location()),
                        ProgramError::Kind::OutOfMemory));
                }
                ScopeSymbol symbol{};
                symbol.symbolType = ScopeSymbol::Tag::Function;
                symbol.data.function = func->functionName;
                if (ast.scope.symbols.insert(alloc, symbol, ast.scope.symbols.len()).hasErr()) {
                    return Error(ProgramError(
                        SourceFileLocation(parseInfo.tokenIter.source(), parseInfo.tokenIter.current().location()),
                        ProgramError::Kind::OutOfMemory));
                }
                nextOpt = parseInfo.tokenIter.next();
            } break;
            case TokenType::StructKeyword: {
                sy_assert(false, "Struct not yet implemented");
            } break;
            default: {
                return Error(ProgramError(
                    SourceFileLocation(parseInfo.tokenIter.source(), parseInfo.tokenIter.current().location()),
                    ProgramError::Kind::CompileSymbol));
            }
            }
            nextOpt = parseInfo.tokenIter.next();
        }
    }

    return ast;
}

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"

TEST_CASE("parseStatement right brace") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "}").takeValue();
    ParseInfo parseInfo{tokenizer.iter(), alloc, nullptr, {}};
    (void)parseInfo.tokenIter.next();
    auto res = parseStatement(&parseInfo, nullptr, nullptr);
    CHECK(res.hasValue());
    CHECK_FALSE(res.value().hasValue());
}

TEST_CASE("parseStatement return") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "return;").takeValue();
    ParseInfo parseInfo{tokenizer.iter(), alloc, nullptr, {}};
    (void)parseInfo.tokenIter.next();
    auto res = parseStatement(&parseInfo, nullptr, nullptr);
    CHECK(res.hasValue());
    CHECK(res.value().hasValue());
    IFunctionStatement* statement = res.value().value();
    CHECK_NE(dynamic_cast<ReturnNode*>(statement), nullptr);
}

TEST_CASE("parseFile empty") {
    Allocator alloc;
    SourceTreeNode source{alloc, {}, {}, SourceFileKind::SyncSourceFile, {}};
    new (&source.elem.syncSourceFile)
        Option<StringUnmanaged>(StringUnmanaged::copyConstructSlice("", alloc).takeValue());

    auto res = parseFile(alloc, &source);
    CHECK(res);
    FileAst ast = res.takeValue();
    CHECK_EQ(ast.functions.len(), 0);
    CHECK_EQ(ast.structs.len(), 0);
    CHECK_FALSE(ast.scope.isInFunction);
    CHECK_FALSE(ast.scope.isSync);
    CHECK_EQ(ast.scope.syncVariables.len(), 0);
    CHECK_EQ(ast.scope.symbols.len(), 0);
    CHECK_FALSE(ast.scope.parent.hasValue());
}

#endif // SYNC_LIB_WITH_TESTS
