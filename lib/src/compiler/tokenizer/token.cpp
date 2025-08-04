#include "token.hpp"
#include "../../util/assert.hpp"
#include "../../util/unreachable.hpp"

static_assert(sizeof(Token) == sizeof(uint32_t));

using sy::StringSlice;

StringSlice tokenTypeToString(TokenType tokenType)
{
    switch(tokenType) {
        case TokenType::Error: return "Error";
        case TokenType::EndOfFile: return "EndOfFile";

        case TokenType::ConstKeyword: return "ConstKeyword";
        case TokenType::MutKeyword: return "MutKeyword";
        case TokenType::ReturnKeyword: return "ReturnKeyword";
        case TokenType::FnKeyword: return "FnKeyword";
        case TokenType::PubKeyword: return "PubKeyword";
        case TokenType::IfKeyword: return "IfKeyword";
        case TokenType::ElseKeyword: return "ElseKeyword";
        case TokenType::SwitchKeyword: return "SwitchKeyword";
        case TokenType::WhileKeyword: return "WhileKeyword";
        case TokenType::ForKeyword: return "ForKeyword";
        case TokenType::BreakKeyword: return "BreakKeyword";
        case TokenType::ContinueKeyword: return "ContinueKeyword";
        case TokenType::StructKeyword: return "StructKeyword";
        case TokenType::EnumKeyword: return "EnumKeyword";
        case TokenType::SyncKeyword: return "SyncKeyword";
        case TokenType::TrueKeyword: return "TrueKeyword";
        case TokenType::FalseKeyword: return "FalseKeyword";
        case TokenType::NullKeyword: return "NullKeyword";
        case TokenType::AndKeyword: return "AndKeyword";
        case TokenType::OrKeyword: return "OrKeyword";
        case TokenType::DynKeyword: return "DynKeyword";

        case TokenType::BoolPrimitive: return "BoolPrimitive";
        case TokenType::I8Primitive: return "I8Primitive";
        case TokenType::I16Primitive: return "I16Primitive";
        case TokenType::I32Primitive: return "I32Primitive";
        case TokenType::I64Primitive: return "I64Primitive";
        case TokenType::U8Primitive: return "U8Primitive";
        case TokenType::U16Primitive: return "U16Primitive";
        case TokenType::U32Primitive: return "U32Primitive";
        case TokenType::U64Primitive: return "U64Primitive";
        case TokenType::USizePrimitive: return "USizePrimitive";
        case TokenType::F32Primitive: return "F32Primitive";
        case TokenType::F64Primitive: return "F64Primitive";
        case TokenType::CharPrimitive: return "CharPrimitive";
        case TokenType::StrPrimitive: return "StrPrimitive";
        case TokenType::StringPrimitive: return "StringPrimitive";
        case TokenType::SyncOwnedPrimitive: return "SyncOwnedPrimitive";
        case TokenType::SyncSharedPrimitive: return "SyncSharedPrimitive";
        case TokenType::SyncWeakPrimitive: return "SyncWeakPrimitive";

        case TokenType::IntLiteral: return "IntLiteral";
        case TokenType::FloatLiteral: return "FloatLiteral";
        case TokenType::CharLiteral: return "CharLiteral";
        case TokenType::StringLiteral: return "StringLiteral";

        case TokenType::Identifier: return "Identifier";

        case TokenType::EqualOperator: return "EqualOperator";
        case TokenType::AssignOperator: return "AssignOperator";
        case TokenType::NotEqualOperator: return "NotEqualOperator";
        case TokenType::ErrorUncheckedUnwrapOperator: return "ErrorUncheckedUnwrapOperator";
        case TokenType::OptionUncheckedUnwrapOperator: return "OptionUncheckedUnwrapOperator";
        //case TokenType::NotOperator: return "NotOperator";
        case TokenType::LessOrEqualOperator: return "LessOrEqualOperator";
        case TokenType::LessOperator: return "LessOperator";
        case TokenType::GreaterOrEqualOperator: return "GreaterOrEqualOperator";
        case TokenType::GreaterOperator: return "GreaterOperator";
        case TokenType::AddAssignOperator: return "AddAssignOperator";
        case TokenType::AddOperator: return "AddOperator";
        case TokenType::SubtractAssignOperator: return "SubtractAssignOperator";
        case TokenType::SubtractOperator: return "SubtractOperator";
        case TokenType::MultiplyAssignOperator: return "MultiplyAssignOperator";
        case TokenType::MultiplyOperator: return "MultiplyOperator";
        case TokenType::DivideAssignOperator: return "DivideAssignOperator";
        case TokenType::DivideOperator: return "DivideOperator";
        case TokenType::PowerAssignOperator: return "PowerAssignOperator";
        case TokenType::PowerOperator: return "PowerOperator";
        case TokenType::ModuloAssignOperator: return "ModuloAssignOperator";
        case TokenType::ModuloOperator: return "ModuloOperator";
        case TokenType::BitshiftRightAssignOperator: return "BitshiftRightAssignOperator";
        case TokenType::BitshiftRightOperator: return "BitshiftRightOperator";
        case TokenType::BitshiftLeftAssignOperator: return "BitshiftLeftAssignOperator";
        case TokenType::BitshiftLeftOperator: return "BitshiftLeftOperator";
        case TokenType::BitAndAssignOperator: return "BitAndAssignOperator";
        //case TokenType::BitAndOperator: return "BitAndOperator";
        case TokenType::BitOrAssignOperator: return "BitOrAssignOperator";
        case TokenType::BitOrOperator: return "BitOrOperator";
        case TokenType::BitXorAssignOperator: return "BitXorAssignOperator";
        case TokenType::BitXorOperator: return "BitXorOperator";
        case TokenType::BitNotAssignOperator: return "BitNotAssignOperator";
        case TokenType::BitNotOperator: return "BitNotOperator";

        case TokenType::LeftParenthesesSymbol: return "LeftParenthesesSymbol";
        case TokenType::RightParenthesesSymbol: return "RightParenthesesSymbol";
        case TokenType::LeftBracketSymbol: return "LeftBracketSymbol";
        case TokenType::RightBracketSymbol: return "RightBracketSymbol";
        case TokenType::LeftBraceSymbol: return "LeftBraceSymbol";
        case TokenType::RightBraceSymbol: return "RightBraceSymbol";
        case TokenType::ColonSymbol: return "ColonSymbol";
        case TokenType::SemicolonSymbol: return "SemicolonSymbol";
        case TokenType::DotSymbol: return "DotSymbol";
        case TokenType::CommaSymbol: return "CommaSymbol";
        case TokenType::OptionalSymbol: return "OptionalSymbol";
        // case TokenType::ErrorSymbol: return "ErrorSymbol";
        //case TokenType::ImmutableReferenceSymbol: return "ImmutableReferenceSymbol";
        case TokenType::MutableReferenceSymbol: return "MutableReferenceSymbol";
        case TokenType::AmpersandSymbol: return "AmpersandSymbol";
        case TokenType::ExclamationSymbol: return "ExclamationSymbol";
    default:
        sy_assert(false, "Invalid token");
    }
    unreachable();
}

