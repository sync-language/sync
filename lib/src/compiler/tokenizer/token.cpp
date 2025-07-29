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
        case TokenType::SemicolonSymbol: return "SemicolonSymbol";
        case TokenType::DotSymbol: return "DotSymbol";
        case TokenType::OptionalSymbol: return "OptionalSymbol";
        case TokenType::ErrorSymbol: return "ErrorSymbol";
        case TokenType::ImmutableReferenceSymbol: return "ImmutableReferenceSymbol";
        case TokenType::MutableReferenceSymbol: return "MutableReferenceSymbol";
    default:
        sy_assert(false, "Invalid token");
    }
    unreachable();
}
