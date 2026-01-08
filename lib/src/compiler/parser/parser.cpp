#include "parser.hpp"
#include "../../core/core_internal.h"
#include "../source_tree/source_tree.hpp"
#include "ast/function_definition.hpp"
#include "ast/return.hpp"

using namespace sy;

namespace sy {
extern ProgramErrorReporter defaultErrReporter;
}

Error<ProgramError> sy::ParseInfo::reportErr(ProgramError errKind, uint32_t inBytePos, StringSlice msg) const noexcept {
    SourceFileLocation asFileLocation = SourceFileLocation(this->tokenIter.source(), inBytePos);
    asFileLocation.fileName = this->moduleName;
    if (this->errReporter)
        this->errReporter(errKind, asFileLocation, msg, this->errReporterArg);
    return Error(errKind);
}

sy::FileAst::~FileAst() noexcept {
    for (size_t i = 0; i < this->nonGenericFunctions.len(); i++) {
        delete this->nonGenericFunctions[i];
    }
    for (size_t i = 0; i < this->nonGenericStructs.len(); i++) {
        delete this->nonGenericStructs[i];
    }

    this->nonGenericFunctions.destroy(this->alloc);
    this->nonGenericStructs.destroy(this->alloc);
}

Result<Option<IFunctionStatement*>, ProgramError>
sy::parseStatement(ParseInfo* parseInfo, DynArray<StackVariable>* localVariables, Scope* currentScope) noexcept {
    const TokenType tokenType = parseInfo->tokenIter.current().tag();
    if (tokenType == TokenType::RightBraceSymbol) {
        return Option<IFunctionStatement*>(); // no more nodes
    }

    auto makeOutOfMemory = [parseInfo]() {
        parseInfo->reportErr(ProgramError::OutOfMemory, parseInfo->tokenIter.current().location(), "Out of memory");
        return Error(ProgramError::OutOfMemory);
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
        return Option<IFunctionStatement*>(node);
    } break;
    default:
        parseInfo->reportErr(ProgramError::CompileFunctionStatement, parseInfo->tokenIter.current().location(),
                             "Unknown token for start of statement");
        return Error(ProgramError::CompileFunctionStatement);
    }
}

Result<FileAst, ProgramError> sy::parseFile(Allocator alloc, const SourceTreeNode* fileSource,
                                            ProgramErrorReporter errReporter, void* errReporterArg) noexcept {
    sy_assert(fileSource->kind == SourceFileKind::SyncSourceFile, "Expected Sync source code file");

    if (errReporter == nullptr) {
        errReporter = defaultErrReporter;
    }

    FileAst ast{};
    ast.alloc = alloc;
    const StringSlice fileSlice = fileSource->elem.syncSourceFile.value().asSlice();
    auto tokenizerRes = Tokenizer::create(alloc, fileSlice);
    if (tokenizerRes.hasErr()) {
        errReporter(tokenizerRes.err(), SourceFileLocation(), StringSlice("Tokenizer"), errReporterArg);
        return Error(tokenizerRes.takeErr());
    }
    Tokenizer tokenizer = tokenizerRes.takeValue();
    ParseInfo parseInfo = ParseInfo(tokenizer.iter(), alloc, fileSource->elem.syncSourceFile.value().asSlice(),
                                    errReporter, errReporterArg);

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
                    parseInfo.reportErr(ProgramError::OutOfMemory, parseInfo.tokenIter.current().location(),
                                        "Out of memory");
                    return Error(ProgramError::OutOfMemory);
                }

                if (auto res = func->init(&parseInfo, &ast.scope); res.hasErr()) {
                    return Error(res.takeErr());
                }

                if (ast.nonGenericFunctions.push(func, alloc).hasErr()) {
                    parseInfo.reportErr(ProgramError::OutOfMemory, parseInfo.tokenIter.current().location(),
                                        "Out of memory");
                    return Error(ProgramError::OutOfMemory);
                }
                ScopeSymbol symbol{};
                symbol.symbolType = ScopeSymbol::Tag::Function;
                symbol.data.function = func->functionName;
                if (ast.scope.symbols.insert(alloc, symbol, ast.scope.symbols.len()).hasErr()) {
                    parseInfo.reportErr(ProgramError::OutOfMemory, parseInfo.tokenIter.current().location(),
                                        "Out of memory");
                    return Error(ProgramError::OutOfMemory);
                }
                nextOpt = parseInfo.tokenIter.next();
            } break;
            case TokenType::StructKeyword: {
                sy_assert(false, "Struct not yet implemented");
            } break;
            default: {
                parseInfo.reportErr(ProgramError::CompileSymbol, parseInfo.tokenIter.current().location(),
                                    "Expected end of file, fn keyword, or struct keyword");
                return Error(ProgramError::CompileSymbol);
            }
            }
            nextOpt = parseInfo.tokenIter.next();
        }
    }

    new (&ast.imports) MapUnmanaged<StringSlice, bool>(std::move(parseInfo.imports));

    return Result<FileAst, ProgramError>(std::move(ast));
}

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"

TEST_CASE("parseStatement right brace") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "}").takeValue();
    ParseInfo parseInfo = ParseInfo(tokenizer.iter(), alloc, {}, nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    auto res = parseStatement(&parseInfo, nullptr, nullptr);
    CHECK(res.hasValue());
    CHECK_FALSE(res.value().hasValue());
}

TEST_CASE("parseStatement return") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "return;").takeValue();
    ParseInfo parseInfo = ParseInfo(tokenizer.iter(), alloc, {}, nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    auto res = parseStatement(&parseInfo, nullptr, nullptr);
    CHECK(res.hasValue());
    CHECK(res.value().hasValue());
    IFunctionStatement* statement = res.value().value();
    CHECK_NE(dynamic_cast<ReturnNode*>(statement), nullptr);
    delete res.value().value();
}

TEST_CASE("parseFile empty") {
    Allocator alloc;
    SourceTreeNode source{alloc, {}, {}, SourceFileKind::SyncSourceFile, {}};
    new (&source.elem.syncSourceFile)
        Option<StringUnmanaged>(StringUnmanaged::copyConstructSlice("", alloc).takeValue());

    auto res = parseFile(alloc, &source, nullptr, nullptr);
    CHECK(res);
    FileAst ast = res.takeValue();
    CHECK_EQ(ast.nonGenericFunctions.len(), 0);
    CHECK_EQ(ast.nonGenericStructs.len(), 0);
    CHECK_FALSE(ast.scope.isInFunction);
    CHECK_FALSE(ast.scope.isSync);
    CHECK_EQ(ast.scope.syncVariables.len(), 0);
    CHECK_EQ(ast.scope.symbols.len(), 0);
    CHECK_FALSE(ast.scope.parent.hasValue());
}

#endif // SYNC_LIB_WITH_TESTS