// no need to include whole header

constexpr static bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

constexpr static bool isAlpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

constexpr static bool isNumeric(char c) {
    return (c >= '0' && c <= '9');
}

constexpr static bool isSeparator(char c) {
    return c == ';' ||
        c == ',' ||
        c == ':' ||
        c == '(' ||
        c == ')' ||
        c == '[' ||
        c == ']' ||
        c == '{' ||
        c == '}';
}

constexpr static bool isAlphaNumeric(char c) {
    return isAlpha(c) || isNumeric(c);
}

constexpr static bool isAlphaNumericOrUnderscore(char c) {
    return isAlphaNumeric(c) || c == '_';
}

/// @return -1 if reached end of source
static uint32_t nonWhitespaceStartFrom(const StringSlice source, const uint32_t start) {
    const char* strData = source.data();
    const uint32_t len = static_cast<uint32_t>(source.len());
    for(uint32_t i = start; i < len; i++) {
        if(!isSpace(strData[i])) return i;
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
    for(uint32_t i = start; i < len; i++) {
        if(!isAlphaNumericOrUnderscore(strData[i])) return i;
    }
    return static_cast<uint32_t>(-1);
}

static bool sliceFoundAtUnchecked(const StringSlice source, const StringSlice toFind, const uint32_t start) {
    const uint32_t toFindLen = static_cast<uint32_t>(toFind.len());

    for(uint32_t i = 0; i < toFindLen; i++) {
        if(source[start + i] != toFind[i])  return false;
    }
    return true;
}

#pragma region Keywords

static std::tuple<Token, uint32_t> extractKeywordOrIdentifier(
    const StringSlice source,
    const uint32_t remainingSourceLen,
    const uint32_t remainingPossibleTokenLen,
    const uint32_t start,
    const TokenType possibleTokenType
) {
    const bool onlyCharsLeft = remainingSourceLen == remainingPossibleTokenLen;
    if (onlyCharsLeft) {
        return std::make_tuple(Token(possibleTokenType, start - 1), start + remainingPossibleTokenLen);
    }

    const bool whitespaceAfter = isSpace(source[start + remainingPossibleTokenLen]);
    const bool separatorAfter = isSeparator(source[start + remainingPossibleTokenLen]);
    if(whitespaceAfter || separatorAfter) {
        return std::make_tuple(Token(possibleTokenType, start - 1), start + remainingPossibleTokenLen);
    }

    const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + remainingPossibleTokenLen);
    return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
}

