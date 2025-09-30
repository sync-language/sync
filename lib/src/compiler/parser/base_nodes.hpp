#ifndef SY_COMPILER_PARSER_BASE_NODES_HPP_
#define SY_COMPILER_PARSER_BASE_NODES_HPP_

#include "../../mem/allocator.hpp"
#include "../../types/result/result.hpp"
#include "../../types/string/string.hpp"
#include "../compile_info.hpp"

namespace sy {
struct ParseInfo;
struct Scope;

namespace detail {
class IBaseParserNode {
  public:
    IBaseParserNode(Allocator inAlloc) noexcept : alloc_(inAlloc) {}

    virtual ~IBaseParserNode() noexcept {}

    virtual Result<void, CompileError> init(ParseInfo* parseInfo, Scope* outerScope) = 0;

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
class IFunctionLogicNode : public detail::IBaseParserNode {
  public:
    IFunctionLogicNode(Allocator inAlloc) noexcept : IBaseParserNode(inAlloc) {}

    virtual void buildFunction() const = 0;
};

/// Handles the functions themselves, which own a bunch of `IFunctionLogicNode`
/// instances. This is designed for normal functions, special functions, extern
/// functions (?), anonymous functions, and whatever else.
class IFunctionDefinition : public detail::IBaseParserNode {
  public:
    IFunctionDefinition(Allocator inAlloc) noexcept : IBaseParserNode(inAlloc) {}

    virtual void compile() const = 0;
};

/// Handles type definitions. This includes structs, enums, unions, aliases,
/// and whatever else.
class ITypeDefNode : public detail::IBaseParserNode {
  public:
    ITypeDefNode(Allocator inAlloc) noexcept : IBaseParserNode(inAlloc) {}

    virtual void defineType() const = 0;
};
} // namespace sy

#endif // SY_COMPILER_PARSER_BASE_NODES_HPP_