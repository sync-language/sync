#include "parser.hpp"

using namespace sy;

Result<Option<IFunctionLogicNode*>, CompileError>
sy::parseStatement(ParseInfo* parseInfo, DynArray<StackVariable>* localVariables, Scope* currentScope) {
    const TokenType tokenType = parseInfo->tokenIter.current().tag();
    if (tokenType == TokenType::RightBraceSymbol) {
        return Option<IFunctionLogicNode*>(); // no more nodes
    }

    (void)localVariables;
    (void)currentScope;

    switch (tokenType) {
    case TokenType::ReturnKeyword: {
    } break;
    default:
        break;
    }

    return Error(CompileError::createInvalidFunctionStatement(detail::sourceLocationFromFileLocation(
        parseInfo->tokenizer.source(), parseInfo->tokenIter.current().location())));
}