static std::tuple<Token, uint32_t> parseIfAndSignedIntegerTypesOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseElseEnumOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseUnsignedIntegerTypesOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseBoolTypeBreakOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseCharConstContinueOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseMutOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseReturnOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseStructSyncStrSwitchOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseStringSharedOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseFloatTypesForFalseFnOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseTrueOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parsePubOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseOwnedOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseWeakOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseAndOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseOrOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseNullOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseDynOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

static std::tuple<Token, uint32_t> parseWhileOrIdentifier(
    const StringSlice source,
    const uint32_t start
);

#pragma endregion

// Operators and symbols will not bother checking if the characters after them 
// are whitespace, separators, or alphanumeric. They will parse the token and
// set the end to be right after regardless of the following character.
#pragma region Operators_Symbols

static std::tuple<Token, uint32_t> parseLessOrBitshiftLeft(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == '<', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::LessOperator, start - 1), static_cast<uint32_t>(-1));
    }

    if(remainingSourceLen >= 2) {
        if(sliceFoundAtUnchecked(source, "<=", start)) {
            return std::make_tuple(Token(TokenType::BitshiftLeftAssignOperator, start - 1), start + 2);
        }
    }

    if(source[start] == '<') {
        return std::make_tuple(Token(TokenType::BitshiftLeftOperator, start - 1), start + 1);
    }

    if(source[start] == '=') {
        return std::make_tuple(Token(TokenType::LessOrEqualOperator, start - 1), start + 1);
    }

    return std::make_tuple(Token(TokenType::LessOperator, start - 1), start);
}

static std::tuple<Token, uint32_t> parseGreaterOrBitshiftRight(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == '>', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::GreaterOperator, start - 1), static_cast<uint32_t>(-1));
    }

    if(remainingSourceLen >= 2) {
        if(sliceFoundAtUnchecked(source, ">=", start)) {
            return std::make_tuple(Token(TokenType::BitshiftRightAssignOperator, start - 1), start + 2);
        }
    }

    if(source[start] == '>') {
        return std::make_tuple(Token(TokenType::BitshiftRightOperator, start - 1), start + 1);
    }

    if(source[start] == '=') {
        return std::make_tuple(Token(TokenType::GreaterOrEqualOperator, start - 1), start + 1);
    }

    return std::make_tuple(Token(TokenType::GreaterOperator, start - 1), start);
}

static std::tuple<Token, uint32_t> parseEqualsOrAssign(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == '=', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;
    
    if(remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::AssignOperator, start - 1), static_cast<uint32_t>(-1));
    }
    
    if(source[start] == '=') {
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
template<char startChar, TokenType nonAssignType, TokenType assignType>
static std::tuple<Token, uint32_t> parseMathOperatorWithAssign(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == startChar, "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen == 0) {
        return std::make_tuple(Token(nonAssignType, start - 1), static_cast<uint32_t>(-1));
    }
    
    if(source[start] == '=') {
        return std::make_tuple(Token(assignType, start - 1), start + 1);
    }

    return std::make_tuple(Token(nonAssignType, start - 1), start);
}

#pragma endregion

