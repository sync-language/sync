#ifndef SY_COMPILER_PARSER_AST_RETURN_HPP_
#define SY_COMPILER_PARSER_AST_RETURN_HPP_

#include "../../../types/option/option.hpp"
#include "../base_nodes.hpp"
#include "../expression.hpp"

namespace sy {
class ReturnNode : public IFunctionStatement {
  public:
    ReturnNode(Allocator inAlloc) : IFunctionStatement(inAlloc) {}

    Option<Expression> retValue{};

    virtual Result<void, ProgramError> init(ParseInfo* parseInfo, DynArray<StackVariable>* variables,
                                            Scope* currentScope) noexcept override;

    virtual Result<void, ProgramError> compileStatement(FunctionBuilder* builder) const noexcept override;
};
} // namespace sy

#endif // SY_COMPILER_PARSER_AST_RETURN_HPP_