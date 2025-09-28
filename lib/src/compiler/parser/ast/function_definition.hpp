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
    virtual ~FunctionDefinitionNode() noexcept;

    virtual Result<void, int> init(ParseInfo* parseInfo, Scope* outerScope) override;

    StringSlice functionName{};
    DynArrayUnmanaged<StackVariable> args{};
    Option<TypeResolutionInfo> retType{};
    Scope* scope = nullptr;
};
} // namespace sy

#endif // SY_COMPILER_PARSER_AST_FUNCTION_DEFINITION_HPP_