std::tuple<Token, uint32_t> Token::parseToken(
    const StringSlice source,
    const uint32_t start
) {
    const uint32_t nonWhitespaceStart = nonWhitespaceStartFrom(source, start);
    if(nonWhitespaceStart == static_cast<uint32_t>(-1)) {
        return std::make_tuple(Token(TokenType::EndOfFile, nonWhitespaceStart), 0);
    }

    switch(source[nonWhitespaceStart]) {
        // For tokens with no possible variants and are 1 character, this works
        // Semicolon is on most lines of code
        case ';': return std::make_tuple(Token(TokenType::SemicolonSymbol, nonWhitespaceStart),
            nonWhitespaceStart + 1);
        case ',': return std::make_tuple(Token(TokenType::CommaSymbol, nonWhitespaceStart),
            nonWhitespaceStart + 1);
        case '{': return std::make_tuple(Token(TokenType::LeftBraceSymbol, nonWhitespaceStart),
            nonWhitespaceStart + 1);
        case '}': return std::make_tuple(Token(TokenType::RightBraceSymbol, nonWhitespaceStart),
            nonWhitespaceStart + 1);
        case '(': return std::make_tuple(Token(TokenType::LeftParenthesesSymbol, nonWhitespaceStart),
            nonWhitespaceStart + 1);
        case ')': return std::make_tuple(Token(TokenType::RightParenthesesSymbol, nonWhitespaceStart),
            nonWhitespaceStart + 1);
        case '[': return std::make_tuple(Token(TokenType::LeftBracketSymbol, nonWhitespaceStart),
            nonWhitespaceStart + 1);
        case ']': return std::make_tuple(Token(TokenType::RightBracketSymbol, nonWhitespaceStart),
            nonWhitespaceStart + 1);
        case ':': return std::make_tuple(Token(TokenType::ColonSymbol, nonWhitespaceStart),
            nonWhitespaceStart + 1);
        case '?': return std::make_tuple(Token(TokenType::OptionalSymbol, nonWhitespaceStart),
            nonWhitespaceStart + 1);

        case '_': { // is definitely an identifier
            // already did first char
            const uint32_t end = endOfAlphaNumericOrUnderscore(source, nonWhitespaceStart + 1);
            return std::make_tuple(Token(TokenType::Identifier, nonWhitespaceStart), end);
        };
        
        default: break;
    }

    // if will definitely be used a lot, along with probably the signed integer
    // types so checking those first is good
    if(source[nonWhitespaceStart] == 'i') {
        return parseIfAndSignedIntegerTypesOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // else will probably be used a lot as well
    if(source[nonWhitespaceStart] == 'e') {
        return parseElseEnumOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // const should be extremely used. char and continue exist too.
    if(source[nonWhitespaceStart] == 'c') {
        return parseCharConstContinueOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // mut also should be extremely used
    if(source[nonWhitespaceStart] == 'm') {
        return parseMutOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // return    
    if(source[nonWhitespaceStart] == 'r') {
        return parseReturnOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // unsigned integer types will also get used quite a lot
    if(source[nonWhitespaceStart] == 'u') {
        return parseUnsignedIntegerTypesOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // bool should be common. break exists too.
    if(source[nonWhitespaceStart] == 'b') {
        return parseBoolTypeBreakOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // struct, sync (lowercase), str, switch
    if(source[nonWhitespaceStart] == 's') {
        return parseStructSyncStrSwitchOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // capital S (String, Shared)
    if(source[nonWhitespaceStart] == 'S') {
        return parseStringSharedOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // float types, for, false, fn
    if(source[nonWhitespaceStart] == 'f') {
        return parseFloatTypesForFalseFnOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // true
    if(source[nonWhitespaceStart] == 't') {
        return parseTrueOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // pub
    if(source[nonWhitespaceStart] == 'p') {
        return parsePubOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // Owned
    if(source[nonWhitespaceStart] == 'O') {
        return parseOwnedOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // Weak
    if(source[nonWhitespaceStart] == 'W') {
        return parseWeakOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // and
    if(source[nonWhitespaceStart] == 'a') {
        return parseAndOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // or
    if(source[nonWhitespaceStart] == 'o') {
        return parseOrOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // null
    if(source[nonWhitespaceStart] == 'n') {
        return parseNullOrIdentifier(source, nonWhitespaceStart + 1);
    }
    
    // dyn
    if(source[nonWhitespaceStart] == 'd') {
        return parseDynOrIdentifier(source, nonWhitespaceStart + 1);
    }

    // while
    if(source[nonWhitespaceStart] == 'w') {
        return parseWhileOrIdentifier(source, nonWhitespaceStart + 1);
    }

    if(source[nonWhitespaceStart] == '<') {
        return parseLessOrBitshiftLeft(source, nonWhitespaceStart + 1);
    }

    if(source[nonWhitespaceStart] == '>') {
        return parseGreaterOrBitshiftRight(source, nonWhitespaceStart + 1);
    }

    if(source[nonWhitespaceStart] == '=') {
        return parseEqualsOrAssign(source, nonWhitespaceStart + 1);
    }

    if(source[nonWhitespaceStart] == '+') {
        return parseMathOperatorWithAssign
            <'+', TokenType::AddOperator, TokenType::AddAssignOperator>
            (source, nonWhitespaceStart + 1);
    }

    if(source[nonWhitespaceStart] == '*') {
        return parseMathOperatorWithAssign
            <'*', TokenType::MultiplyOperator, TokenType::MultiplyAssignOperator>
            (source, nonWhitespaceStart + 1);
    }

    if(source[nonWhitespaceStart] == '/') {
        return parseMathOperatorWithAssign
            <'/', TokenType::DivideOperator, TokenType::DivideAssignOperator>
            (source, nonWhitespaceStart + 1);
    }

    if(source[nonWhitespaceStart] == '%') {
        return parseMathOperatorWithAssign
            <'%', TokenType::ModuloOperator, TokenType::ModuloAssignOperator>
            (source, nonWhitespaceStart + 1);
    }

    if(source[nonWhitespaceStart] == '|') {
        return parseMathOperatorWithAssign
            <'|', TokenType::BitOrOperator, TokenType::BitOrAssignOperator>
            (source, nonWhitespaceStart + 1);
    }

    if(source[nonWhitespaceStart] == '^') {
        return parseMathOperatorWithAssign
            <'^', TokenType::BitXorOperator, TokenType::BitXorAssignOperator>
            (source, nonWhitespaceStart + 1);
    }

    if(source[nonWhitespaceStart] == '~') {
        return parseMathOperatorWithAssign
            <'~', TokenType::BitNotOperator, TokenType::BitNotAssignOperator>
            (source, nonWhitespaceStart + 1);
    }

    if(source[nonWhitespaceStart] == '!') {
        return parseMathOperatorWithAssign
            <'!', TokenType::ExclamationSymbol, TokenType::NotEqualOperator>
            (source, nonWhitespaceStart + 1);
    }
    
    return std::make_tuple(Token(TokenType::Error, static_cast<uint32_t>(-1)), 0);
}

static std::tuple<Token, uint32_t> parseIfAndSignedIntegerTypesOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'i', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen == 0) {
        // Literally the identifier "i". Conveniently this is common for iterators.
        return std::make_tuple(Token(TokenType::Identifier, start - 1), start);
    }

    if (source[start] == 'f') {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 1, start, TokenType::IfKeyword);
    }
    if (source[start] == '8') {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 1, start, TokenType::I8Primitive);
    }

    if(remainingSourceLen >= 2) { // cannot fit i64, i32, or i16. Must be if, i8, or an identifier
        // likely used the most
        // TODO get actual metrics for this
        if (sliceFoundAtUnchecked(source, "32", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start, TokenType::I32Primitive);
        }

        // 64 bit signed integer probably used less than 32 bit signed
        if (sliceFoundAtUnchecked(source, "64", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start, TokenType::I64Primitive);
        }

        // 16 bit integers probably used the least
        if (sliceFoundAtUnchecked(source, "16", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start, TokenType::I16Primitive);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseElseEnumOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'e', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen < 3) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "lse", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start, TokenType::ElseKeyword);
    }

    if (sliceFoundAtUnchecked(source, "num", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start, TokenType::EnumKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 3);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseUnsignedIntegerTypesOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'u', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen == 0) {
        // Identifier "u"
        return std::make_tuple(Token(TokenType::Identifier, start - 1), start);
    }

    if(source[start] == '8') {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 1, start, TokenType::U8Primitive);
    }

    if(remainingSourceLen >= 2) { // cannot fit u64, u32, or u16. Must be if, i8, or an identifier
        // While 32 bit signed is probably more popular than 64 bit signed, 64 bit unsigned is probably more popular
        // TODO get actual metrics for this
        if (sliceFoundAtUnchecked(source, "64", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start, TokenType::U64Primitive);
        }

        if (sliceFoundAtUnchecked(source, "32", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start, TokenType::U32Primitive);
        }

        // 16 bit integers probably used the least
        if (sliceFoundAtUnchecked(source, "16", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start, TokenType::U16Primitive);
        }
    }

    if(remainingSourceLen >= 4) {
        if (sliceFoundAtUnchecked(source, "size", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start, TokenType::USizePrimitive);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 1);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseBoolTypeBreakOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'b', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen < 3) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "ool", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start, TokenType::BoolPrimitive);
    }

    if(remainingSourceLen >= 4) {
        if(sliceFoundAtUnchecked(source, "reak", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start, TokenType::BreakKeyword);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 3);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseCharConstContinueOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'c', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen < 3) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "har", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start, TokenType::CharPrimitive);
    }

    if(remainingSourceLen >= 4) {
        if (sliceFoundAtUnchecked(source, "onst", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start, TokenType::ConstKeyword);
        }
    }

    if(remainingSourceLen >= 7) {
        if(sliceFoundAtUnchecked(source, "ontinue", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 7, start, TokenType::ContinueKeyword);
        } 
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 3);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseMutOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'm', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen < 2) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "ut", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start, TokenType::MutKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 2);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseReturnOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'r', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen < 5) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "eturn", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 5, start, TokenType::ReturnKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 5);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseStructSyncStrSwitchOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 's', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen < 2) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    // struct starts with str so this must be done first
    if(remainingSourceLen >= 5) {
        if(sliceFoundAtUnchecked(source, "truct", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 5, start, TokenType::StructKeyword);    
        }
        if(sliceFoundAtUnchecked(source, "witch", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 5, start, TokenType::SwitchKeyword);    
        }
    }

    if (sliceFoundAtUnchecked(source, "tr", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start, TokenType::StrPrimitive);
    }

    if(remainingSourceLen >= 3) {
        if(sliceFoundAtUnchecked(source, "ync", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start, TokenType::SyncKeyword);    
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 2);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseStringSharedOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'S', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen < 5) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if(sliceFoundAtUnchecked(source, "tring", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 5, start, TokenType::StringPrimitive);    
    }
    if(sliceFoundAtUnchecked(source, "hared", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 5, start, TokenType::SyncSharedPrimitive);    
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 5);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseFloatTypesForFalseFnOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'f', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen == 0) {
        return std::make_tuple(Token(TokenType::FnKeyword, start - 1), start + 1);
    }

    if(source[start] == 'n') {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 1, start, TokenType::FnKeyword);
    }

    if(remainingSourceLen >= 2) {
        if (sliceFoundAtUnchecked(source, "or", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start, TokenType::ForKeyword);
        }

        // prefer 64 bit floats to 32 bit floats for accuracy
        if (sliceFoundAtUnchecked(source, "64", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start, TokenType::F64Primitive);
        }

        if (sliceFoundAtUnchecked(source, "32", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start, TokenType::F32Primitive);
        }
    }

    if(remainingSourceLen >= 4) {
        if (sliceFoundAtUnchecked(source, "alse", start)) {
            return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start, TokenType::FalseKeyword);
        }
    }
    
    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 1);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseTrueOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 't', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen < 3) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "rue", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start, TokenType::TrueKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 3);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parsePubOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'p', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen < 2) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "ub", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start, TokenType::PubKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 2);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseOwnedOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'O', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen < 4) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "wned", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start, TokenType::SyncOwnedPrimitive);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 4);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseWeakOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'W', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen < 3) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "eak", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start, TokenType::SyncWeakPrimitive);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 3);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseAndOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'a', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen < 2) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "nd", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start, TokenType::AndKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 2);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseOrOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'o', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(source[start] == 'r') {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 1, start, TokenType::OrKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 1);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseNullOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'n', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen < 2) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "ull", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 3, start, TokenType::NullKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 3);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseDynOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'd', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen < 2) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "yn", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 2, start, TokenType::DynKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 2);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseWhileOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'w', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if(remainingSourceLen < 2) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if (sliceFoundAtUnchecked(source, "hile", start)) {
        return extractKeywordOrIdentifier(source, remainingSourceLen, 4, start, TokenType::WhileKeyword);
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 4);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

