#ifndef SY_COMPILER_PARSER_AST_FUNCTION_DEFINITION_HPP_
#define SY_COMPILER_PARSER_AST_FUNCTION_DEFINITION_HPP_

#include "../../../types/array/dynamic_array.hpp"
#include "../../../types/option/option.hpp"
#include "../../compile_info.hpp"
#include "../base_nodes.hpp"
#include "../stack_variables.hpp"
#include "../type_resolution.hpp"

namespace sy {
class Type;

class FunctionDefinitionNode : public IFunctionDefinition {
  public:
    virtual ~FunctionDefinitionNode() noexcept = default;

    virtual Result<void, CompileError> init(ParseInfo* parseInfo, Scope* outerScope) override;

    StringSlice functionName{};
    DynArray<StackVariable> args{};
    Option<TypeResolutionInfo> retType{};
    Scope* scope = nullptr;
};
} // namespace sy

#endif // SY_COMPILER_PARSER_AST_FUNCTION_DEFINITION_HPP_