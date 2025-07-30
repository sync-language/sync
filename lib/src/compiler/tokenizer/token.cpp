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
        case TokenType::AssertKeyword: return "AssertKeyword";

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
        case TokenType::NotOperator: return "NotOperator";
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
        case TokenType::BitAndOperator: return "BitAndOperator";
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
        case TokenType::ErrorSymbol: return "ErrorSymbol";
        case TokenType::ImmutableReferenceSymbol: return "ImmutableReferenceSymbol";
        case TokenType::MutableReferenceSymbol: return "MutableReferenceSymbol";
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
    return isAlpha(c) && isNumeric(c);
}

constexpr static bool isAlphaNumericOrUnderscore(char c) {
    return isAlpha(c) || isNumeric(c) || c == '_';
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
        return std::make_tuple(Token(TokenType::IfKeyword, start - 1), start + 1);
    }
    if (source[start] == '8') {
        return std::make_tuple(Token(TokenType::I8Primitive, start - 1), start + 1);
    }

    if(remainingSourceLen >= 2) { // cannot fit i64, i32, or i16. Must be if, i8, or an identifier
        if (sliceFoundAtUnchecked(source, "64", start)) {
            if (
                remainingSourceLen == 2 ||
                isSpace(source[start + 2] ||
                isSeparator(source[start + 2]))
            ) {
                return std::make_tuple(Token(TokenType::I64Primitive, start - 1), start + 2);
            }
            const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 2);
            return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
        }

        if (sliceFoundAtUnchecked(source, "32", start)) {
            if (
                remainingSourceLen == 2 ||
                isSpace(source[start + 2] ||
                    isSeparator(source[start + 2]))
                ) {
                return std::make_tuple(Token(TokenType::I32Primitive, start - 1), start + 2);
            }
            const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 2);
            return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
        }

        if (sliceFoundAtUnchecked(source, "16", start)) {
            if (
                remainingSourceLen == 2 ||
                isSpace(source[start + 2] ||
                    isSeparator(source[start + 2]))
                ) {
                return std::make_tuple(Token(TokenType::I16Primitive, start - 1), start + 2);
            }
            const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 2);
            return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
        }
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

static std::tuple<Token, uint32_t> parseConstContinueOrIdentifier(
    const StringSlice source,
    const uint32_t start
) {
    sy_assert(source[start - 1] == 'c', "Invalid parse operation");

    const uint32_t remainingSourceLen = static_cast<uint32_t>(source.len()) - start;

    if (remainingSourceLen < 4) {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start);
        return std::make_tuple(Token(TokenType::Identifier, start), end);
    }

    if (sliceFoundAtUnchecked(source, "onst", start)) {
        // char after is whitespace
        if (remainingSourceLen == 4 || isSpace(source[start + 4]) || isSeparator(source[start + 4])) {
            return std::make_tuple(Token(TokenType::ConstKeyword, start - 1), start + 4);
        }
        
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 4);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }

    if(remainingSourceLen >= 7) {
        if(sliceFoundAtUnchecked(source, "ontinue", start)) {
            // char after is whitespace
            if(remainingSourceLen == 7 || isSpace(source[start + 7]) || isSeparator(source[start + 7])) {
                return std::make_tuple(Token(TokenType::ContinueKeyword, start - 1), start + 7);
            }

            const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 7);
            return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
        } 
    }

    {
        const uint32_t end = endOfAlphaNumericOrUnderscore(source, start + 1);
        return std::make_tuple(Token(TokenType::Identifier, start - 1), end);
    }
}

std::tuple<Token, uint32_t> Token::parseToken(
    const StringSlice source,
    const uint32_t start,
    const Token previous
) {
    (void)previous;

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

    if(source[nonWhitespaceStart] == 'c') {
        return parseConstContinueOrIdentifier(source, nonWhitespaceStart + 1);
    }

    return std::make_tuple(Token(TokenType::Error, static_cast<uint32_t>(-1)), 0);
}

#ifndef SYNC_LIB_NO_TESTS

#include "../../doctest.h"

