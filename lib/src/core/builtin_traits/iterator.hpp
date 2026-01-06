#ifndef _SY_CORE_BUILTIN_TRAITS_ITERATOR_HPP_
#define _SY_CORE_BUILTIN_TRAITS_ITERATOR_HPP_

#include "../../program/program_error.hpp"
#include "../../types/result/result.hpp"

namespace sy {
class Type;
class RawFunction;

class IteratorTrait {
  public:
    /// Type that implements the trait
    const Type* self;
    /// Item type that `next` yields, wrapped in an optional
    const Type* item;
    /// The relevant type for actual iteration. Useful for certain functions such as sorting.
    const Type* valueType;
    /// Get the next item in the iterator. It returns an optional.
    const RawFunction* next;
};

class IteratorObj {
  public:
    /// Actual iterator object
    void* obj;
    /// Trait implementation
    const IteratorTrait* traitImpl;

    Result<void, ProgramError> next(void* outOptional) noexcept;
};

} // namespace sy

#endif