#ifndef SYNC_LIB_NO_TESTS

#include "../../doctest.h"
#include <string>
#include <cstring>

static void testParseKeyword(const char* keyword, TokenType expectedTokenType)
{
    const size_t keywordLength = std::strlen(keyword);

    auto stdStringToSlice = [](const std::string& s) {
        return StringSlice(s.data(), s.size());
    };

    { // as is
        const auto slice = StringSlice(keyword, keywordLength);
        auto [token, end] = Token::parseToken(slice, 0);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 0);
        CHECK_GE(end, keywordLength);
    }
    { // with space in front
        const std::string str = std::string(" ") + keyword;
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 1);
        CHECK_GE(end, keywordLength);
    }
    { // with space at the end
        const std::string str = keyword + std::string(" ");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 0);
        CHECK_EQ(end, keywordLength);
    }
    { // with space at the front and end
        const std::string str = std::string(" ") + keyword + ' ';
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 1);
        CHECK_EQ(end, keywordLength + 1); // space before so 1 after
    }
    { // separator at the end
        const std::string str = keyword + std::string(";");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 0);
        CHECK_EQ(end, keywordLength);
    }
    { // fail cause non whitespace and non separator character at the end
        const std::string str = keyword + std::string("i");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0);
        CHECK_NE(token.tag(), expectedTokenType);
        CHECK_GE(end, keywordLength + 1); // goes after keyword length
    }
}

