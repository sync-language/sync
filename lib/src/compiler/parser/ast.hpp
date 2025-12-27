#ifndef _SY_COMPILER_PARSER_AST_HPP_
#define _SY_COMPILER_PARSER_AST_HPP_

#include "../../types/array/dynamic_array.hpp"
#include "../../types/string/string_slice.hpp"

namespace sy {
struct ParsedNodeChildren {

    ParsedNodeChildren() noexcept = default;

    ~ParsedNodeChildren() noexcept;

    uint32_t len() const noexcept { return count; }

    uint32_t getChild(uint32_t index) const noexcept;

    Result<void, AllocErr> pushChild(uint32_t childIndex, Allocator alloc) noexcept;

  private:
    static constexpr uint32_t MAX_INLINE_STORAGE = (sizeof(DynArray<uint32_t>) / sizeof(uint32_t));

    struct InlineStorage {
        uint32_t storage[MAX_INLINE_STORAGE]{};
    };

    union Storage {
        InlineStorage inlineChildren{};
        DynArray<uint32_t> dynChildren;

        Storage() noexcept : inlineChildren(InlineStorage()) {}

        ~Storage() noexcept {}
    };

    bool isDynamic = false;
    uint32_t count = 0; // due to padding, this will fit good
    Storage storage{};
};

enum class ExprBinaryOp : uint8_t {
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,

    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,
    BitshiftLeft,
    BitshiftRight,

    Equal,
    NotEqual,
    LessThan,
    LessThanEqual,
    GreaterThan,
    GreaterThanEqual,

    LogicalAnd,
    LogicalOr,
};

enum class ExprUnaryOp : uint8_t {
    Negate,
    LogicalNot,
    BitwiseNot,
    AddressOf,
};

enum class ParsedNodeTag : uint8_t {
    NumberLiteral,
    BoolLiteral,
    StringLiteral,
    NullLiteral,

    Identifier,
    Binary,
    Unary,
    CallOrGeneric,
    FieldAccess,
    ArrayIndex,
    Dereference,
    UnwrapNull,
    UnwrapError,
    Cast,
    StructLiteral,
    ArrayLiteral,
    TupleLiteral,

    PointerType,
    SliceType,
    DynType,
    ArrayType,
    NullableType,
    ErrorUnionType,
    UniqueType,
    SharedType,
    WeakType,
    TupleType,
    FnPointerType,

    VarDeclaration,
    Assignment,
    IfStatement,
    WhileLoop,
    ForLoop,
    SwitchStatement,
    ReturnStatement,
    ThrowStatement,
    TryStatement,
    CatchStatement,
    BreakStatement,
    ContinueStatement,
    SyncBlock,
    Block,
    ExpressionStatement,

    FunctionDefintion,
    StructDefinition,
    EnumDefinition,
    TraitDeclaration,

    FunctionParameter,
    SyncParameter,
};

struct ParsedNode {
    ParsedNodeTag tag;

    bool isMutable;
    bool isPublic;
    ExprBinaryOp binaryOp;
    ExprUnaryOp unaryOp;

    StringSlice value;
    StringSlice lifetime;

    ParsedNodeChildren children;

    uint32_t sourceLocation;
};

} // namespace sy

#endif // _SY_COMPILER_PARSER_AST_HPP_