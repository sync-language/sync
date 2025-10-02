#include "expression.hpp"
#include "base_nodes.hpp"
#include "parser.hpp"
#include <charconv>
#include <cstring>

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

static Result<size_t, AllocErr> getOrMakeDstVarIndex(StringSlice partial, DynArray<StackVariable>* variables,
                                                     Option<size_t> dstVarIndex, Allocator alloc) noexcept {
    if (dstVarIndex.hasValue()) {
        return dstVarIndex.value();
    }

    // will use SSO, so won't fail
    String tempName = String::copyConstructSlice("%", alloc).takeValue();
    if (tempName.append(partial).hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }
    char buf[24]; // more than enough
    std::to_chars(buf, buf + sizeof(buf), variables->len());
    if (tempName.append(StringSlice(buf, std::strlen(buf))).hasErr()) {
        return Error(AllocErr::OutOfMemory);
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

Result<Expression, CompileError> sy::Expression::parse(ParseInfo* parseInfo, DynArray<StackVariable>* variables,
                                                       Option<size_t> dstVarIndex) noexcept {
    Expression expr{};

    switch (parseInfo->tokenIter.current().tag()) {
    case TokenType::TrueKeyword: {
        auto res = getOrMakeDstVarIndex("true", variables, dstVarIndex, parseInfo->alloc);
        if (res.hasErr()) {
            return Error(CompileError::createOutOfMemory());
        }
        expr.variableIndex = res.value();
        expr.tag = Expression::ExprType::BoolLit;
        expr.metadata.boolLit = true;
    } break;
    case TokenType::FalseKeyword: {
        auto res = getOrMakeDstVarIndex("false", variables, dstVarIndex, parseInfo->alloc);
        if (res.hasErr()) {
            return Error(CompileError::createOutOfMemory());
        }
        expr.variableIndex = res.value();
        expr.tag = Expression::ExprType::BoolLit;
        expr.metadata.boolLit = false;
    } break;
    default:
        break;
    }

    return expr;
}
