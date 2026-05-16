#include "token.hpp"
#include "../../core/core_internal.h"
#include "../../util/simd.hpp"

using namespace sy;

static_assert(sizeof(Token) == sizeof(uint32_t));

StringSlice sy::tokenTypeToString(TokenType tokenType) {
    switch (tokenType) {
    case TokenType::Error:
        return "Error";
    case TokenType::EndOfFile:
        return "EndOfFile";

    case TokenType::ConstKeyword:
        return "ConstKeyword";
    case TokenType::MutKeyword:
        return "MutKeyword";
    case TokenType::ReturnKeyword:
        return "ReturnKeyword";
    case TokenType::ThrowKeyword:
        return "ThrowKeyword";
    case TokenType::TryKeyword:
        return "TryKeyword";
    case TokenType::CatchKeyword:
        return "CatchKeyword";
    case TokenType::FnKeyword:
        return "FnKeyword";
    case TokenType::PubKeyword:
        return "PubKeyword";
    case TokenType::IfKeyword:
        return "IfKeyword";
    case TokenType::ElseKeyword:
        return "ElseKeyword";
    case TokenType::SwitchKeyword:
        return "SwitchKeyword";
    case TokenType::WhileKeyword:
        return "WhileKeyword";
    case TokenType::ForKeyword:
        return "ForKeyword";
    case TokenType::InKeyword:
        return "InKeyword";
    case TokenType::BreakKeyword:
        return "BreakKeyword";
    case TokenType::ContinueKeyword:
        return "ContinueKeyword";
    case TokenType::StructKeyword:
        return "StructKeyword";
    case TokenType::EnumKeyword:
        return "EnumKeyword";
    case TokenType::DynKeyword:
        return "DynKeyword";
    case TokenType::LifetimeDynKeyword:
        return "LifetimeDynKeyword";
    case TokenType::TraitKeyword:
        return "TraitKeyword";
    case TokenType::WhereKeyword:
        return "WhereKeyword";
    case TokenType::SelfIdentifierKeyword:
        return "SelfIdentifierKeyword";
    case TokenType::SelfTypeKeyword:
        return "SelfTypeKeyword";
    case TokenType::ImplKeyword:
        return "ImplKeyword";
    case TokenType::SpecificKeyword:
        return "SpecificKeyword";
    case TokenType::SyncKeyword:
        return "SyncKeyword";
    case TokenType::TrueKeyword:
        return "TrueKeyword";
    case TokenType::FalseKeyword:
        return "FalseKeyword";
    case TokenType::NullKeyword:
        return "NullKeyword";
    case TokenType::AndKeyword:
        return "AndKeyword";
    case TokenType::OrKeyword:
        return "OrKeyword";
    case TokenType::UniqueKeyword:
        return "UniqueKeyword";
    case TokenType::SharedKeyword:
        return "SharedKeyword";
    case TokenType::WeakKeyword:
        return "WeakKeyword";
    case TokenType::AsKeyword:
        return "AsKeyword";
    case TokenType::PanicKeyword:
        return "PanicKeyword";
    case TokenType::AssertKeyword:
        return "AssertKeyword";
    case TokenType::PrintKeyword:
        return "PrintKeyword";
    case TokenType::ExternKeyword:
        return "ExternKeyword";
    case TokenType::ImportKeyword:
        return "ImportKeyword";
    case TokenType::ParallelKeyword:
        return "ParallelKeyword";
    case TokenType::AwaitKeyword:
        return "AwaitKeyword";

    case TokenType::BoolPrimitive:
        return "BoolPrimitive";
    case TokenType::I8Primitive:
        return "I8Primitive";
    case TokenType::I16Primitive:
        return "I16Primitive";
    case TokenType::I32Primitive:
        return "I32Primitive";
    case TokenType::I64Primitive:
        return "I64Primitive";
    case TokenType::U8Primitive:
        return "U8Primitive";
    case TokenType::U16Primitive:
        return "U16Primitive";
    case TokenType::U32Primitive:
        return "U32Primitive";
    case TokenType::U64Primitive:
        return "U64Primitive";
    case TokenType::USizePrimitive:
        return "USizePrimitive";
    case TokenType::F32Primitive:
        return "F32Primitive";
    case TokenType::F64Primitive:
        return "F64Primitive";
    case TokenType::CharPrimitive:
        return "CharPrimitive";
    case TokenType::StrPrimitive:
        return "StrPrimitive";
    case TokenType::StringPrimitive:
        return "StringPrimitive";
    case TokenType::TypePrimitive:
        return "TypePrimitive";
    case TokenType::TaskPrimitive:
        return "TaskPrimitive";

    case TokenType::NumberLiteral:
        return "NumberLiteral";
    case TokenType::CharLiteral:
        return "CharLiteral";
    case TokenType::StringLiteral:
        return "StringLiteral";
    case TokenType::FormatString:
        return "FormatString";

    case TokenType::Identifier:
        return "Identifier";

    case TokenType::EqualOperator:
        return "EqualOperator";
    case TokenType::AssignOperator:
        return "AssignOperator";
    case TokenType::NotEqualOperator:
        return "NotEqualOperator";
    case TokenType::ErrorUnwrapOperator:
        return "ErrorUnwrapOperator";
    case TokenType::OptionUnwrapOperator:
        return "OptionUnwrapOperator";
    // case TokenType::NotOperator: return "NotOperator";
    case TokenType::LessOrEqualOperator:
        return "LessOrEqualOperator";
    case TokenType::LessOperator:
        return "LessOperator";
    case TokenType::GreaterOrEqualOperator:
        return "GreaterOrEqualOperator";
    case TokenType::GreaterOperator:
        return "GreaterOperator";
    case TokenType::AddAssignOperator:
        return "AddAssignOperator";
    case TokenType::AddOperator:
        return "AddOperator";
    case TokenType::SubtractAssignOperator:
        return "SubtractAssignOperator";
    case TokenType::SubtractOperator:
        return "SubtractOperator";
    case TokenType::MultiplyAssignOperator:
        return "MultiplyAssignOperator";
    // case TokenType::MultiplyOperator:
    //     return "MultiplyOperator";
    case TokenType::DivideAssignOperator:
        return "DivideAssignOperator";
    case TokenType::DivideOperator:
        return "DivideOperator";
    // case TokenType::PowerAssignOperator: return "PowerAssignOperator";
    // case TokenType::PowerOperator: return "PowerOperator";
    case TokenType::ModuloAssignOperator:
        return "ModuloAssignOperator";
    case TokenType::ModuloOperator:
        return "ModuloOperator";
    case TokenType::BitshiftRightAssignOperator:
        return "BitshiftRightAssignOperator";
    case TokenType::BitshiftRightOperator:
        return "BitshiftRightOperator";
    case TokenType::BitshiftLeftAssignOperator:
        return "BitshiftLeftAssignOperator";
    case TokenType::BitshiftLeftOperator:
        return "BitshiftLeftOperator";
    case TokenType::BitAndAssignOperator:
        return "BitAndAssignOperator";
    // case TokenType::BitAndOperator: return "BitAndOperator";
    case TokenType::BitOrAssignOperator:
        return "BitOrAssignOperator";
    case TokenType::BitOrOperator:
        return "BitOrOperator";
    case TokenType::BitXorAssignOperator:
        return "BitXorAssignOperator";
    case TokenType::BitXorOperator:
        return "BitXorOperator";
    case TokenType::BitNotAssignOperator:
        return "BitNotAssignOperator";
    case TokenType::BitNotOperator:
        return "BitNotOperator";

    case TokenType::LeftParenthesesSymbol:
        return "LeftParenthesesSymbol";
    case TokenType::RightParenthesesSymbol:
        return "RightParenthesesSymbol";
    case TokenType::LeftBracketSymbol:
        return "LeftBracketSymbol";
    case TokenType::RightBracketSymbol:
        return "RightBracketSymbol";
    case TokenType::LeftBraceSymbol:
        return "LeftBraceSymbol";
    case TokenType::RightBraceSymbol:
        return "RightBraceSymbol";
    case TokenType::ColonSymbol:
        return "ColonSymbol";
    case TokenType::SemicolonSymbol:
        return "SemicolonSymbol";
    case TokenType::DotSymbol:
        return "DotSymbol";
    case TokenType::CommaSymbol:
        return "CommaSymbol";
    case TokenType::OptionalSymbol:
        return "OptionalSymbol";
    // case TokenType::ErrorSymbol: return "ErrorSymbol";
    // case TokenType::ImmutableReferenceSymbol: return "ImmutableReferenceSymbol";
    case TokenType::MutableReferenceSymbol:
        return "MutableReferenceSymbol";
    case TokenType::AmpersandSymbol:
        return "AmpersandSymbol";
    case TokenType::ExclamationSymbol:
        return "ExclamationSymbol";
    case TokenType::AsteriskSymbol:
        return "AsteriskSymbol";

    case TokenType::LifetimePointer:
        return "LifetimePointer";
    case TokenType::ConcreteLifetime:
        return "ConcreteLifetime";
    case TokenType::Slice:
        return "Slice";
    case TokenType::SliceLifetime:
        return "SliceLifetime";
    default:
        sy_assert(false, "Invalid token");
    }
    sync_unreachable();
}

// no need to include whole header

constexpr static bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

constexpr static bool isAlpha(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }

constexpr static bool isNumeric(char c) { return (c >= '0' && c <= '9'); }

constexpr static bool isSeparator(char c) {
    // @ and ' for lifetime stuff

    return c == ';' || c == ',' || c == ':' || c == '(' || c == ')' || c == '[' || c == ']' ||
           c == '{' || c == '}' || c == '@' || c == '\'';
}

constexpr static bool isAlphaNumeric(char c) { return isAlpha(c) || isNumeric(c); }

constexpr static bool isAlphaNumericOrUnderscore(char c) { return isAlphaNumeric(c) || c == '_'; }

/// @return -1 if reached end of source
static uint32_t nonWhitespaceStartFrom(const StringSlice source, const uint32_t start,
                                       uint32_t* lineNumber) {

    const char* strData = source.data();
    const uint32_t len = static_cast<uint32_t>(source.len());
    for (uint32_t i = start; i < len; i++) {
        if (strData[i] == '\n')
            (*lineNumber) += 1;
        if (!isSpace(strData[i]))
            return i;
    }
    return static_cast<uint32_t>(-1);
}

// /// @return -1 if reached end of source
// static uint32_t firstWhitespaceStartFrom(const StringSlice source, const uint32_t start) {
//     const char* strData = source.data();
//     const uint32_t len = static_cast<uint32_t>(source.len());
//     for(uint32_t i = start; i < len; i++) {
//         if(isSpace(strData[i])) return i;
//     }
//     return static_cast<uint32_t>(-1);
// }

// static uint32_t endOfAlphaNumeric(const StringSlice source, const uint32_t start) {
//     const char* strData = source.data();
//     const uint32_t len = static_cast<uint32_t>(source.len());
//     for(uint32_t i = start; i < len; i++) {
//         if(!isAlphaNumeric(strData[i])) return i;
//     }
//     return static_cast<uint32_t>(-1);
// }

static uint32_t endOfAlphaNumericOrUnderscore(const StringSlice source, const uint32_t start) {
    const char* strData = source.data();
    const uint32_t len = static_cast<uint32_t>(source.len());
    uint32_t i = start;
    for (; i < len; i++) {
        if (!isAlphaNumericOrUnderscore(strData[i]))
            break;
    }
    return i;
}

