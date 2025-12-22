#ifndef SY_COMPILER_PARSER_TYPE_RESOLUTION_HPP_
#define SY_COMPILER_PARSER_TYPE_RESOLUTION_HPP_

#include "../../mem/allocator.hpp"
#include "../../program/program_error.hpp"
#include "../../types/array/dynamic_array.hpp"
#include "../../types/option/option.hpp"
#include "../../types/result/result.hpp"
#include "../../types/string/string_slice.hpp"

namespace sy {
class Type;
struct ParseInfo;

enum class ParsedTypeTag : int32_t {
    Named,
    Pointer,
    Slice,
    StaticArray,
    Dyn,
    FnPointer,
    Nullable,
    Tuple,
    ErrorUnion,
    Unique,
    Shared,
    Weak,
    IntLiteral,
    // TODO handle binary expressions and unary expressions and stuff
};

struct ParsedTypeNode {
    ParsedTypeTag tag;
    /// Used when tag is one of the following:
    /// - Pointer
    /// - Slice
    bool isMutable{};
    /// Used when tag is one of the following:
    /// - Named
    /// - Dyn
    /// - Generic
    /// - Lifetime
    StringSlice name{};
    /// Used when tag is one of the following:
    /// - IntLiteral
    StringSlice expression{};
    /// May be used when tag is one of the following:
    /// - Named (concrete lifetime)
    /// - Pointer (reference lifetime)
    /// - Slice (reference lifetime)
    /// - Dyn (reference lifetime)
    StringSlice lifetime{};
    DynArray<uint16_t> childrenIndices{};
};

enum class ParsedTypeErr {

};

struct ParsedType {
    /// The actual type info nodes. `ParsedTypeNode::childrenIndices` references this array.
    DynArray<ParsedTypeNode> nodes;

    static Result<ParsedType, ProgramError> parse(ParseInfo* parseInfo);
};

struct TypeResolutionInfo {
    StringSlice typeString;
    Option<const Type*> knownType;

    enum class Err : int {
        NotAType,
    };

    static Result<TypeResolutionInfo, Err> parse(ParseInfo* parseInfo) noexcept;
};
} // namespace sy

#endif // SY_COMPILER_PARSER_TYPE_RESOLUTION_HPP_