TEST_SUITE("const") {
    TEST_CASE("simple") {
        const StringSlice slice = "const";
        auto [token, end] = Token::parseToken(slice, 0, Token(TokenType::Error, 0));
        CHECK_EQ(token.tag(), TokenType::ConstKeyword);
        CHECK_EQ(token.location(), 0);
        CHECK_GE(end, slice.len());
    }

    TEST_CASE("leading whitespace") {
        const StringSlice slice = " const";
        auto [token, end] = Token::parseToken(slice, 0, Token(TokenType::Error, 0));
        CHECK_EQ(token.tag(), TokenType::ConstKeyword);
        CHECK_EQ(token.location(), 1);
        CHECK_GE(end, slice.len());
    }

    TEST_CASE("trailing whitespace") {
        const StringSlice slice = "const ";
        auto [token, end] = Token::parseToken(slice, 0, Token(TokenType::Error, 0));
        CHECK_EQ(token.tag(), TokenType::ConstKeyword);
        CHECK_EQ(token.location(), 0);
        CHECK_GE(end, 5);
    }

    TEST_CASE("whitespace before and after") {
        const StringSlice slice = " const ";
        auto [token, end] = Token::parseToken(slice, 0, Token(TokenType::Error, 0));
        CHECK_EQ(token.tag(), TokenType::ConstKeyword);
        CHECK_EQ(token.location(), 1);
        CHECK_GE(end, 6);
    }

    TEST_CASE("fail cause character after") {
        const StringSlice slice = "constt";
        auto [token, end] = Token::parseToken(slice, 0, Token(TokenType::Error, 0));
        CHECK_NE(token.tag(), TokenType::ConstKeyword);
        CHECK_GE(end, 6);
    }

    TEST_CASE("fail cause character before") {
        const StringSlice slice = "cconst";
        auto [token, end] = Token::parseToken(slice, 0, Token(TokenType::Error, 0));
        CHECK_NE(token.tag(), TokenType::ConstKeyword);
        CHECK_GE(end, 6);
    }

    TEST_CASE("separator after") {
        const StringSlice slice = "const;";
        auto [token, end] = Token::parseToken(slice, 0, Token(TokenType::Error, 0));
        CHECK_EQ(token.tag(), TokenType::ConstKeyword);
        CHECK_EQ(token.location(), 0);
        CHECK_EQ(end, 5);
    }
}

TEST_SUITE("continue") {
    TEST_CASE("simple") {
        const StringSlice slice = "continue";
        auto [token, end] = Token::parseToken(slice, 0, Token(TokenType::Error, 0));
        CHECK_EQ(token.tag(), TokenType::ContinueKeyword);
        CHECK_EQ(token.location(), 0);
        CHECK_GE(end, slice.len());
    }

    TEST_CASE("leading whitespace") {
        const StringSlice slice = " continue";
        auto [token, end] = Token::parseToken(slice, 0, Token(TokenType::Error, 0));
        CHECK_EQ(token.tag(), TokenType::ContinueKeyword);
        CHECK_EQ(token.location(), 1);
        CHECK_GE(end, slice.len());
    }

    TEST_CASE("trailing whitespace") {
        const StringSlice slice = "continue ";
        auto [token, end] = Token::parseToken(slice, 0, Token(TokenType::Error, 0));
        CHECK_EQ(token.tag(), TokenType::ContinueKeyword);
        CHECK_EQ(token.location(), 0);
        CHECK_GE(end, 5);
    }

    TEST_CASE("whitespace before and after") {
        const StringSlice slice = " continue ";
        auto [token, end] = Token::parseToken(slice, 0, Token(TokenType::Error, 0));
        CHECK_EQ(token.tag(), TokenType::ContinueKeyword);
        CHECK_EQ(token.location(), 1);
        CHECK_GE(end, 6);
    }

    TEST_CASE("fail cause character after") {
        const StringSlice slice = "continuee";
        auto [token, end] = Token::parseToken(slice, 0, Token(TokenType::Error, 0));
        CHECK_NE(token.tag(), TokenType::ContinueKeyword);
        CHECK_GE(end, 6);
    }

    TEST_CASE("fail cause character before") {
        const StringSlice slice = "ccontinue";
        auto [token, end] = Token::parseToken(slice, 0, Token(TokenType::Error, 0));
        CHECK_NE(token.tag(), TokenType::ContinueKeyword);
        CHECK_GE(end, 6);
    }
    
    TEST_CASE("separator after") {
        const StringSlice slice = "continue;";
        auto [token, end] = Token::parseToken(slice, 0, Token(TokenType::Error, 0));
        CHECK_EQ(token.tag(), TokenType::ContinueKeyword);
        CHECK_EQ(token.location(), 0);
        CHECK_EQ(end, 8);
    }
}

#endif // SYNC_LIB_NO_TESTS
