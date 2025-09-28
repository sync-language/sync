#ifndef SY_COMPILER_PARSER_STACK_VARIABLES_HPP_
#define SY_COMPILER_PARSER_STACK_VARIABLES_HPP_

#include "../../core.h"
#include "../../types/string/string.hpp"
#include "type_resolution.hpp"

namespace sy {
struct StackVariable {
    StringUnmanaged name;
    bool isTemporary;
    bool isMutable;
    TypeResolutionInfo typeInfo;
};
} // namespace sy

#endif // SY_COMPILER_PARSER_STACK_VARIABLES_HPP_