TEST_CASE("const") {
    testParseKeyword("const", TokenType::ConstKeyword);
}

TEST_CASE("continue") {
    testParseKeyword("continue", TokenType::ContinueKeyword);
}

TEST_CASE("if") {
    testParseKeyword("if", TokenType::IfKeyword);
}

TEST_CASE("i8") {
    testParseKeyword("i8", TokenType::I8Primitive);
}

TEST_CASE("i16") {
    testParseKeyword("i16", TokenType::I16Primitive);
}

TEST_CASE("i32") {
    testParseKeyword("i32", TokenType::I32Primitive);
}

TEST_CASE("i64") {
    testParseKeyword("i64", TokenType::I64Primitive);
}

TEST_CASE("u8") {
    testParseKeyword("u8", TokenType::U8Primitive);
}

TEST_CASE("u16") {
    testParseKeyword("u16", TokenType::U16Primitive);
}

TEST_CASE("u32") {
    testParseKeyword("u32", TokenType::U32Primitive);
}

TEST_CASE("u64") {
    testParseKeyword("u64", TokenType::U64Primitive);
}

TEST_CASE("usize") {
    testParseKeyword("usize", TokenType::USizePrimitive);
}

TEST_CASE("else") {
    testParseKeyword("else", TokenType::ElseKeyword);
}

