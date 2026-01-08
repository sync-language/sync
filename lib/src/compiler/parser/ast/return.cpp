#include "return.hpp"
#include "../../../core/core_internal.h"
#include "../../../interpreter/bytecode.hpp"
#include "../../../interpreter/function_builder.hpp"
#include "../parser.hpp"

using namespace sy;

Result<void, ProgramError> sy::ReturnNode::init(ParseInfo* parseInfo, DynArray<StackVariable>* variables,
                                                Scope*) noexcept {
    sy_assert(parseInfo->tokenIter.current().tag() == TokenType::ReturnKeyword, "Expected return keyword");
    sy_assert(this->retValue.hasValue() == false, "Should not already have an expression value to return");

    const StringSlice source = parseInfo->tokenIter.source();
    auto makeEndOfFileError = [source, parseInfo]() -> Error<ProgramError> {
        parseInfo->reportErr(ProgramError::CompileFunctionStatement, static_cast<uint32_t>(source.len() - 1),
                             "Unexpected end of file");
        return Error(ProgramError::CompileFunctionStatement);
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

Result<void, ProgramError> sy::ReturnNode::compileStatement(FunctionBuilder* builder) const noexcept {
    if (this->retValue.hasValue() == false) {
        const operators::Return ret = {static_cast<uint64_t>(operators::Return::OPCODE)};
        const Bytecode asBytecode = Bytecode(ret);
        if (builder->pushBytecode(&asBytecode, 1).hasErr()) {
            return Error(ProgramError::OutOfMemory);
        }
    } else {
        auto expressionErr = this->retValue.value().compileExpression(builder);
        if (expressionErr.hasErr()) {
            return expressionErr;
        }

        const operators::ReturnValue ret = {static_cast<uint64_t>(operators::ReturnValue::OPCODE),
                                            static_cast<uint64_t>(this->retValue.value().variableIndex)};
        const Bytecode asBytecode = Bytecode(ret);
        if (builder->pushBytecode(&asBytecode, 1).hasErr()) {
            return Error(ProgramError::OutOfMemory);
        }
    }
    return {};
}

#if SYNC_LIB_WITH_TESTS

#include "../../../doctest.h"

TEST_CASE("ReturnNode no expression") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "return;").takeValue();
    ParseInfo parseInfo = ParseInfo(tokenizer.iter(), alloc, {}, nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ReturnNode ret(alloc);
    DynArray<StackVariable> variables{};
    CHECK(ret.init(&parseInfo, &variables, nullptr));
    CHECK_FALSE(ret.retValue.hasValue());

    FunctionBuilder builder(alloc);
    CHECK(ret.compileStatement(&builder));
    CHECK_EQ(builder.bytecode.len(), 1);
    CHECK_EQ(builder.bytecode[0].getOpcode(), OpCode::Return);
}

TEST_CASE("ReturnNode boolean literal true") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "return true;").takeValue();
    ParseInfo parseInfo = ParseInfo(tokenizer.iter(), alloc, {}, nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ReturnNode ret(alloc);
    DynArray<StackVariable> variables{};
    CHECK(ret.init(&parseInfo, &variables, nullptr));
    CHECK(ret.retValue.hasValue());
    CHECK(ret.retValue.value().tag == Expression::ExprType::BoolLit);
    CHECK(ret.retValue.value().metadata.boolLit);

    FunctionBuilder builder(alloc);
    CHECK(ret.compileStatement(&builder));
    CHECK_EQ(builder.bytecode.len(), 2);
    CHECK_EQ(builder.bytecode[0].getOpcode(), OpCode::LoadImmediateScalar);
    const auto load = *reinterpret_cast<const operators::LoadImmediateScalar*>(&builder.bytecode[0]);
    CHECK_EQ(load.scalarTag, static_cast<uint64_t>(ScalarTag::Bool));
    CHECK_EQ(load.immediate, static_cast<uint64_t>(true));
    CHECK_EQ(builder.bytecode[1].getOpcode(), OpCode::ReturnValue);
}

TEST_CASE("ReturnNode boolean literal false") {
    Allocator alloc;
    Tokenizer tokenizer = Tokenizer::create(alloc, "return false;").takeValue();
    ParseInfo parseInfo = ParseInfo(tokenizer.iter(), alloc, {}, nullptr, nullptr);
    (void)parseInfo.tokenIter.next();
    ReturnNode ret = ReturnNode(alloc);
    DynArray<StackVariable> variables{};
    CHECK(ret.init(&parseInfo, &variables, nullptr));
    CHECK(ret.retValue.hasValue());
    CHECK(ret.retValue.value().tag == Expression::ExprType::BoolLit);
    CHECK_FALSE(ret.retValue.value().metadata.boolLit);

    FunctionBuilder builder(alloc);
    CHECK(ret.compileStatement(&builder));
    CHECK_EQ(builder.bytecode.len(), 2);
    CHECK_EQ(builder.bytecode[0].getOpcode(), OpCode::LoadImmediateScalar);
    const auto load = *reinterpret_cast<const operators::LoadImmediateScalar*>(&builder.bytecode[0]);
    CHECK_EQ(load.scalarTag, static_cast<uint64_t>(ScalarTag::Bool));
    CHECK_EQ(load.immediate, static_cast<uint64_t>(false));
    CHECK_EQ(builder.bytecode[1].getOpcode(), OpCode::ReturnValue);
}

#endif // SYNC_LIB_WITH_TESTS