#ifndef SY_COMPILER_PARSER_TYPE_RESOLUTION_HPP_
#define SY_COMPILER_PARSER_TYPE_RESOLUTION_HPP_

#include "../../types/option/option.hpp"
#include "../../types/result/result.hpp"

namespace sy {
class Type;
struct ParseInfo;

struct TypeResolutionInfo {

    Option<const Type*> knownType;

    enum class Err : int {
        NotAType,
    };

    static Result<TypeResolutionInfo, Err> parse(ParseInfo* parseInfo) noexcept;
};
} // namespace sy

#endif // SY_COMPILER_PARSER_TYPE_RESOLUTION_HPP_