TEST_CASE("enum") {
    testParseKeyword("enum", TokenType::EnumKeyword);
}

TEST_CASE("bool") {
    testParseKeyword("bool", TokenType::BoolPrimitive);
}

TEST_CASE("break") {
    testParseKeyword("break", TokenType::BreakKeyword);
}

TEST_CASE("mut") {
    testParseKeyword("mut", TokenType::MutKeyword);
}

TEST_CASE("str") {
    testParseKeyword("str", TokenType::StrPrimitive);
}

TEST_CASE("sync") {
    testParseKeyword("sync", TokenType::SyncKeyword);
}

TEST_CASE("struct") {
    testParseKeyword("struct", TokenType::StructKeyword);
}

TEST_CASE("switch") {
    testParseKeyword("switch", TokenType::SwitchKeyword);
}

TEST_CASE("String") {
    testParseKeyword("String", TokenType::StringPrimitive);
}

TEST_CASE("Shared") {
    testParseKeyword("Shared", TokenType::SyncSharedPrimitive);
}

TEST_CASE("f32") {
    testParseKeyword("f32", TokenType::F32Primitive);
}

TEST_CASE("f64") {
    testParseKeyword("f64", TokenType::F64Primitive);
}

TEST_CASE("for") {
    testParseKeyword("for", TokenType::ForKeyword);
}

TEST_CASE("false") {
    testParseKeyword("false", TokenType::FalseKeyword);
}

TEST_CASE("char") {
    testParseKeyword("char", TokenType::CharPrimitive);
}

TEST_CASE("fn") {
    testParseKeyword("fn", TokenType::FnKeyword);
}

TEST_CASE("true") {
    testParseKeyword("true", TokenType::TrueKeyword);
}

TEST_CASE("pub") {
    testParseKeyword("pub", TokenType::PubKeyword);
}

TEST_CASE("Owned") {
    testParseKeyword("Owned", TokenType::SyncOwnedPrimitive);
}

TEST_CASE("Weak") {
    testParseKeyword("Weak", TokenType::SyncWeakPrimitive);
}

TEST_CASE("return") {
    testParseKeyword("return", TokenType::ReturnKeyword);
}

TEST_CASE("and") {
    testParseKeyword("and", TokenType::AndKeyword);
}

TEST_CASE("or") {
    testParseKeyword("or", TokenType::OrKeyword);
}

TEST_CASE("null") {
    testParseKeyword("null", TokenType::NullKeyword);
}

TEST_CASE("dyn") {
    testParseKeyword("dyn", TokenType::DynKeyword);
}

TEST_CASE("while") {
    testParseKeyword("while", TokenType::WhileKeyword);
}

