#include "function_definition.hpp"
#include "../../../types/type_info.hpp"
#include "../../../util/assert.hpp"
#include "../parser.hpp"

using sy::Allocator;
using sy::DynArrayUnmanaged;
using sy::FunctionDefinitionNode;
using sy::ParseInfo;
using sy::Result;
using sy::StackVariable;
using sy::Type;
using sy::TypeResolutionInfo;

static Result<DynArrayUnmanaged<StackVariable>, int> parseFunctionArgs(ParseInfo* parseInfo, Allocator alloc) {
    sy_assert(parseInfo->tokenIter.current().tag() == TokenType::LeftParenthesesSymbol, "Expected left parentheses");

    (void)alloc;

    DynArrayUnmanaged<StackVariable> args;
    return args;
}

FunctionDefinitionNode::~FunctionDefinitionNode() noexcept {
    Allocator allocator = this->alloc();
    this->args.destroy(allocator);
}

Result<void, int> sy::FunctionDefinitionNode::init(ParseInfo* parseInfo, Scope* outerScope) {
    sy_assert(parseInfo->tokenIter.current().tag() == TokenType::FnKeyword,
              "Function definition node must be initialized from a fn keyword token");

    { // function name
        auto result = parseInfo->tokenIter.next();
        if (result.has_value() == false) {
            return Error(0); // idk
        }
        const Token next = result.value();
        if (next.tag() != TokenType::Identifier) {
            return Error(0); // idk
        }

        this->functionName = parseInfo->tokenIter.currentSlice();
    }

    { // arguments
        auto nextResult = parseInfo->tokenIter.next();
        if (nextResult.has_value() == false) {
            return Error(0); // idk
        }
        const Token next = nextResult.value();
        if (next.tag() != TokenType::LeftParenthesesSymbol) {
            return Error(0); // idk
        }

        auto parseResult = parseFunctionArgs(parseInfo, this->alloc());
        if (parseResult.hasErr()) {
            return Error(parseResult.takeErr());
        }
        new (&this->args) DynArrayUnmanaged<StackVariable>(parseResult.takeValue());
    }

    (void)outerScope;

    return {};
}
