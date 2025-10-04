#include "parser.hpp"

using namespace sy;

Result<Option<IFunctionStatement*>, ProgramError>
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

    return Error(
        ProgramError(SourceFileLocation(parseInfo->tokenIter.source(), parseInfo->tokenIter.current().location()),
                     ProgramError::Kind::CompileFunctionStatement));
}
