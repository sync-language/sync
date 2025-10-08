#ifndef SY_COMPILER_PARSER_AST_FUNCTION_DEFINITION_HPP_
#define SY_COMPILER_PARSER_AST_FUNCTION_DEFINITION_HPP_

#include "../../../types/array/dynamic_array.hpp"
#include "../../../types/option/option.hpp"
#include "../base_nodes.hpp"
#include "../stack_variables.hpp"
#include "../type_resolution.hpp"

namespace sy {
class Type;

class FunctionDefinitionNode : public IFunctionDefinition {
  public:
    FunctionDefinitionNode(Allocator inAlloc) : IFunctionDefinition(inAlloc) {}

    virtual ~FunctionDefinitionNode() noexcept;

    virtual Result<void, ProgramError> init(ParseInfo* parseInfo, Scope* outerScope) noexcept override;

    virtual Result<void, ProgramError> compile() const noexcept { return {}; }

    StringSlice functionName{};
    DynArray<StackVariable> args{};
    Option<TypeResolutionInfo> retType{};
    DynArray<StackVariable> localVariables{};
    DynArray<IFunctionStatement*> statements;
    Scope* scope = nullptr;
};
} // namespace sy

#endif // SY_COMPILER_PARSER_AST_FUNCTION_DEFINITION_HPP_