static void testParseOperatorOrSymbol(
    const char* operatorOrSymbol,
    TokenType expectedTokenType,
    bool checkAnyOperatorAfter,
    bool checkSameOperatorAfter
) {
    const size_t length = std::strlen(operatorOrSymbol);

    auto stdStringToSlice = [](const std::string& s) {
        return StringSlice(s.data(), s.size());
    };

    { // as is
        const auto slice = StringSlice(operatorOrSymbol, length);
        auto [token, end] = Token::parseToken(slice, 0);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 0);
        CHECK_GE(end, length);
    }
    { // with space in front
        const std::string str = std::string(" ") + operatorOrSymbol;
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 1);
        CHECK_GE(end, length);
    }
    { // with space at the end
        const std::string str = operatorOrSymbol + std::string(" ");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 0);
        CHECK_EQ(end, length);
    }
    { // with space at the front and end
        const std::string str = std::string(" ") + operatorOrSymbol + ' ';
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 1);
        CHECK_EQ(end, length + 1); // space before so 1 after
    }
    { // separator at the end
        const std::string str = operatorOrSymbol + std::string(";");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(token.location(), 0);
        CHECK_EQ(end, length);
    }
    { // works fine with a non whitespace after
        const std::string str = operatorOrSymbol + std::string("i");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(end, length); // goes after keyword length
    }
    if(checkAnyOperatorAfter) { 
        // works fine with another operator after
        // some operators cannot have others after them. very context dependant, so we resolve this later
        const std::string str = operatorOrSymbol + std::string("!");
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(end, length); // goes after keyword length
    }
    if(checkSameOperatorAfter) { 
        // works fine with the same operator after 
        // some operators cannot have others after them. very context dependant, so we resolve this later
        const std::string str = operatorOrSymbol + std::string(operatorOrSymbol);
        auto [token, end] = Token::parseToken(stdStringToSlice(str), 0);
        CHECK_EQ(token.tag(), expectedTokenType);
        CHECK_EQ(end, length); // goes after keyword length
    }
}

TEST_CASE("<") {
    testParseOperatorOrSymbol("<", TokenType::LessOperator, true, false);
}

TEST_CASE("<=") {
    testParseOperatorOrSymbol("<=", TokenType::LessOrEqualOperator, true, true);
}

TEST_CASE("<<") {
    testParseOperatorOrSymbol("<<", TokenType::BitshiftLeftOperator, true, true);
}

TEST_CASE("<<=") {
    testParseOperatorOrSymbol("<<=", TokenType::BitshiftLeftAssignOperator, true, true);
}

TEST_CASE(">") {
    testParseOperatorOrSymbol(">", TokenType::GreaterOperator, true, false);
}

TEST_CASE(">=") {
    testParseOperatorOrSymbol(">=", TokenType::GreaterOrEqualOperator, true, true);
}

TEST_CASE(">>") {
    testParseOperatorOrSymbol(">>", TokenType::BitshiftRightOperator, true, true);
}

TEST_CASE(">>=") {
    testParseOperatorOrSymbol(">>=", TokenType::BitshiftRightAssignOperator, true, true);
}

TEST_CASE("=") {
    testParseOperatorOrSymbol("=", TokenType::AssignOperator, true, false);
}

TEST_CASE("==") {
    testParseOperatorOrSymbol("==", TokenType::EqualOperator, true, true);
}

TEST_CASE("+") {
    testParseOperatorOrSymbol("+", TokenType::AddOperator, true, true);
}

TEST_CASE("+=") {
    testParseOperatorOrSymbol("+=", TokenType::AddAssignOperator, true, true);
}

TEST_CASE("*") {
    testParseOperatorOrSymbol("*", TokenType::MultiplyOperator, true, true);
}

TEST_CASE("*=") {
    testParseOperatorOrSymbol("*=", TokenType::MultiplyAssignOperator, true, true);
}

TEST_CASE("/") {
    testParseOperatorOrSymbol("/", TokenType::DivideOperator, true, true);
}

TEST_CASE("/=") {
    testParseOperatorOrSymbol("/=", TokenType::DivideAssignOperator, true, true);
}

TEST_CASE("%") {
    testParseOperatorOrSymbol("%", TokenType::ModuloOperator, true, true);
}

TEST_CASE("%=") {
    testParseOperatorOrSymbol("%=", TokenType::ModuloAssignOperator, true, true);
}

TEST_CASE("|") {
    testParseOperatorOrSymbol("|", TokenType::BitOrOperator, true, true);
}

TEST_CASE("|=") {
    testParseOperatorOrSymbol("|=", TokenType::BitOrAssignOperator, true, true);
}

TEST_CASE("^") {
    testParseOperatorOrSymbol("^", TokenType::BitXorOperator, true, true);
}

TEST_CASE("^=") {
    testParseOperatorOrSymbol("^=", TokenType::BitXorAssignOperator, true, true);
}

TEST_CASE("~") {
    testParseOperatorOrSymbol("~", TokenType::BitNotOperator, true, true);
}

TEST_CASE("~=") {
    testParseOperatorOrSymbol("~=", TokenType::BitNotAssignOperator, true, true);
}

TEST_CASE("!") {
    testParseOperatorOrSymbol("!", TokenType::ExclamationSymbol, true, true);
}

TEST_CASE("!=") {
    testParseOperatorOrSymbol("!=", TokenType::NotEqualOperator, true, true);
}

#endif // SYNC_LIB_NO_TESTS
