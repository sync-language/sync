#ifndef SY_COMPILER_PARSER_BASE_NODES_HPP_
#define SY_COMPILER_PARSER_BASE_NODES_HPP_

#include "../../mem/allocator.hpp"
#include "../../program/program_error.hpp"
#include "../../types/array/dynamic_array.hpp"
#include "../../types/result/result.hpp"
#include "../../types/string/string.hpp"
#include "stack_variables.hpp"

namespace sy {
struct ParseInfo;
struct Scope;
struct FunctionBuilder;

namespace detail {
class IBaseParserNode {
  public:
    IBaseParserNode(Allocator inAlloc) noexcept : alloc_(inAlloc) {}

    virtual ~IBaseParserNode() noexcept {}

    Allocator alloc() const { return this->alloc_; }

    virtual String toString() const;

    static void* operator new(size_t size, Allocator inAlloc) noexcept;

    static void operator delete(void* self) noexcept;

    static void operator delete(void* self, Allocator inAlloc) noexcept;

  private:
    /// Stupid. Necessary to make operator delete work properly with allocators.
    size_t size_;
    Allocator alloc_;
};
} // namespace detail

/// Handles the logical components / instructions within a function body.
class IFunctionStatement : public detail::IBaseParserNode {
  public:
    IFunctionStatement(Allocator inAlloc) noexcept : IBaseParserNode(inAlloc) {}

    static Result<Option<IFunctionStatement*>, ProgramError>
    parseStatement(ParseInfo* parseInfo, DynArray<StackVariable>* variables, Scope* currentScope);

    virtual Result<void, ProgramError> init(ParseInfo* parseInfo, DynArray<StackVariable>* variables,
                                            Scope* currentScope) noexcept = 0;

    virtual Result<void, ProgramError> compileStatement(FunctionBuilder* builder) const noexcept = 0;
};

/// Handles the functions themselves, which own a bunch of `IFunctionLogicNode`
/// instances. This is designed for normal functions, special functions, extern
/// functions (?), anonymous functions, and whatever else.
class IFunctionDefinition : public detail::IBaseParserNode {
  public:
    IFunctionDefinition(Allocator inAlloc) noexcept : IBaseParserNode(inAlloc) {}

    virtual Result<void, ProgramError> init(ParseInfo* parseInfo, Scope* outerScope) noexcept = 0;

    virtual Result<void, ProgramError> compile() const noexcept = 0;

    virtual StringSlice unqualifiedName() const noexcept = 0;

    virtual StringSlice qualifiedName() const noexcept = 0;
};

/// Handles type definitions. This includes structs, enums, unions, aliases,
/// and whatever else.
class ITypeDefNode : public detail::IBaseParserNode {
  public:
    ITypeDefNode(Allocator inAlloc) noexcept : IBaseParserNode(inAlloc) {}

    virtual Result<void, ProgramError> defineType() const noexcept = 0;
};
} // namespace sy

#endif // SY_COMPILER_PARSER_BASE_NODES_HPP_