static bool sliceFoundAtUnchecked(const StringSlice source, const StringSlice toFind,
                                  const uint32_t start) {
    const uint32_t toFindLen = static_cast<uint32_t>(toFind.len());

    for (uint32_t i = 0; i < toFindLen; i++) {
        if (source[start + i] != toFind[i])
            return false;
    }
    return true;
}

static bool validCharForNumberLiteral(char c) {
    return isNumeric(c) || c == '.' || c == 'x' // hex
           || c == 'X'                          // hex
           || (c >= 'a' && c <= 'f')            // binary (b) and hex
           || (c >= 'A' && c <= 'F');           // binary (B) and hex
}

#pragma region Keywords

static std::tuple<Token, uint32_t>
extractKeywordOrIdentifier(const StringSlice source, const uint32_t remainingSourceLen,
                           const uint32_t remainingPossibleTokenLen, const uint32_t start,
                           const TokenType possibleTokenType) {
    const bool onlyCharsLeft = remainingSourceLen == remainingPossibleTokenLen;
    if (onlyCharsLeft) {
        return std::make_tuple(Token(possibleTokenType, start - 1),
                               start + remainingPossibleTokenLen);
    }

    const bool whitespaceAfter = isSpace(source[start + remainingPossibleTokenLen]);
    const bool separatorAfter = isSeparator(source[start + remainingPossibleTokenLen]);
    if (whitespaceAfter || separatorAfter) {
        return std::make_tuple(Token(possibleTokenType, start - 1),
                               start + remainingPossibleTokenLen);
    }

    const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + remainingPossibleTokenLen);
    return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
}

static std::tuple<Token, uint32_t>
parseIfInImplImportAndSignedIntegerTypesOrIdentifier(const StringSlice source,
                                                     const uint32_t start);

static std::tuple<Token, uint32_t> parseElseEnumExternOrIdentifier(const StringSlice source,
                                                                   const uint32_t start);

static std::tuple<Token, uint32_t> parseUnsignedIntegerTypesOrIdentifier(const StringSlice source,
                                                                         const uint32_t start);

static std::tuple<Token, uint32_t> parseBoolTypeBreakOrIdentifier(const StringSlice source,
                                                                  const uint32_t start);

static std::tuple<Token, uint32_t>
parseCharConstContinueComptimeCatchOrIdentifier(const StringSlice source, const uint32_t start);

static std::tuple<Token, uint32_t> parseMutOrIdentifier(const StringSlice source,
                                                        const uint32_t start);

static std::tuple<Token, uint32_t> parseReturnOrIdentifier(const StringSlice source,
                                                           const uint32_t start);

static std::tuple<Token, uint32_t>
parseStructSyncStrSwitchSpecificOrIdentifier(const StringSlice source, const uint32_t start);

static std::tuple<Token, uint32_t> parseStringSharedSetSelfOrIdentifier(const StringSlice source,
                                                                        const uint32_t start);

static std::tuple<Token, uint32_t>
parseFloatTypesForFalseFnFormatstrOrIdentifier(const StringSlice source, const uint32_t start);

static std::tuple<Token, uint32_t> parseTrueThrowTraitTryOrIdentifier(const StringSlice source,
                                                                      const uint32_t start);

static std::tuple<Token, uint32_t> parsePubPanicPrintParallelOrIdentifier(const StringSlice source,
                                                                          const uint32_t start);

static std::tuple<Token, uint32_t> parseUniqueOrIdentifier(const StringSlice source,
                                                           const uint32_t start);

static std::tuple<Token, uint32_t> parseWeakOrIdentifier(const StringSlice source,
                                                         const uint32_t start);

static std::tuple<Token, uint32_t> parseAndAsAssertAwaitOrIdentifier(const StringSlice source,
                                                                     const uint32_t start);

static std::tuple<Token, uint32_t> parseOrOrIdentifier(const StringSlice source,
                                                       const uint32_t start);

static std::tuple<Token, uint32_t> parseNullOrIdentifier(const StringSlice source,
                                                         const uint32_t start);

static std::tuple<Token, uint32_t> parseDynOrIdentifier(const StringSlice source,
                                                        const uint32_t start);

static std::tuple<Token, uint32_t> parseWhileWhereOrIdentifier(const StringSlice source,
                                                               const uint32_t start);

static std::tuple<Token, uint32_t> parseTypeTaskOrIdentifier(const StringSlice source,
                                                             const uint32_t start);

static std::tuple<Token, uint32_t> parseListOrIdentifier(const StringSlice source,
                                                         const uint32_t start);

static std::tuple<Token, uint32_t> parseMapOrIdentifier(const StringSlice source,
                                                        const uint32_t start);

static std::tuple<Token, uint32_t> parseIdentifier(const StringSlice source, const uint32_t start);

#pragma endregion

// Operators and symbols will not bother checking if the characters after them
// are whitespace, separators, or alphanumeric. They will parse the token and
// set the end to be right after regardless of the following character.
#pragma region Operators_Symbols

static std::tuple<Token, uint32_t> parseLessOrBitshiftLeft(const StringSlice source,
                                                           const uint32_t start) {
    sy_assert(source[start - 1] == '<', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::LessOperator, start - 1),
                               static_cast<uint32_t>(-1));
    }

    if (remainingSourceLen >= 2) {
        if (sliceFoundAtUnchecked(source, "<=", start)) {
            return std::make_tuple(Token(TokenType::BitshiftLeftAssignOperator, start - 1),
                                   start + 2);
        }
    }

    if (source[start] == '<') {
        return std::make_tuple(Token(TokenType::BitshiftLeftOperator, start - 1), start + 1);
    }

    if (source[start] == '=') {
        return std::make_tuple(Token(TokenType::LessOrEqualOperator, start - 1), start + 1);
    }

    return std::make_tuple(Token(TokenType::LessOperator, start - 1), start);
}

static std::tuple<Token, uint32_t> parseGreaterOrBitshiftRight(const StringSlice source,
                                                               const uint32_t start) {
    sy_assert(source[start - 1] == '>', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::GreaterOperator, start - 1),
                               static_cast<uint32_t>(-1));
    }

    if (remainingSourceLen >= 2) {
        if (sliceFoundAtUnchecked(source, ">=", start)) {
            return std::make_tuple(Token(TokenType::BitshiftRightAssignOperator, start - 1),
                                   start + 2);
        }
    }

    if (source[start] == '>') {
        return std::make_tuple(Token(TokenType::BitshiftRightOperator, start - 1), start + 1);
    }

    if (source[start] == '=') {
        return std::make_tuple(Token(TokenType::GreaterOrEqualOperator, start - 1), start + 1);
    }

    return std::make_tuple(Token(TokenType::GreaterOperator, start - 1), start);
}

static std::tuple<Token, uint32_t> parseEqualsOrAssign(const StringSlice source,
                                                       const uint32_t start) {
    sy_assert(source[start - 1] == '=', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::AssignOperator, start - 1),
                               static_cast<uint32_t>(-1));
    }

    if (source[start] == '=') {
        return std::make_tuple(Token(TokenType::EqualOperator, start - 1), start + 1);
    }

    return std::make_tuple(Token(TokenType::AssignOperator, start - 1), start);
}

/// Works for the following operators
///
/// - \+
/// - *
/// - /
/// - %
/// - |
/// - ^
/// - ~
/// - ! (Just as exclamation)
template <char startChar, TokenType nonAssignType, TokenType assignType>
static std::tuple<Token, uint32_t> parseMathOperatorWithAssign(const StringSlice source,
                                                               const uint32_t start) {
    sy_assert(source[start - 1] == startChar, "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(nonAssignType, start - 1), static_cast<uint32_t>(-1));
    }

    if (source[start] == '=') {
        return std::make_tuple(Token(assignType, start - 1), start + 1);
    }

    return std::make_tuple(Token(nonAssignType, start - 1), start);
}

static std::tuple<Token, uint32_t> parseDotOperatorsAndSymbol(const StringSlice source,
                                                              const uint32_t start) {
    sy_assert(source[start - 1] == '.', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::DotSymbol, start - 1), static_cast<uint32_t>(-1));
    }

    if (source[start] == '?') {
        return std::make_tuple(Token(TokenType::OptionUnwrapOperator, start - 1), start + 1);
    }

    if (source[start] == '!') {
        return std::make_tuple(Token(TokenType::ErrorUnwrapOperator, start - 1), start + 1);
    }

    return std::make_tuple(Token(TokenType::DotSymbol, start - 1), start);
}

static std::tuple<Token, uint32_t> parseAmpersandOrMutableReference(const StringSlice source,
                                                                    const uint32_t start) {
    sy_assert(source[start - 1] == '&', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen < 3) {
        return std::make_tuple(Token(TokenType::AmpersandSymbol, start - 1), start);
    }

    if (sliceFoundAtUnchecked(source, "mut", start)) {
        if (remainingSourceLen == 3) {
            return std::make_tuple(Token(TokenType::MutableReferenceSymbol, start - 1),
                                   static_cast<uint32_t>(-1));
        }

        if (!isAlphaNumericOrUnderscore(source[start + 3])) {
            return std::make_tuple(Token(TokenType::MutableReferenceSymbol, start - 1), start + 3);
        }
    }

    return std::make_tuple(Token(TokenType::AmpersandSymbol, start - 1), start);
}

static std::tuple<Token, uint32_t> parseMultiplyOrPointer(const StringSlice source,
                                                          const uint32_t start) {
    sy_assert(source[start - 1] == '*', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::AsteriskSymbol, start - 1),
                               static_cast<uint32_t>(-1));
    }

    if (source[start] == '=') {
        return std::make_tuple(Token(TokenType::MultiplyAssignOperator, start - 1), start + 1);
    } else if (source[start] == '\'') {
        return std::make_tuple(Token(TokenType::LifetimePointer, start - 1), start + 1);
    }

    return std::make_tuple(Token(TokenType::AsteriskSymbol, start - 1), start);
}

static std::tuple<Token, uint32_t> parseConcreteLifetime(const StringSlice source,
                                                         const uint32_t start) {
    sy_assert(source[start - 1] == '@', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;
    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::Error, start - 1), static_cast<uint32_t>(-1));
    }

    if (source[start] == '\'') {
        return std::make_tuple(Token(TokenType::ConcreteLifetime, start - 1), start + 1);
    }

    return std::make_tuple(Token(TokenType::Error, start - 1), start);
}

static std::tuple<Token, uint32_t> parseSliceOrLeftBracket(const StringSlice source,
                                                           const uint32_t start) {
    sy_assert(source[start - 1] == '[', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::LeftBracketSymbol, start - 1),
                               static_cast<uint32_t>(-1));
    }

    if (remainingSourceLen >= 2) {
        if (sliceFoundAtUnchecked(source, "]\'", start)) {
            return std::make_tuple(Token(TokenType::SliceLifetime, start - 1), start + 2);
        }
    }

    if (source[start] == ']') {
        return std::make_tuple(Token(TokenType::Slice, start - 1), start + 1);
    }

    return std::make_tuple(Token(TokenType::LeftBracketSymbol, start - 1), start);
}

#pragma endregion

