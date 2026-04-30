#include "expression.hpp"
#include "../../interpreter/bytecode.hpp"
#include "../../interpreter/function_builder.hpp"
#include "base_nodes.hpp"
#include "parser.hpp"
#include <charconv>
#include <cstring>
#include <iostream>

using namespace sy;

sy::Expression::~Expression() noexcept {
    switch (this->tag) {
    case ExprType::Expression: {
        delete this->metadata.expression;
        this->metadata.expression = nullptr;
    }
    default:
        break;
    }
}

static Result<size_t, AllocErr> getOrMakeDstVarIndex(StringSlice partial,
                                                     DynArray<StackVariable>* variables,
                                                     Option<size_t> dstVarIndex,
                                                     Allocator alloc) noexcept {
    if (dstVarIndex.hasValue()) {
        return dstVarIndex.value();
    }

    auto tempRes = String::init("%", alloc);
    if (tempRes.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    String tempName = tempRes.takeValue();
    if (auto concatRes = tempName.concat(partial); concatRes.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    } else {
        tempName = concatRes.takeValue();
    }
    char buf[24]{'\0'}; // more than enough
    std::to_chars(buf, buf + sizeof(buf), variables->len());
    if (auto concatRes = tempName.concat(StringSlice(buf, std::strlen(buf))); concatRes.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    } else {
        tempName = concatRes.takeValue();
    }

    StackVariable variable{
        std::move(tempName),
        true,  // temporary
        false, // mutable
        {}     // type resolution info
    };

    if (variables->push(std::move(variable)).hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    return variables->len() - 1;
}

Result<Expression, ProgramError> sy::Expression::parse(ParseInfo* parseInfo,
                                                       DynArray<StackVariable>* variables,
                                                       Option<size_t> dstVarIndex) noexcept {
    Expression expr{};

    switch (parseInfo->tokenIter.current().tag()) {
    case TokenType::TrueKeyword: {
        auto res = getOrMakeDstVarIndex("true", variables, dstVarIndex, parseInfo->alloc);
        if (res.hasErr()) {
            parseInfo->reportErr(ProgramError::OutOfMemory,
                                 parseInfo->tokenIter.current().location(), "Out of memory");
            return Error(ProgramError::OutOfMemory);
        }
        expr.variableIndex = res.value();
        expr.tag = Expression::ExprType::BoolLit;
        expr.metadata.boolLit = true;
    } break;
    case TokenType::FalseKeyword: {
        auto res = getOrMakeDstVarIndex("false", variables, dstVarIndex, parseInfo->alloc);
        if (res.hasErr()) {
            parseInfo->reportErr(ProgramError::OutOfMemory,
                                 parseInfo->tokenIter.current().location(), "Out of memory");
            return Error(ProgramError::OutOfMemory);
        }
        expr.variableIndex = res.value();
        expr.tag = Expression::ExprType::BoolLit;
        expr.metadata.boolLit = false;
    } break;
    default:
        parseInfo->reportErr(ProgramError::CompileExpression,
                             parseInfo->tokenIter.current().location(), "Invalid expression");
        return Error(ProgramError::CompileExpression);
    }

    return expr;
}

Result<void, ProgramError> sy::Expression::compileExpression(FunctionBuilder* builder) const {
    switch (this->tag) {
    case ExprType::BoolLit: {
        operators::LoadImmediateScalar loadBoolImmediate;
        loadBoolImmediate.reserveOpcode = static_cast<uint64_t>(loadBoolImmediate.OPCODE);
        loadBoolImmediate.scalarTag = static_cast<uint64_t>(ScalarTag::Bool);
        loadBoolImmediate.dst = static_cast<uint64_t>(this->variableIndex);
        loadBoolImmediate.immediate = static_cast<uint64_t>(this->metadata.boolLit);
        const Bytecode asBytecode = Bytecode(loadBoolImmediate);
        if (builder->pushBytecode(&asBytecode, 1).hasErr()) {
            return Error(ProgramError::OutOfMemory);
        }
    } break;
    default:
        return Error(ProgramError::CompileExpression);
    }
    return {};
}
