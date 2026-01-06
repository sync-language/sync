#ifndef _SY_CORE_BUILTIN_FUNCTIONS_SORT_HPP_
#define _SY_CORE_BUILTIN_FUNCTIONS_SORT_HPP_

#include "../../program/program_error.hpp"
#include "../../types/result/result.hpp"
#include "../../types/type_info.hpp"
#include "../core.h"

namespace sy {
class IteratorObj;

/// Sorts values from an iterator into the slice `outBuf`.
/// @param iter The iterator. Size is not known ahead of time.
/// @param outBuf The buffer to write the elements into.
/// @param outBufElemSize The amount of elements the buffer can store.
/// @return An error if encountered, such as `ProgramError::BufferTooSmall`, otherwise nothing.
Result<void, ProgramError> sortIntoSlice(IteratorObj* iter, void* outBuf, size_t outBufElemSize) noexcept;
} // namespace sy

#endif // _SY_CORE_BUILTIN_FUNCTIONS_SORT_HPP_