static std::tuple<Token, uint32_t> parseSubtractOrNegativeNumberLiteral(const StringSlice source,
                                                                        const uint32_t start) {
    sy_assert(source[start - 1] == '-', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::SubtractOperator, start - 1),
                               static_cast<uint32_t>(-1));
    }

    if (source[start] == '=') {
        return std::make_tuple(Token(TokenType::SubtractAssignOperator, start - 1), start + 1);
    }

    if (isNumeric(source[start])) {
        // ignore multiple decimal places
        uint32_t i = start + 1;
        for (; i < (start + remainingSourceLen); i++) {
            if (!validCharForNumberLiteral(source[i])) {
                break;
            }
        }
        return std::make_tuple(Token(TokenType::NumberLiteral, start - 1), i);
    }

    return std::make_tuple(Token(TokenType::SubtractOperator, start - 1), start);
}

static std::tuple<Token, uint32_t> parseCharLiteral(const StringSlice source,
                                                    const uint32_t start) {
    sy_assert(source[start - 1] == '\'', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::Error, start - 1), static_cast<uint32_t>(-1));
    }

    if (source[start] == '\'') {
        // A char literal of '' is invalid
        return std::make_tuple(Token(TokenType::Error, start - 1), start + 1);
    }

    uint32_t i = start;
    bool didFind = false;
    for (; i < (remainingSourceLen + start); i++) {
        const char c = source[i];
        const char before = source[i - 1];

        if (c == '\n') {
            // Multline char literals? Probably nonsense
            return std::make_tuple(Token(TokenType::Error, i), static_cast<uint32_t>(-1));
        }
        if (c == '\'' && before != '\\') {
            didFind = true;
            break;
        }
    }

    if (didFind) {
        return std::make_tuple(Token(TokenType::CharLiteral, start - 1), i + 1);
    } else {
        return std::make_tuple(Token(TokenType::Error, start - 1), static_cast<uint32_t>(-1));
    }
}

static std::tuple<Token, uint32_t> parseStringLiteral(const StringSlice source,
                                                      const uint32_t start) {
    sy_assert(source[start - 1] == '\"', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::Error, start - 1), static_cast<uint32_t>(-1));
    }

    // A str literal of "" is valid, as that's just an empty string

    uint32_t i = start;
    bool didFind = false;
    for (; i < (remainingSourceLen + start); i++) {
        const char c = source[i];
        const char before = source[i - 1];
        if (c == '\n') {
            // TODO figure out how to do multiline string literals
            return std::make_tuple(Token(TokenType::Error, i), static_cast<uint32_t>(-1));
        }
        if (c == '\"' && before != '\\') {
            didFind = true;
            break;
        }
    }

    if (didFind) {
        return std::make_tuple(Token(TokenType::StringLiteral, start - 1), i + 1);
    } else {
        return std::make_tuple(Token(TokenType::Error, start - 1), static_cast<uint32_t>(-1));
    }
}

static std::tuple<Token, uint32_t> parseNumberLiteral(const StringSlice source,
                                                      const uint32_t start) {
    sy_assert(isNumeric(source[start - 1]), "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::NumberLiteral, start - 1),
                               static_cast<uint32_t>(-1));
    }

    // ignore multiple decimal places
    uint32_t i = start;
    for (; i < (start + remainingSourceLen); i++) {
        if (!validCharForNumberLiteral(source[i])) {
            break;
        }
    }
    return std::make_tuple(Token(TokenType::NumberLiteral, start - 1), i);
}

