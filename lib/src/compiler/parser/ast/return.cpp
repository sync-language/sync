#include "return.hpp"
#include "../../../util/assert.hpp"
#include "../parser.hpp"

using namespace sy;

Result<void, CompileError> sy::ReturnNode::init(ParseInfo* parseInfo, DynArray<StackVariable>* variables,
                                                Scope*) noexcept {
    sy_assert(parseInfo->tokenIter.current().tag() == TokenType::ReturnKeyword, "Expected return keyword");
    sy_assert(this->retValue.hasValue() == false, "Should not already have an expression value to return");

    const StringSlice source = parseInfo->tokenIter.source();
    auto makeEndOfFileError = [source]() -> Error<CompileError> {
        return Error(CompileError::createInvalidFunctionSignature(
            detail::sourceLocationFromFileLocation(source, static_cast<uint32_t>(source.len() - 1))));
    };

    auto result = parseInfo->tokenIter.next();
    if (result.has_value() == false) {
        return makeEndOfFileError();
    }

    const Token next = result.value();
    if (next.tag() == TokenType::SemicolonSymbol) {
        return {};
    }

    auto expressionResult = Expression::parse(parseInfo, variables, {});
    if (expressionResult.hasErr()) {
        return Error(expressionResult.takeErr());
    }

    this->retValue = expressionResult.takeValue();
    return {};
}

#if SYNC_LIB_WITH_TESTS

#include "../../../doctest.h"

TEST_CASE("ReturnNode no expression") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "return;").takeValue();
    ParseInfo parseInfo{tokenizer.iter(), alloc, nullptr, {}};
    (void)parseInfo.tokenIter.next();
    ReturnNode* ret = new (alloc) ReturnNode(alloc);
    DynArray<StackVariable> variables{};
    CHECK(ret->init(&parseInfo, &variables, nullptr));
    CHECK_FALSE(ret->retValue.hasValue());
}

TEST_CASE("ReturnNode boolean literal true") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "return true;").takeValue();
    ParseInfo parseInfo{tokenizer.iter(), alloc, nullptr, {}};
    (void)parseInfo.tokenIter.next();
    ReturnNode* ret = new (alloc) ReturnNode(alloc);
    DynArray<StackVariable> variables{};
    CHECK(ret->init(&parseInfo, &variables, nullptr));
    CHECK(ret->retValue.hasValue());
    CHECK(ret->retValue.value().tag == Expression::ExprType::BoolLit);
    CHECK(ret->retValue.value().metadata.boolLit);
}

TEST_CASE("ReturnNode boolean literal false") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "return false;").takeValue();
    ParseInfo parseInfo{tokenizer.iter(), alloc, nullptr, {}};
    (void)parseInfo.tokenIter.next();
    ReturnNode ret = ReturnNode(alloc);
    DynArray<StackVariable> variables{};
    CHECK(ret.init(&parseInfo, &variables, nullptr));
    CHECK(ret.retValue.hasValue());
    CHECK(ret.retValue.value().tag == Expression::ExprType::BoolLit);
    CHECK_FALSE(ret.retValue.value().metadata.boolLit);
}

#endif // SYNC_LIB_WITH_TESTS