#ifndef SY_COMPILER_PARSER_TYPE_RESOLUTION_HPP_
#define SY_COMPILER_PARSER_TYPE_RESOLUTION_HPP_

#include "../../types/option/option.hpp"

namespace sy {
class Type;

struct TypeResolutionInfo {
    Option<const Type*> knownType;
};
} // namespace sy

#endif // SY_COMPILER_PARSER_TYPE_RESOLUTION_HPP_