std::tuple<Token, uint32_t> sy::Token::parseToken(const StringSlice source, const uint32_t start,
                                                  uint32_t* lineNumber) {
    const uint32_t nonWhitespaceStart = nonWhitespaceStartFrom(source, start, lineNumber);
    if (nonWhitespaceStart == static_cast<uint32_t>(-1)) {
        return std::make_tuple(Token(TokenType::EndOfFile, nonWhitespaceStart), 0);
    }

    switch (source[nonWhitespaceStart]) {
    // For tokens with no possible variants and are 1 character, this works
    // Semicolon is on most lines of code
    case ';':
        return std::make_tuple(Token(TokenType::SemicolonSymbol, nonWhitespaceStart),
                               nonWhitespaceStart + 1);
    case ',':
        return std::make_tuple(Token(TokenType::CommaSymbol, nonWhitespaceStart),
                               nonWhitespaceStart + 1);
    case '{':
        return std::make_tuple(Token(TokenType::LeftBraceSymbol, nonWhitespaceStart),
                               nonWhitespaceStart + 1);
    case '}':
        return std::make_tuple(Token(TokenType::RightBraceSymbol, nonWhitespaceStart),
                               nonWhitespaceStart + 1);
    case '(':
        return std::make_tuple(Token(TokenType::LeftParenthesesSymbol, nonWhitespaceStart),
                               nonWhitespaceStart + 1);
    case ')':
        return std::make_tuple(Token(TokenType::RightParenthesesSymbol, nonWhitespaceStart),
                               nonWhitespaceStart + 1);
    // case '[':
    //     return std::make_tuple(Token(TokenType::LeftBracketSymbol, nonWhitespaceStart),
    //     nonWhitespaceStart + 1);
    case ']':
        return std::make_tuple(Token(TokenType::RightBracketSymbol, nonWhitespaceStart),
                               nonWhitespaceStart + 1);
    case ':':
        return std::make_tuple(Token(TokenType::ColonSymbol, nonWhitespaceStart),
                               nonWhitespaceStart + 1);
    case '?':
        return std::make_tuple(Token(TokenType::OptionalSymbol, nonWhitespaceStart),
                               nonWhitespaceStart + 1);

    case '_': { // is definitely an identifier
        // already did first char
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, nonWhitespaceStart + 1);
        return std::make_tuple(Token(TokenType::Identifier, nonWhitespaceStart), end);
    };

    default:
        break;
    }

    // if, impl, and import will definitely be used a lot, along with probably the signed integer
    // types so checking those first is good
    if (source[nonWhitespaceStart] == 'i') {
        return parseIfInImplImportAndSignedIntegerTypesOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // else will probably be used a lot as well
    if (source[nonWhitespaceStart] == 'e') {
        return parseElseEnumExternOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // const should be extremely used. char and continue exist too.
    if (source[nonWhitespaceStart] == 'c') {
        return parseCharConstContinueComptimeCatchOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // mut also should be extremely used
    if (source[nonWhitespaceStart] == 'm') {
        return parseMutOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // return
    if (source[nonWhitespaceStart] == 'r') {
        return parseReturnOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // unsigned integer types will also get used quite a lot
    if (source[nonWhitespaceStart] == 'u') {
        return parseUnsignedIntegerTypesOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // bool should be common. break exists too.
    if (source[nonWhitespaceStart] == 'b') {
        return parseBoolTypeBreakOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // struct, sync (lowercase), str, switch
    if (source[nonWhitespaceStart] == 's') {
        return parseStructSyncStrSwitchSpecificOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // capital S (String, Shared, Set, Self)
    if (source[nonWhitespaceStart] == 'S') {
        return parseStringSharedSetSelfOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // float types, for, false, fn, f-string
    if (source[nonWhitespaceStart] == 'f') {
        return parseFloatTypesForFalseFnFormatstrOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // true
    if (source[nonWhitespaceStart] == 't') {
        return parseTrueThrowTraitTryOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // pub, panic, print, parallel
    if (source[nonWhitespaceStart] == 'p') {
        return parsePubPanicPrintParallelOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // Unique
    if (source[nonWhitespaceStart] == 'U') {
        return parseUniqueOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // Weak
    if (source[nonWhitespaceStart] == 'W') {
        return parseWeakOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // and, as, assert, await
    if (source[nonWhitespaceStart] == 'a') {
        return parseAndAsAssertAwaitOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // or
    if (source[nonWhitespaceStart] == 'o') {
        return parseOrOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // null
    if (source[nonWhitespaceStart] == 'n') {
        return parseNullOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // dyn
    if (source[nonWhitespaceStart] == 'd') {
        return parseDynOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // while, where
    if (source[nonWhitespaceStart] == 'w') {
        return parseWhileWhereOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // Type, Task
    if (source[nonWhitespaceStart] == 'T') {
        return parseTypeTaskOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // List
    if (source[nonWhitespaceStart] == 'L') {
        return parseListOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // Map
    if (source[nonWhitespaceStart] == 'M') {
        return parseMapOrIdentifier(source, nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '.') {
        return parseDotOperatorsAndSymbol(source, nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '*') {
        return parseMultiplyOrPointer(source, nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '&') {
        return parseAmpersandOrMutableReference(source, nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '[') {
        return parseSliceOrLeftBracket(source, nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '@') {
        return parseConcreteLifetime(source, nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '<') {
        return parseLessOrBitshiftLeft(source, nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '>') {
        return parseGreaterOrBitshiftRight(source, nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '=') {
        return parseEqualsOrAssign(source, nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '+') {
        return parseMathOperatorWithAssign<'+', TokenType::AddOperator,
                                           TokenType::AddAssignOperator>(source,
                                                                         nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '/') {
        return parseMathOperatorWithAssign<'/', TokenType::DivideOperator,
                                           TokenType::DivideAssignOperator>(source,
                                                                            nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '%') {
        return parseMathOperatorWithAssign<'%', TokenType::ModuloOperator,
                                           TokenType::ModuloAssignOperator>(source,
                                                                            nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '|') {
        return parseMathOperatorWithAssign<'|', TokenType::BitOrOperator,
                                           TokenType::BitOrAssignOperator>(source,
                                                                           nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '^') {
        return parseMathOperatorWithAssign<'^', TokenType::BitXorOperator,
                                           TokenType::BitXorAssignOperator>(source,
                                                                            nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '~') {
        return parseMathOperatorWithAssign<'~', TokenType::BitNotOperator,
                                           TokenType::BitNotAssignOperator>(source,
                                                                            nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '!') {
        return parseMathOperatorWithAssign<'!', TokenType::ExclamationSymbol,
                                           TokenType::NotEqualOperator>(source,
                                                                        nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '-') {
        return parseSubtractOrNegativeNumberLiteral(source, nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '\"') {
        return parseStringLiteral(source, nonWhitespaceStart + 1);
    }

    if (source[nonWhitespaceStart] == '\'') {
        return parseCharLiteral(source, nonWhitespaceStart + 1);
    }

    if (isNumeric(source[nonWhitespaceStart])) {
        return parseNumberLiteral(source, nonWhitespaceStart + 1);
    }

    if (isAlpha(source[nonWhitespaceStart])) {
        return parseIdentifier(source, nonWhitespaceStart + 1);
    }

    return std::make_tuple(Token(TokenType::Error, nonWhitespaceStart), nonWhitespaceStart + 1);
}

static std::tuple<Token, uint32_t>
parseIfInImplImportAndSignedIntegerTypesOrIdentifier(const StringSlice source,
                                                     const uint32_t start) {
    sy_assert(source[start - 1] == 'i', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        // Literally the identifier "i". Conveniently this is common for iterators.
        return std::make_tuple(Token(TokenType::Identifier, start - 1), start);
    }

    if (source[start] == 'f') {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 1, start,
                                          TokenType::IfKeyword);
    }
    if (source[start] == 'n') {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 1, start,
                                          TokenType::InKeyword);
    }
    if (source[start] == '8') {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 1, start,
                                          TokenType::I8Primitive);
    }

    if (remainingSourceLen >= 2) { // cannot fit i64, i32, or i16. Must be if, i8, or an identifier
        // likely used the most
        // TODO get actual metrics for this
        if (sliceFoundAtUnchecked(source, "32", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::I32Primitive);
        }

        // 64 bit signed integer probably used less than 32 bit signed
        if (sliceFoundAtUnchecked(source, "64", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::I64Primitive);
        }

        // 16 bit integers probably used the least
        if (sliceFoundAtUnchecked(source, "16", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::I16Primitive);
        }
    }

    if (remainingSourceLen >= 3) {
        if (sliceFoundAtUnchecked(source, "mpl", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start,
                                              TokenType::ImplKeyword);
        }
    }

    if (remainingSourceLen >= 5) {
        if (sliceFoundAtUnchecked(source, "mport", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 5, start,
                                              TokenType::ImportKeyword);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseElseEnumExternOrIdentifier(const StringSlice source,
                                                                   const uint32_t start) {
    sy_assert(source[start - 1] == 'e', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen >= 5) {
        if (sliceFoundAtUnchecked(source, "xtern", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 5, start,
                                              TokenType::ExternKeyword);
        }
    }

    if (remainingSourceLen >= 3) {
        if (sliceFoundAtUnchecked(source, "lse", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start,
                                              TokenType::ElseKeyword);
        }

        if (sliceFoundAtUnchecked(source, "num", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start,
                                              TokenType::EnumKeyword);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseUnsignedIntegerTypesOrIdentifier(const StringSlice source,
                                                                         const uint32_t start) {
    sy_assert(source[start - 1] == 'u', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        // Identifier "u"
        return std::make_tuple(Token(TokenType::Identifier, start - 1), start);
    }

    if (source[start] == '8') {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 1, start,
                                          TokenType::U8Primitive);
    }

    if (remainingSourceLen >= 2) { // cannot fit u64, u32, or u16. Must be if, i8, or an identifier
        // While 32 bit signed is probably more popular than 64 bit signed, 64 bit unsigned is
        // probably more popular
        // TODO get actual metrics for this
        if (sliceFoundAtUnchecked(source, "64", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::U64Primitive);
        }

        if (sliceFoundAtUnchecked(source, "32", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::U32Primitive);
        }

        // 16 bit integers probably used the least
        if (sliceFoundAtUnchecked(source, "16", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::U16Primitive);
        }
    }

    if (remainingSourceLen >= 4) {
        if (sliceFoundAtUnchecked(source, "size", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start,
                                              TokenType::USizePrimitive);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseBoolTypeBreakOrIdentifier(const StringSlice source,
                                                                  const uint32_t start) {
    sy_assert(source[start - 1] == 'b', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen < 3) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "ool", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start,
                                          TokenType::BoolPrimitive);
    }

    if (remainingSourceLen >= 4) {
        if (sliceFoundAtUnchecked(source, "reak", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start,
                                              TokenType::BreakKeyword);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t>
parseCharConstContinueComptimeCatchOrIdentifier(const StringSlice source, const uint32_t start) {
    sy_assert(source[start - 1] == 'c', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen < 3) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (remainingSourceLen >= 7) {
        if (sliceFoundAtUnchecked(source, "ontinue", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 7, start,
                                              TokenType::ContinueKeyword);
        }
        if (sliceFoundAtUnchecked(source, "omptime", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 7, start,
                                              TokenType::ComptimeKeyword);
        }
    }

    if (remainingSourceLen >= 4) {
        if (sliceFoundAtUnchecked(source, "onst", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start,
                                              TokenType::ConstKeyword);
        }
        if (sliceFoundAtUnchecked(source, "atch", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start,
                                              TokenType::CatchKeyword);
        }
    }

    if (sliceFoundAtUnchecked(source, "har", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start,
                                          TokenType::CharPrimitive);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseMutOrIdentifier(const StringSlice source,
                                                        const uint32_t start) {
    sy_assert(source[start - 1] == 'm', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen < 2) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "ut", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                          TokenType::MutKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseReturnOrIdentifier(const StringSlice source,
                                                           const uint32_t start) {
    sy_assert(source[start - 1] == 'r', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen < 5) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "eturn", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 5, start,
                                          TokenType::ReturnKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t>
parseStructSyncStrSwitchSpecificOrIdentifier(const StringSlice source, const uint32_t start) {
    sy_assert(source[start - 1] == 's', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen < 2) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    // struct starts with str so this must be done first
    if (remainingSourceLen >= 7) {
        if (sliceFoundAtUnchecked(source, "pecific", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 7, start,
                                              TokenType::SpecificKeyword);
        }
    }

    // struct starts with str so this must be done first
    if (remainingSourceLen >= 5) {
        if (sliceFoundAtUnchecked(source, "truct", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 5, start,
                                              TokenType::StructKeyword);
        }
        if (sliceFoundAtUnchecked(source, "witch", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 5, start,
                                              TokenType::SwitchKeyword);
        }
    }

    if (sliceFoundAtUnchecked(source, "tr", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                          TokenType::StrPrimitive);
    }

    if (remainingSourceLen >= 3) {
        if (sliceFoundAtUnchecked(source, "ync", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start,
                                              TokenType::SyncKeyword);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseStringSharedSetSelfOrIdentifier(const StringSlice source,
                                                                        const uint32_t start) {
    sy_assert(source[start - 1] == 'S', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen >= 2) {
        if (sliceFoundAtUnchecked(source, "et", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::SetPrimitive);
        }
    }

    if (remainingSourceLen >= 3) {
        if (sliceFoundAtUnchecked(source, "elf", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start,
                                              TokenType::SelfKeyword);
        }
    }

    if (remainingSourceLen < 5) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "tring", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 5, start,
                                          TokenType::StringPrimitive);
    }
    if (sliceFoundAtUnchecked(source, "hared", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 5, start,
                                          TokenType::SharedKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t>
parseFloatTypesForFalseFnFormatstrOrIdentifier(const StringSlice source, const uint32_t start) {
    sy_assert(source[start - 1] == 'f', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::Identifier, start - 1), start + 1);
    }

    if (source[start] == '\"') {
        // A str literal of "" is valid, as that's just an empty string

        uint32_t i = start + 1;
        bool didFind = false;
        for (; i < (remainingSourceLen + start); i++) {
            const char c = source[i];
            const char before = source[i - 1];
            // TODO is this right?
            if (c == '\n') {
                // TODO figure out how to do multiline string literals
                return std::make_tuple(Token(TokenType::Error, i), static_cast<uint32_t>(-1));
            }
            if (c == '\"' && before != '\\') {
                didFind = true;
                break;
            }
        }

        if (didFind) {
            return std::make_tuple(Token(TokenType::FormatString, start - 1), i + 1);
        } else {
            return std::make_tuple(Token(TokenType::Error, start - 1), static_cast<uint32_t>(-1));
        }
    }

    if (source[start] == 'n') {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 1, start,
                                          TokenType::FnKeyword);
    }

    if (remainingSourceLen >= 2) {
        if (sliceFoundAtUnchecked(source, "or", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::ForKeyword);
        }

        // prefer 64 bit floats to 32 bit floats for accuracy
        if (sliceFoundAtUnchecked(source, "64", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::F64Primitive);
        }

        if (sliceFoundAtUnchecked(source, "32", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::F32Primitive);
        }
    }

    if (remainingSourceLen >= 4) {
        if (sliceFoundAtUnchecked(source, "alse", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start,
                                              TokenType::FalseKeyword);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseTrueThrowTraitTryOrIdentifier(const StringSlice source,
                                                                      const uint32_t start) {
    sy_assert(source[start - 1] == 't', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::Identifier, start - 1), start + 1);
    }

    if (remainingSourceLen >= 4) {
        if (sliceFoundAtUnchecked(source, "hrow", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start,
                                              TokenType::ThrowKeyword);
        }
        if (sliceFoundAtUnchecked(source, "rait", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start,
                                              TokenType::TraitKeyword);
        }
    }

    if (remainingSourceLen >= 3) {
        if (sliceFoundAtUnchecked(source, "rue", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start,
                                              TokenType::TrueKeyword);
        }
    }

    if (remainingSourceLen >= 2) {
        if (sliceFoundAtUnchecked(source, "ry", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::TryKeyword);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parsePubPanicPrintParallelOrIdentifier(const StringSlice source,
                                                                          const uint32_t start) {
    sy_assert(source[start - 1] == 'p', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen >= 7) {
        if (sliceFoundAtUnchecked(source, "arallel", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 7, start,
                                              TokenType::ParallelKeyword);
        }
    }

    if (remainingSourceLen >= 4) {
        if (sliceFoundAtUnchecked(source, "rint", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start,
                                              TokenType::PrintKeyword);
        }
        if (sliceFoundAtUnchecked(source, "anic", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start,
                                              TokenType::PanicKeyword);
        }
    }

    if (remainingSourceLen >= 2) {
        if (sliceFoundAtUnchecked(source, "ub", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::PubKeyword);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseUniqueOrIdentifier(const StringSlice source,
                                                           const uint32_t start) {
    sy_assert(source[start - 1] == 'U', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen < 5) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "nique", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 5, start,
                                          TokenType::UniqueKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseWeakOrIdentifier(const StringSlice source,
                                                         const uint32_t start) {
    sy_assert(source[start - 1] == 'W', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen < 3) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "eak", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start,
                                          TokenType::WeakKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseAndAsAssertAwaitOrIdentifier(const StringSlice source,
                                                                     const uint32_t start) {
    sy_assert(source[start - 1] == 'a', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen >= 5) {
        if (sliceFoundAtUnchecked(source, "ssert", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 5, start,
                                              TokenType::AssertKeyword);
        }
    }

    if (remainingSourceLen >= 4) {
        if (sliceFoundAtUnchecked(source, "wait", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start,
                                              TokenType::AwaitKeyword);
        }
    }

    if (remainingSourceLen >= 2) {
        if (sliceFoundAtUnchecked(source, "nd", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::AndKeyword);
        }
    }

    if (remainingSourceLen >= 1) {
        if (sliceFoundAtUnchecked(source, "s", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 1, start,
                                              TokenType::AsKeyword);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseOrOrIdentifier(const StringSlice source,
                                                       const uint32_t start) {
    sy_assert(source[start - 1] == 'o', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::Identifier, start - 1), static_cast<uint32_t>(-1));
    }

    if (source[start] == 'r') {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 1, start,
                                          TokenType::OrKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseNullOrIdentifier(const StringSlice source,
                                                         const uint32_t start) {
    sy_assert(source[start - 1] == 'n', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen < 2) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "ull", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start,
                                          TokenType::NullKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseDynOrIdentifier(const StringSlice source,
                                                        const uint32_t start) {
    sy_assert(source[start - 1] == 'd', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen >= 3) {
        if (sliceFoundAtUnchecked(source, "yn\'", start)) {
            return std::make_tuple(Token(TokenType::LifetimeDynKeyword, start - 1), start + 3);
        }
    }

    if (remainingSourceLen >= 2) {
        if (sliceFoundAtUnchecked(source, "yn", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::DynKeyword);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseWhileWhereOrIdentifier(const StringSlice source,
                                                               const uint32_t start) {
    sy_assert(source[start - 1] == 'w', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen >= 4) {
        if (sliceFoundAtUnchecked(source, "hile", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start,
                                              TokenType::WhileKeyword);
        }

        if (sliceFoundAtUnchecked(source, "here", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start,
                                              TokenType::WhereKeyword);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseTypeTaskOrIdentifier(const StringSlice source,
                                                             const uint32_t start) {
    sy_assert(source[start - 1] == 'T', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen >= 3) {
        if (sliceFoundAtUnchecked(source, "ype", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start,
                                              TokenType::TypePrimitive);
        }
        if (sliceFoundAtUnchecked(source, "ask", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start,
                                              TokenType::TaskPrimitive);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseListOrIdentifier(const StringSlice source,
                                                         const uint32_t start) {
    sy_assert(source[start - 1] == 'L', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen >= 3) {
        if (sliceFoundAtUnchecked(source, "ist", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start,
                                              TokenType::ListPrimitive);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseMapOrIdentifier(const StringSlice source,
                                                        const uint32_t start) {
    sy_assert(source[start - 1] == 'M', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen >= 2) {
        if (sliceFoundAtUnchecked(source, "ap", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start,
                                              TokenType::MapPrimitive);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseIdentifier(const StringSlice source, const uint32_t start) {
    sy_assert(isAlpha(source[start - 1]), "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    uint32_t i = start;
    for (; i < (remainingSourceLen + start); i++) {
        if (!isAlphaNumericOrUnderscore(source[i])) {
            break;
        }
    }

    return std::make_tuple(Token(TokenType::Identifier, start - 1), i);
}

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"
#include <cstring>
#include <string>

static uint32_t lineNumberUnused = 0;

static void testParseKeyword(const char* keyword, TokenType expectedTokenType) {
    const size_t keywordLength = std::strlen(keyword);

    auto stdStringToSlice = [](const std::string& s) { return StringSlice(s.data(), s.size()); };

    { // as is
        const auto slice = StringSlice(keyword, keywordLength);
        auto [token, end] = Token::parseToken(slice, 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 0);
        CHECK_GE(end, keywordLength);
    }
    { // with space in front
        const std::string str = std::string(" ") + keyword;
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 1);
        CHECK_GE(end, keywordLength);
    }
    { // with space at the end
        const std::string str = keyword + std::string(" ");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 0);
        CHECK_EQ(end, keywordLength);
    }
    { // with space at the front and end
        const std::string str = std::string(" ") + keyword + ' ';
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 1);
        CHECK_EQ(end, keywordLength + 1); // space before so 1 after
    }
    { // separator at the end
        const std::string str = keyword + std::string(";");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 0);
        CHECK_EQ(end, keywordLength);
    }
    { // fail cause non whitespace and non separator character at the end
        const std::string str = keyword + std::string("i");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_NE(token.tag(), expectedTokenType);
        CHECK_GE(end, keywordLength + 1); // goes after keyword length
    }
}

TEST_CASE("[Token] const") { testParseKeyword("const", TokenType::ConstKeyword); }

TEST_CASE("[Token] continue") { testParseKeyword("continue", TokenType::ContinueKeyword); }

TEST_CASE("[Token] comptime") { testParseKeyword("comptime", TokenType::ComptimeKeyword); }

TEST_CASE("[Token] if") { testParseKeyword("if", TokenType::IfKeyword); }

TEST_CASE("[Token] i8") { testParseKeyword("i8", TokenType::I8Primitive); }

TEST_CASE("[Token] i16") { testParseKeyword("i16", TokenType::I16Primitive); }

TEST_CASE("[Token] i32") { testParseKeyword("i32", TokenType::I32Primitive); }

TEST_CASE("[Token] i64") { testParseKeyword("i64", TokenType::I64Primitive); }

TEST_CASE("[Token] u8") { testParseKeyword("u8", TokenType::U8Primitive); }

TEST_CASE("[Token] u16") { testParseKeyword("u16", TokenType::U16Primitive); }

TEST_CASE("[Token] u32") { testParseKeyword("u32", TokenType::U32Primitive); }

TEST_CASE("[Token] u64") { testParseKeyword("u64", TokenType::U64Primitive); }

TEST_CASE("[Token] usize") { testParseKeyword("usize", TokenType::USizePrimitive); }

TEST_CASE("[Token] else") { testParseKeyword("else", TokenType::ElseKeyword); }

TEST_CASE("[Token] enum") { testParseKeyword("enum", TokenType::EnumKeyword); }

TEST_CASE("[Token] bool") { testParseKeyword("bool", TokenType::BoolPrimitive); }

TEST_CASE("[Token] break") { testParseKeyword("break", TokenType::BreakKeyword); }

TEST_CASE("[Token] mut") { testParseKeyword("mut", TokenType::MutKeyword); }

TEST_CASE("[Token] str") { testParseKeyword("str", TokenType::StrPrimitive); }

TEST_CASE("[Token] sync") { testParseKeyword("sync", TokenType::SyncKeyword); }

TEST_CASE("[Token] struct") { testParseKeyword("struct", TokenType::StructKeyword); }

TEST_CASE("[Token] switch") { testParseKeyword("switch", TokenType::SwitchKeyword); }

TEST_CASE("[Token] String") { testParseKeyword("String", TokenType::StringPrimitive); }

TEST_CASE("[Token] Shared") { testParseKeyword("Shared", TokenType::SharedKeyword); }

TEST_CASE("[Token] f32") { testParseKeyword("f32", TokenType::F32Primitive); }

TEST_CASE("[Token] f64") { testParseKeyword("f64", TokenType::F64Primitive); }

TEST_CASE("[Token] for") { testParseKeyword("for", TokenType::ForKeyword); }

TEST_CASE("[Token] false") { testParseKeyword("false", TokenType::FalseKeyword); }

TEST_CASE("[Token] char") { testParseKeyword("char", TokenType::CharPrimitive); }

TEST_CASE("[Token] fn") { testParseKeyword("fn", TokenType::FnKeyword); }

TEST_CASE("[Token] true") { testParseKeyword("true", TokenType::TrueKeyword); }

TEST_CASE("[Token] pub") { testParseKeyword("pub", TokenType::PubKeyword); }

TEST_CASE("[Token] Unique") { testParseKeyword("Unique", TokenType::UniqueKeyword); }

TEST_CASE("[Token] Weak") { testParseKeyword("Weak", TokenType::WeakKeyword); }

TEST_CASE("[Token] return") { testParseKeyword("return", TokenType::ReturnKeyword); }

TEST_CASE("[Token] throw") { testParseKeyword("throw", TokenType::ThrowKeyword); }

TEST_CASE("[Token] try") { testParseKeyword("try", TokenType::TryKeyword); }

TEST_CASE("[Token] catch") { testParseKeyword("catch", TokenType::CatchKeyword); }

TEST_CASE("[Token] and") { testParseKeyword("and", TokenType::AndKeyword); }

TEST_CASE("[Token] or") { testParseKeyword("or", TokenType::OrKeyword); }

TEST_CASE("[Token] null") { testParseKeyword("null", TokenType::NullKeyword); }

TEST_CASE("[Token] dyn") { testParseKeyword("dyn", TokenType::DynKeyword); }

TEST_CASE("[Token] as") { testParseKeyword("as", TokenType::AsKeyword); }

TEST_CASE("[Token] panic") { testParseKeyword("panic", TokenType::PanicKeyword); }

TEST_CASE("[Token] extern") { testParseKeyword("extern", TokenType::ExternKeyword); }

TEST_CASE("[Token] where") { testParseKeyword("where", TokenType::WhereKeyword); }

TEST_CASE("[Token] Self") { testParseKeyword("Self", TokenType::SelfKeyword); }

TEST_CASE("[Token] impl") { testParseKeyword("impl", TokenType::ImplKeyword); }

TEST_CASE("[Token] specific") { testParseKeyword("specific", TokenType::SpecificKeyword); }

TEST_CASE("[Token] import") { testParseKeyword("import", TokenType::ImportKeyword); }

TEST_CASE("[Token] assert") { testParseKeyword("assert", TokenType::AssertKeyword); }

TEST_CASE("[Token] in") { testParseKeyword("in", TokenType::InKeyword); }

TEST_CASE("[Token] print") { testParseKeyword("print", TokenType::PrintKeyword); };

TEST_CASE("[Token] lifetime dyn") {
    const char* keyword = "dyn\'";
    const size_t keywordLength = std::strlen(keyword);
    auto stdStringToSlice = [](const std::string& s) { return StringSlice(s.data(), s.size()); };

    { // as is
        const auto slice = StringSlice(keyword, keywordLength);
        auto [token, end] = Token::parseToken(slice, 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), TokenType::LifetimeDynKeyword);
        CHECK_EQ(token.location(), 0);
        CHECK_GE(end, keywordLength);
    }
    { // with space in front
        const std::string str = std::string(" ") + keyword;
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), TokenType::LifetimeDynKeyword);
        CHECK_EQ(token.location(), 1);
        CHECK_GE(end, keywordLength);
    }
    { // with space at the end
        const std::string str = keyword + std::string(" ");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), TokenType::LifetimeDynKeyword);
        CHECK_EQ(token.location(), 0);
        CHECK_EQ(end, keywordLength);
    }
    { // with space at the front and end
        const std::string str = std::string(" ") + keyword + ' ';
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), TokenType::LifetimeDynKeyword);
        CHECK_EQ(token.location(), 1);
        CHECK_EQ(end, keywordLength + 1); // space before so 1 after
    }
}

TEST_CASE("[Token] while") { testParseKeyword("while", TokenType::WhileKeyword); }

TEST_CASE("[Token] trait") { testParseKeyword("trait", TokenType::TraitKeyword); }

TEST_CASE("[Token] Type") { testParseKeyword("Type", TokenType::TypePrimitive); }

TEST_CASE("[Token] List") { testParseKeyword("List", TokenType::ListPrimitive); }

TEST_CASE("[Token] Map") { testParseKeyword("Map", TokenType::MapPrimitive); }

TEST_CASE("[Token] Set") { testParseKeyword("Set", TokenType::SetPrimitive); }

TEST_CASE("[Token] parallel") { testParseKeyword("parallel", TokenType::ParallelKeyword); }

TEST_CASE("[Token] await") { testParseKeyword("await", TokenType::AwaitKeyword); }

TEST_CASE("[Token] Task") { testParseKeyword("Task", TokenType::TaskPrimitive); }

static void testParseOperatorOrSymbol(const char* operatorOrSymbol, TokenType expectedTokenType,
                                      bool checkAlphaAfter, bool checkAnyOperatorAfter,
                                      bool checkSameOperatorAfter) {
    const size_t length = std::strlen(operatorOrSymbol);

    auto stdStringToSlice = [](const std::string& s) { return StringSlice(s.data(), s.size()); };

    { // as is
        const auto slice = StringSlice(operatorOrSymbol, length);
        auto [token, end] = Token::parseToken(slice, 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 0);
        CHECK_GE(end, length);
    }
    { // with space in front
        const std::string str = std::string(" ") + operatorOrSymbol;
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 1);
        CHECK_GE(end, length);
    }
    { // with space at the end
        const std::string str = operatorOrSymbol + std::string(" ");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 0);
        CHECK_EQ(end, length);
    }
    { // with space at the front and end
        const std::string str = std::string(" ") + operatorOrSymbol + ' ';
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 1);
        CHECK_EQ(end, length + 1); // space before so 1 after
    }
    { // separator at the end
        const std::string str = operatorOrSymbol + std::string(";");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 0);
        CHECK_EQ(end, length);
    }
    if (checkAlphaAfter) { // works fine with a non whitespace after
        const std::string str = operatorOrSymbol + std::string("i");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(end, length); // goes after keyword length
    }
    if (checkAnyOperatorAfter) {
        // works fine with another operator after
        // some operators cannot have others after them. very context dependant, so we resolve this
        // later
        const std::string str = operatorOrSymbol + std::string("!");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(end, length); // goes after keyword length
    }
    if (checkSameOperatorAfter) {
        // works fine with the same operator after
        // some operators cannot have others after them. very context dependant, so we resolve this
        // later
        const std::string str = operatorOrSymbol + std::string(operatorOrSymbol);
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(end, length); // goes after keyword length
    }
}

TEST_CASE("[Token] <") {
    testParseOperatorOrSymbol("<", TokenType::LessOperator, true, true, false);
}

TEST_CASE("[Token] <=") {
    testParseOperatorOrSymbol("<=", TokenType::LessOrEqualOperator, true, true, true);
}

TEST_CASE("[Token] <<") {
    testParseOperatorOrSymbol("<<", TokenType::BitshiftLeftOperator, true, true, true);
}

TEST_CASE("[Token] <<=") {
    testParseOperatorOrSymbol("<<=", TokenType::BitshiftLeftAssignOperator, true, true, true);
}

TEST_CASE("[Token] >") {
    testParseOperatorOrSymbol(">", TokenType::GreaterOperator, true, true, false);
}

TEST_CASE("[Token] >=") {
    testParseOperatorOrSymbol(">=", TokenType::GreaterOrEqualOperator, true, true, true);
}

TEST_CASE("[Token] >>") {
    testParseOperatorOrSymbol(">>", TokenType::BitshiftRightOperator, true, true, true);
}

TEST_CASE("[Token] >>=") {
    testParseOperatorOrSymbol(">>=", TokenType::BitshiftRightAssignOperator, true, true, true);
}

TEST_CASE("[Token] =") {
    testParseOperatorOrSymbol("=", TokenType::AssignOperator, true, true, false);
}

TEST_CASE("[Token] ==") {
    testParseOperatorOrSymbol("==", TokenType::EqualOperator, true, true, true);
}

TEST_CASE("[Token] +") { testParseOperatorOrSymbol("+", TokenType::AddOperator, true, true, true); }

TEST_CASE("[Token] +=") {
    testParseOperatorOrSymbol("+=", TokenType::AddAssignOperator, true, true, true);
}

TEST_CASE("[Token] *") {
    testParseOperatorOrSymbol("*", TokenType::AsteriskSymbol, true, true, true);
}

TEST_CASE("[Token] *\'") {
    testParseOperatorOrSymbol("*\'", TokenType::LifetimePointer, true, true, true);
}

TEST_CASE("[Token] *=") {
    testParseOperatorOrSymbol("*=", TokenType::MultiplyAssignOperator, true, true, true);
}

TEST_CASE("[Token] /") {
    testParseOperatorOrSymbol("/", TokenType::DivideOperator, true, true, true);
}

TEST_CASE("[Token] /=") {
    testParseOperatorOrSymbol("/=", TokenType::DivideAssignOperator, true, true, true);
}

TEST_CASE("[Token] %") {
    testParseOperatorOrSymbol("%", TokenType::ModuloOperator, true, true, true);
}

TEST_CASE("[Token] %=") {
    testParseOperatorOrSymbol("%=", TokenType::ModuloAssignOperator, true, true, true);
}

TEST_CASE("[Token] |") {
    testParseOperatorOrSymbol("|", TokenType::BitOrOperator, true, true, true);
}

TEST_CASE("[Token] |=") {
    testParseOperatorOrSymbol("|=", TokenType::BitOrAssignOperator, true, true, true);
}

TEST_CASE("[Token] ^") {
    testParseOperatorOrSymbol("^", TokenType::BitXorOperator, true, true, true);
}

TEST_CASE("[Token] ^=") {
    testParseOperatorOrSymbol("^=", TokenType::BitXorAssignOperator, true, true, true);
}

TEST_CASE("[Token] ~") {
    testParseOperatorOrSymbol("~", TokenType::BitNotOperator, true, true, true);
}

TEST_CASE("[Token] ~=") {
    testParseOperatorOrSymbol("~=", TokenType::BitNotAssignOperator, true, true, true);
}

TEST_CASE("[Token] !") {
    testParseOperatorOrSymbol("!", TokenType::ExclamationSymbol, true, true, true);
}

TEST_CASE("[Token] !=") {
    testParseOperatorOrSymbol("!=", TokenType::NotEqualOperator, true, true, true);
}

TEST_CASE("[Token] .") { testParseOperatorOrSymbol(".", TokenType::DotSymbol, true, false, true); }

TEST_CASE("[Token] .?") {
    testParseOperatorOrSymbol(".?", TokenType::OptionUnwrapOperator, true, true, true);
}

TEST_CASE("[Token] .!") {
    testParseOperatorOrSymbol(".!", TokenType::ErrorUnwrapOperator, true, true, true);
}

TEST_CASE("[Token] &") {
    testParseOperatorOrSymbol("&", TokenType::AmpersandSymbol, true, true, true);
}

TEST_CASE("[Token] &mut") {
    testParseOperatorOrSymbol("&mut", TokenType::MutableReferenceSymbol, false, true, true);
}

TEST_CASE("[Token] @\'") {
    testParseOperatorOrSymbol("@\'", TokenType::ConcreteLifetime, false, true, true);
}

TEST_CASE("[Token] (") {
    testParseOperatorOrSymbol("(", TokenType::LeftParenthesesSymbol, true, true, true);
}

TEST_CASE("[Token] )") {
    testParseOperatorOrSymbol(")", TokenType::RightParenthesesSymbol, true, true, true);
}

TEST_CASE("[Token] [") {
    testParseOperatorOrSymbol("[", TokenType::LeftBracketSymbol, true, true, true);
}

TEST_CASE("[Token] ]") {
    testParseOperatorOrSymbol("]", TokenType::RightBracketSymbol, true, true, true);
}

TEST_CASE("[Token] {") {
    testParseOperatorOrSymbol("{", TokenType::LeftBraceSymbol, true, true, true);
}

TEST_CASE("[Token] }") {
    testParseOperatorOrSymbol("}", TokenType::RightBraceSymbol, true, true, true);
}

TEST_CASE("[Token] :") { testParseOperatorOrSymbol(":", TokenType::ColonSymbol, true, true, true); }

TEST_CASE("[Token] ;") {
    testParseOperatorOrSymbol(";", TokenType::SemicolonSymbol, true, true, true);
}

TEST_CASE("[Token] ,") { testParseOperatorOrSymbol(",", TokenType::CommaSymbol, true, true, true); }

TEST_CASE("[Token] ?") {
    testParseOperatorOrSymbol("?", TokenType::OptionalSymbol, true, true, true);
}

TEST_CASE("[Token] -") {
    testParseOperatorOrSymbol("-", TokenType::SubtractOperator, true, true, true);
}

TEST_CASE("[Token] -=") {
    testParseOperatorOrSymbol("-=", TokenType::SubtractAssignOperator, true, true, true);
}

TEST_CASE("[Token] []") { testParseOperatorOrSymbol("[]", TokenType::Slice, true, true, true); }

TEST_CASE("[Token] []\'") {
    testParseOperatorOrSymbol("[]\'", TokenType::SliceLifetime, true, true, true);
}

TEST_CASE("[Token] Negative Numbers") {
    testParseOperatorOrSymbol("-0", TokenType::NumberLiteral, true, true, true);
    testParseOperatorOrSymbol("-1", TokenType::NumberLiteral, true, true, true);
    testParseOperatorOrSymbol("-2", TokenType::NumberLiteral, true, true, true);
    testParseOperatorOrSymbol("-3", TokenType::NumberLiteral, true, true, true);
    testParseOperatorOrSymbol("-4", TokenType::NumberLiteral, true, true, true);
    testParseOperatorOrSymbol("-5", TokenType::NumberLiteral, true, true, true);
    testParseOperatorOrSymbol("-6", TokenType::NumberLiteral, true, true, true);
    testParseOperatorOrSymbol("-7", TokenType::NumberLiteral, true, true, true);
    testParseOperatorOrSymbol("-8", TokenType::NumberLiteral, true, true, true);
    testParseOperatorOrSymbol("-9", TokenType::NumberLiteral, true, true, true);
    testParseOperatorOrSymbol("-1.1", TokenType::NumberLiteral, true, true, true);

    // The following ones will be invalid later in compilation, but for purely extracting a token,
    // it's fine. The tokenizer first extracts the start and end of the token's range, in which the
    // metadata can be then parsed, such as validating numbers.

    testParseOperatorOrSymbol("-9.", TokenType::NumberLiteral, true, true, true);
    testParseOperatorOrSymbol("-2.", TokenType::NumberLiteral, true, true, true);
    testParseOperatorOrSymbol("-3..5", TokenType::NumberLiteral, true, true, true);
    testParseOperatorOrSymbol("-5....7.", TokenType::NumberLiteral, true, true, true);
}

TEST_SUITE("string literals") {
    TEST_CASE("[Token] empty string") {
        {
            const StringSlice s = "\"\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = " \"\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = "\"\" ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, 2);
        }
    }

    TEST_CASE("[Token] 1 character string") {
        {
            const StringSlice s = "\"a\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = " \"a\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = "\"a\" ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, 3);
        }
    }

    TEST_CASE("[Token] multiple character string") {
        {
            const StringSlice s = "\"abc\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = " \"abc\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = "\"abc\" ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, 5);
        }
    }

    TEST_CASE("[Token] has quote character within") {
        {
            const StringSlice s = "\"\\\"\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = " \"\\\"\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = "\"\\\"\" ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, 3);
        }
    }

    TEST_CASE("[Token] has apostrophe character within") {
        {
            const StringSlice s = "\"\\\'\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = " \"\\\'\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = "\"\\\'\" ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::StringLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, 3);
        }
    }

    TEST_CASE("[Token] invalid") {
        { // not terminated last character
            const StringSlice s = " \"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::Error);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        { // not terminated
            const StringSlice s = "  \" ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::Error);
            CHECK_EQ(token.location(), 2);
            CHECK_GE(end, s.len());
        }
        { // new line within
            const StringSlice s = " \"\n\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::Error);
            CHECK_EQ(token.location(), 2);
            CHECK_GE(end, s.len());
        }
    }
}

TEST_SUITE("char literals") {
    TEST_CASE("[Token] 1 character string") {
        {
            const StringSlice s = "\'a\'";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::CharLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = " \'a\'";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::CharLiteral);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = "\'a\' ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::CharLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, 3);
        }
    }

    TEST_CASE("[Token] multiple character string") {
        {
            const StringSlice s = "\'abc\'";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::CharLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = " \'abc\'";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::CharLiteral);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = "\'abc\' ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::CharLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, 5);
        }
    }

    TEST_CASE("[Token] has escaped quote character within") {
        {
            const StringSlice s = "\'\\\"\'";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::CharLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = " \'\\\"\'";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::CharLiteral);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = "\'\\\"\' ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::CharLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, 3);
        }
    }

    TEST_CASE("[Token] has escaped apostrophe character within") {
        {
            const StringSlice s = "\'\\\'\'";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::CharLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = " \'\\\'\'";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::CharLiteral);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = "\'\\\'\' ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::CharLiteral);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, 3);
        }
    }

    TEST_CASE("[Token] invalid") {
        { // empty
            const StringSlice s = " \'\'";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::Error);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        { // not terminated last character
            const StringSlice s = " \'";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::Error);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        { // not terminated
            const StringSlice s = "  \' ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::Error);
            CHECK_EQ(token.location(), 2);
            CHECK_GE(end, s.len());
        }
        { // new line within
            const StringSlice s = " \'\n\'";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::Error);
            CHECK_EQ(token.location(), 2);
            CHECK_GE(end, s.len());
        }
    }
}

TEST_SUITE("format string literals") {
    TEST_CASE("[Token] format empty string") {
        {
            const StringSlice s = "f\"\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = " f\"\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = "f\"\" ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, 2);
        }
    }

    TEST_CASE("[Token] format 1 character string") {
        {
            const StringSlice s = "f\"a\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = " f\"a\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = "f\"a\" ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, 3);
        }
    }

    TEST_CASE("[Token] format multiple character string") {
        {
            const StringSlice s = "f\"abc\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = " f\"abc\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = "f\"abc\" ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, 5);
        }
    }

    TEST_CASE("[Token] format has quote character within") {
        {
            const StringSlice s = "f\"\\\"\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = " f\"\\\"\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = "f\"\\\"\" ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, 3);
        }
    }

    TEST_CASE("[Token] format has apostrophe character within") {
        {
            const StringSlice s = "f\"\\\'\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = " f\"\\\'\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        {
            const StringSlice s = "f\"\\\'\" ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::FormatString);
            CHECK_EQ(token.location(), 0);
            CHECK_GE(end, 3);
        }
    }

    TEST_CASE("[Token] format invalid") {
        { // not terminated last character
            const StringSlice s = " \"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::Error);
            CHECK_EQ(token.location(), 1);
            CHECK_GE(end, s.len());
        }
        { // not terminated
            const StringSlice s = "  \" ";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::Error);
            CHECK_EQ(token.location(), 2);
            CHECK_GE(end, s.len());
        }
        { // new line within
            const StringSlice s = " \"\n\"";
            auto [token, end] = Token::parseToken(s, 0, &lineNumberUnused);
            CHECK_EQ(token.tag(), TokenType::Error);
            CHECK_EQ(token.location(), 2);
            CHECK_GE(end, s.len());
        }
    }
}

TEST_CASE("[Token] Positive Numbers") {
    testParseOperatorOrSymbol("0", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("1", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("2", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("3", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("4", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("5", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("6", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("7", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("8", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("9", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("1.0", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("5.127640124", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("0xFF", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("0x01", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("0b1", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("0b1001", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("0b0001", TokenType::NumberLiteral, true, true, false);

    // The following ones will be invalid later in compilation, but for purely extracting a token,
    // it's fine. The tokenizer first extracts the start and end of the token's range, in which the
    // metadata can be then parsed, such as validating numbers.

    testParseOperatorOrSymbol("9.", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("2.", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("3..5", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("5....7.", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("4..X.7.", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("3..bb.7.", TokenType::NumberLiteral, true, true, false);
    testParseOperatorOrSymbol("1abcdefABCDEF", TokenType::NumberLiteral, true, true, false);
}

static void testParseIdentifier(const char* identifier) {
    const size_t length = std::strlen(identifier);

    auto stdStringToSlice = [](const std::string& s) { return StringSlice(s.data(), s.size()); };

    { // as is
        const auto slice = StringSlice(identifier, length);
        auto [token, end] = Token::parseToken(slice, 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), TokenType::Identifier);
        CHECK_EQ(token.location(), 0);
        CHECK_GE(end, length);
    }
    { // with space in front
        const std::string str = std::string(" ") + identifier;
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), TokenType::Identifier);
        CHECK_EQ(token.location(), 1);
        CHECK_GE(end, length);
    }
    { // with space at the end
        const std::string str = identifier + std::string(" ");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), TokenType::Identifier);
        CHECK_EQ(token.location(), 0);
        CHECK_EQ(end, length);
    }
    { // with space at the front and end
        const std::string str = std::string(" ") + identifier + ' ';
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), TokenType::Identifier);
        CHECK_EQ(token.location(), 1);
        CHECK_EQ(end, length + 1); // space before so 1 after
    }
    { // separator at the end
        const std::string str = identifier + std::string(";");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), TokenType::Identifier);
        CHECK_EQ(token.location(), 0);
        CHECK_EQ(end, length);
    }
    { // is a different identifier
        const std::string str = identifier + std::string("i");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0, &lineNumberUnused);
        CHECK_EQ(token.tag(), TokenType::Identifier);
        CHECK_EQ(token.location(), 0);
        CHECK_GE(end, length + 1); // goes after keyword length
    }
}

TEST_CASE("[Token] identifiers") {
    for (char i = 'a'; i < 'z'; i++) {
        char buf[3] = {i, '\0', '\0'};
        // testParseIdentifier(buf);
        buf[1] = 'a'; // Now it's like "aa", "ba"...
        testParseIdentifier(buf);
    }

    // similar to some keywords
    testParseIdentifier("constt");
    testParseIdentifier("mutt");
    testParseIdentifier("returnn");
    testParseIdentifier("fnn");
    testParseIdentifier("pubb");
    testParseIdentifier("iff");
    testParseIdentifier("elsee");
    testParseIdentifier("switchh");
    testParseIdentifier("whilee");
    testParseIdentifier("forr");
    testParseIdentifier("breakk");
    testParseIdentifier("continuee");
    testParseIdentifier("structt");
    testParseIdentifier("enumm");
    testParseIdentifier("dynn");
    testParseIdentifier("syncc");
    testParseIdentifier("truee");
    testParseIdentifier("falsee");
    testParseIdentifier("nulll");
    testParseIdentifier("andd");
    testParseIdentifier("orr");
    testParseIdentifier("booll");
    testParseIdentifier("i88");
    testParseIdentifier("i166");
    testParseIdentifier("i322");
    testParseIdentifier("i644");
    testParseIdentifier("u88");
    testParseIdentifier("u166");
    testParseIdentifier("u322");
    testParseIdentifier("u644");
    testParseIdentifier("usizee");
    testParseIdentifier("f322");
    testParseIdentifier("f644");
    testParseIdentifier("charr");
    testParseIdentifier("strr");
    testParseIdentifier("Stringg");
    testParseIdentifier("Ownedd");
    testParseIdentifier("Sharedd");
    testParseIdentifier("Weakk");
}

#endif // SYNC_LIB_NO_TESTS

using namespace sy;

TokenType sy::TokenRef::kind() const noexcept {
    sy_assert(this->store_->tokensLen_ >= this->index_, "Out of bounds somehow");
    return this->store_->tokens_[this->index_];
}

TokenSpan sy::TokenRef::span() const noexcept {
    sy_assert(this->store_->tokensLen_ >= this->index_, "Out of bounds somehow");
    return this->store_->spans_[this->index_];
}

StringSlice sy::TokenRef::text() const noexcept {
    TokenSpan span = this->span();
    StringSlice source = this->store_->source_;
    sy_assert(source.len() >= span.byteLocation + span.length, "Out of bounds token text");
    return StringSlice(source.data() + static_cast<ptrdiff_t>(span.byteLocation), span.length);
}

sy::NewTokenIter::NewTokenIter(const TokenStore* store) noexcept {
    current_.store_ = store;
    current_.index_ = 0;
}

Option<TokenRef> sy::NewTokenIter::next() noexcept {
    if (this->current_.index_ >= this->current_.store_->tokensLen_) {
        return {};
    }
    TokenRef out = this->current_;
    this->current_.index_ += 1;
    return out;
}

#pragma region Keyword_Bitmasks

namespace {
/// Since all Sync keywords are 8 ASCII characters in size or less, they can be turned into
/// 64-bit integer bitmasks. Character `0` occupies the least significant byte.
/// @tparam N The size of the compile time string literal.
/// @param inStr The compile time string literal.
/// @return 64-bit integer bitmask of the ASCII keyword.
template <size_t N> constexpr static uint64_t makeKeywordIntMask(char const (&inStr)[N]) {
    static_assert((N - 1) <= sizeof(uint64_t), "too big!");
    uint64_t out = 0;
    for (size_t i = 0; i < (N - 1); i++) {
        out |= static_cast<uint64_t>(static_cast<uint8_t>(inStr[i])) << (i * 8);
    }
    return out;
}

// Keywords themselves.
constexpr uint64_t AND_KEYWORD_BITMASK = makeKeywordIntMask("and");
constexpr uint64_t AS_KEYWORD_BITMASK = makeKeywordIntMask("as");
constexpr uint64_t ASSERT_KEYWORD_BITMASK = makeKeywordIntMask("assert");
constexpr uint64_t AWAIT_KEYWORD_BITMASK = makeKeywordIntMask("await");
constexpr uint64_t BREAK_KEYWORD_BITMASK = makeKeywordIntMask("break");
constexpr uint64_t CATCH_KEYWORD_BITMASK = makeKeywordIntMask("catch");
constexpr uint64_t COMPTIME_KEYWORD_BITMASK = makeKeywordIntMask("comptime");
constexpr uint64_t CONST_KEYWORD_BITMASK = makeKeywordIntMask("const");
constexpr uint64_t CONTINUE_KEYWORD_BITMASK = makeKeywordIntMask("continue");
constexpr uint64_t DYN_KEYWORD_BITMASK = makeKeywordIntMask("dyn");
constexpr uint64_t ELSE_KEYWORD_BITMASK = makeKeywordIntMask("else");
constexpr uint64_t ENUM_KEYWORD_BITMASK = makeKeywordIntMask("enum");
constexpr uint64_t EXTERN_KEYWORD_BITMASK = makeKeywordIntMask("extern");
constexpr uint64_t FN_KEYWORD_BITMASK = makeKeywordIntMask("fn");
constexpr uint64_t FOR_KEYWORD_BITMASK = makeKeywordIntMask("for");
constexpr uint64_t IF_KEYWORD_BITMASK = makeKeywordIntMask("if");
constexpr uint64_t IMPL_KEYWORD_BITMASK = makeKeywordIntMask("impl");
constexpr uint64_t IMPORT_KEYWORD_BITMASK = makeKeywordIntMask("import");
constexpr uint64_t MUT_KEYWORD_BITMASK = makeKeywordIntMask("mut");
constexpr uint64_t OR_KEYWORD_BITMASK = makeKeywordIntMask("or");
constexpr uint64_t PANIC_KEYWORD_BITMASK = makeKeywordIntMask("panic");
constexpr uint64_t PARALLEL_KEYWORD_BITMASK = makeKeywordIntMask("parallel");
constexpr uint64_t PRINT_KEYWORD_BITMASK = makeKeywordIntMask("print");
constexpr uint64_t PUB_KEYWORD_BITMASK = makeKeywordIntMask("pub");
constexpr uint64_t RETURN_KEYWORD_BITMASK = makeKeywordIntMask("return");
constexpr uint64_t SELF_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("Self");
constexpr uint64_t SELF_IDENTIFIER_KEYWORD_BITMASK = makeKeywordIntMask("self");
constexpr uint64_t STRUCT_KEYWORD_BITMASK = makeKeywordIntMask("struct");
constexpr uint64_t SWITCH_KEYWORD_BITMASK = makeKeywordIntMask("switch");
constexpr uint64_t SYNC_KEYWORD_BITMASK = makeKeywordIntMask("sync");
constexpr uint64_t TEST_KEYWORD_BITMASK = makeKeywordIntMask("test");
constexpr uint64_t THROW_KEYWORD_BITMASK = makeKeywordIntMask("throw");
constexpr uint64_t TRAIT_KEYWORD_BITMASK = makeKeywordIntMask("trait");
constexpr uint64_t TRY_KEYWORD_BITMASK = makeKeywordIntMask("try");
constexpr uint64_t WHERE_KEYWORD_BITMASK = makeKeywordIntMask("where");
constexpr uint64_t WHILE_KEYWORD_BITMASK = makeKeywordIntMask("while");

// Built-in type primitives, not the expression or generic ones, anything that has a literal
// expression version
constexpr uint64_t BOOL_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("bool");
constexpr uint64_t I8_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("i8");
constexpr uint64_t I16_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("i16");
constexpr uint64_t I32_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("i32");
constexpr uint64_t I64_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("i64");
constexpr uint64_t U8_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("u8");
constexpr uint64_t U16_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("u16");
constexpr uint64_t U32_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("u32");
constexpr uint64_t U64_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("u64");
constexpr uint64_t USIZE_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("usize");
constexpr uint64_t F32_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("f32");
constexpr uint64_t F64_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("f64");
constexpr uint64_t CHAR_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("char");
constexpr uint64_t TYPE_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("Type");
constexpr uint64_t STR_TYPE_KEYWORD_BITMASK = makeKeywordIntMask("str");

struct ParsedToken {
    TokenType tokenType;
    TokenSpan span;
};

ParsedToken parseKeywordOrIdentifier(StringSlice simdPaddedSource, size_t start) {
    size_t end = start;
    bool reachedEOF = false;
    uint64_t keywordBitmask = 0;
    keywordBitmask |= static_cast<uint64_t>(static_cast<uint8_t>(simdPaddedSource[start]));
    end += 1;
    for (size_t i = 1; i < 8; i++) {
        const size_t offset = start + i;
        if (offset >= simdPaddedSource.len()) {
            reachedEOF = true;
            break;
        }
        if (!isAlphaNumericOrUnderscore(simdPaddedSource[offset])) {
            break;
        }
        keywordBitmask |= static_cast<uint64_t>(static_cast<uint8_t>(simdPaddedSource[offset]))
                          << (i * 8);
        end += 1;
    }

    if (!reachedEOF &&
        isAlphaNumericOrUnderscore(simdPaddedSource[end])) { // longer, definitely identifier
        end += 1;
        while (true) {
            if (end >= simdPaddedSource.len()) {
                reachedEOF = true;
                break;
            }
            if (!isAlphaNumericOrUnderscore(simdPaddedSource[end])) {
                break;
            }
            end += 1;
        }
        const TokenSpan span = {.byteLocation = start, .length = end - start};
        return ParsedToken{.tokenType = TokenType::Identifier, .span = span};
    } else {
        const TokenSpan span = {.byteLocation = start, .length = end - start};

        switch (keywordBitmask) {
        case AND_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::AndKeyword, .span = span};
        case AS_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::AsKeyword, .span = span};
        case ASSERT_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::AssertKeyword, .span = span};
        case AWAIT_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::AwaitKeyword, .span = span};
        case BREAK_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::BreakKeyword, .span = span};
        case CATCH_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::CatchKeyword, .span = span};
        case COMPTIME_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::ComptimeKeyword, .span = span};
        case CONST_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::ConstKeyword, .span = span};
        case CONTINUE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::ContinueKeyword, .span = span};
        case DYN_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::DynKeyword, .span = span};
        case ELSE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::ElseKeyword, .span = span};
        case ENUM_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::EnumKeyword, .span = span};
        case EXTERN_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::ExternKeyword, .span = span};
        case FN_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::FnKeyword, .span = span};
        case FOR_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::ForKeyword, .span = span};
        case IF_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::IfKeyword, .span = span};
        case IMPL_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::ImplKeyword, .span = span};
        case IMPORT_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::ImportKeyword, .span = span};
        case MUT_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::MutKeyword, .span = span};
        case OR_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::OrKeyword, .span = span};
        case PANIC_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::PanicKeyword, .span = span};
        case PARALLEL_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::ParallelKeyword, .span = span};
        case PRINT_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::PrintKeyword, .span = span};
        case PUB_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::PubKeyword, .span = span};
        case RETURN_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::ReturnKeyword, .span = span};
        case SELF_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::SelfTypeKeyword, .span = span};
        case SELF_IDENTIFIER_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::SelfIdentifierKeyword, .span = span};
        case STRUCT_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::StructKeyword, .span = span};
        case SWITCH_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::SwitchKeyword, .span = span};
        case SYNC_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::SyncKeyword, .span = span};
        case TEST_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::TestKeyword, .span = span};
        case THROW_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::ThrowKeyword, .span = span};
        case TRAIT_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::TraitKeyword, .span = span};
        case TRY_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::TryKeyword, .span = span};
        case WHERE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::WhereKeyword, .span = span};
        case WHILE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::WhileKeyword, .span = span};
        case BOOL_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::BoolPrimitive, .span = span};
        case I8_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::I8Primitive, .span = span};
        case I16_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::I16Primitive, .span = span};
        case I32_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::I32Primitive, .span = span};
        case I64_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::I64Primitive, .span = span};
        case U8_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::U8Primitive, .span = span};
        case U16_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::U16Primitive, .span = span};
        case U32_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::U32Primitive, .span = span};
        case U64_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::U64Primitive, .span = span};
        case USIZE_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::USizePrimitive, .span = span};
        case F32_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::F32Primitive, .span = span};
        case F64_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::F64Primitive, .span = span};
        case CHAR_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::CharPrimitive, .span = span};
        case TYPE_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::TypePrimitive, .span = span};
        case STR_TYPE_KEYWORD_BITMASK:
            return ParsedToken{.tokenType = TokenType::StrPrimitive, .span = span};
        default:
            return ParsedToken{.tokenType = TokenType::Identifier, .span = span};
        }
    }
}

///
/// @param source The actual source string.
/// @param start Must be the byte index of a non-space character.
/// @return Either the parsed token, or a parsing error.
Result<ParsedToken, ProgramError> parseToken(StringSlice simdPaddedSource, size_t start) noexcept {
    sy_assert(simdPaddedSource.len() > start,
              "Expected non-zero amount of characters left to read");
    sy_assert(!isSpace(simdPaddedSource[start]), "Expected non-space character");
    const char firstChar = simdPaddedSource[start];
    if (isAlpha(firstChar) || firstChar == '_') { // Either is a keyword, or an identifier
        return parseKeywordOrIdentifier(simdPaddedSource, start);
    }

    // TODO
}

Result<TokenStore::SimdAllocatedSource, AllocErr>
TokenStore::allocatedSimdPaddedSource(Allocator alloc, size_t sourceLen) noexcept {
    auto paddedSimdSourceAllocCapacity = [](size_t sourceLen, size_t align) -> size_t {
        const size_t remainder = sourceLen % align;
        if (remainder == 0) {
            return sourceLen + align;
        }
        return (sourceLen + align) + (align - remainder);
    };

    size_t outCapacity;
    auto res = [&alloc, sourceLen, paddedSimdSourceAllocCapacity, &outCapacity]() {
#if defined(__wasm__)
        outCapacity = paddedSimdSourceAllocCapacity(sourceLen, 16);
        return alloc.allocAlignedArray<char>(outCapacity, 16);
#elif defined(_M_X64) || defined(__x86_64__)
        if (internal::cpuHasAVX512BW()) {
            outCapacity = paddedSimdSourceAllocCapacity(sourceLen, 64);
            return alloc.allocAlignedArray<char>(outCapacity, 64);
        } else if (internal::cpuHasAVX2()) {
            outCapacity = paddedSimdSourceAllocCapacity(sourceLen, 32);
            return alloc.allocAlignedArray<char>(outCapacity, 32);
        } else {
            outCapacity = paddedSimdSourceAllocCapacity(sourceLen, 16);
            return alloc.allocAlignedArray<char>(outCapacity, 16);
        }
#elif defined(__aarch64__) || defined(_M_ARM64)
        outCapacity = paddedSimdSourceAllocCapacity(sourceLen, 16);
        return alloc.allocAlignedArray<char>(outCapacity, 16);
#elif defined(__riscv) && (__riscv_xlen == 64)
        outCapacity = paddedSimdSourceAllocCapacity(sourceLen, 16);
        return alloc.allocAlignedArray<char>(outCapacity, 16);
#else
        outCapacity = paddedSimdSourceAllocCapacity(sourceLen, 16);
        return alloc.allocAlignedArray<char>(outCapacity, 16);
#endif
    }();

    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    SimdAllocatedSource source = {.data = res.value(), .len = sourceLen, .capacity = outCapacity};
    return source;
}

} // namespace

#pragma endregion

Result<TokenStore, ProgramError> sy::TokenStore::init(Allocator alloc,
                                                      StringSlice source) noexcept {
    TokenStore self(alloc);
    if (source.len() == 0) {
        return self;
    }

    /// Guarantees a full SIMD word after the source is done.

    auto simdPaddedSourceRes = TokenStore::allocatedSimdPaddedSource(alloc, source.len());
    if (simdPaddedSourceRes.hasErr()) {
        return Error(ProgramError::OutOfMemory);
    }
    self.source_ = simdPaddedSourceRes.value();
    size_t i = 0;
    for (; i < source.len(); i++) {
        self.source_.data[i] = source.data()[i];
    }
    for (; i < self.source_.capacity; i++) {
        self.source_.data[i] = '\0';
    }

    { // preallocate
      // reasonable default?
        const size_t initialCapacity = source.len() < 8 ? 8 : source.len() / 8;
        auto res = self.ensureCapacity(initialCapacity);
        if (res.hasErr()) {
            return Error(ProgramError::OutOfMemory);
        }
    }

    bool keepFinding = true;
    while (keepFinding) {
        // TODO
    }
}

Result<void, AllocErr> sy::TokenStore::ensureCapacity(size_t capacity) noexcept {
    if (this->tokensCapacity_ >= capacity) [[likely]] {
        return {};
    }

    const size_t newCapacity = capacity * 2;

    auto tokensRes = this->alloc_.allocArray<TokenType>(newCapacity);
    if (tokensRes.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    auto spansRes = this->alloc_.allocArray<TokenSpan>(newCapacity);
    if (spansRes.hasErr()) {
        this->alloc_.freeArray(tokensRes.value(), newCapacity);
        return Error(AllocErr::OutOfMemory);
    }

    TokenType* newTokens = tokensRes.value();
    TokenSpan* newSpans = spansRes.value();

    for (size_t i = 0; i < tokensLen_; i++) {
        newTokens[i] = this->tokens_[i];
    }
    for (size_t i = 0; i < tokensLen_; i++) {
        newSpans[i] = this->spans_[i];
    }

    if (this->tokensCapacity_) {
        this->alloc_.freeArray(this->tokens_, this->tokensCapacity_);
        this->alloc_.freeArray(this->spans_, this->tokensCapacity_);
    }

    this->tokens_ = newTokens;
    this->spans_ = newSpans;
    this->tokensCapacity_ = newCapacity;
    return {};
}
