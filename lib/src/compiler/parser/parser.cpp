#include "parser.hpp"

using namespace sy;

Result<Option<IFunctionStatement*>, CompileError>
sy::parseStatement(ParseInfo* parseInfo, DynArray<StackVariable>* localVariables, Scope* currentScope) {
    const TokenType tokenType = parseInfo->tokenIter.current().tag();
    if (tokenType == TokenType::RightBraceSymbol) {
        return Option<IFunctionStatement*>(); // no more nodes
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
        parseInfo->tokenIter.source(), parseInfo->tokenIter.current().location())));
}
