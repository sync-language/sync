#ifndef SY_COMPILER_PARSER_EXPRESSION_HPP_
#define SY_COMPILER_PARSER_EXPRESSION_HPP_

#include "../../mem/allocator.hpp"
#include "../../types/array/dynamic_array.hpp"
#include "../../types/option/option.hpp"
#include "../../types/result/result.hpp"
#include "../compile_info.hpp"
#include "../tokenizer/file_literals.hpp"
#include "stack_variables.hpp"

namespace sy {
class IFunctionStatement;
struct ParseInfo;
struct FunctionBuilder;

struct Expression {
    struct Deref {
        size_t sourceVariableIndex;
    };

    struct MakeRef {
        size_t sourceVariableIndex;
        bool isMutable;
    };

    enum class ExprType : int {
        Variable,
        BoolLit,
        NumLit,
        Deref,
        MakeRef,
        Null,
        Expression,
    };

    union Metadata {
        void* variableUnused;
        bool boolLit;
        NumberLiteral numLit;
        Deref deref;
        MakeRef makeRef;
        void* nullUnused;
        IFunctionStatement* expression;

        Metadata() noexcept : variableUnused(nullptr) {}
    };

    size_t variableIndex{};
    ExprType tag{};
    Metadata metadata{};
    Allocator alloc{};

    ~Expression() noexcept;

    static Result<Expression, CompileError> parse(ParseInfo* parseInfo, DynArray<StackVariable>* variables,
                                                  Option<size_t> dstVarIndex) noexcept;

    Result<void, CompileError> compileExpression(FunctionBuilder* builder) const;
};
} // namespace sy

#endif // SY_COMPILER_PARSER_EXPRESSION_HPP_