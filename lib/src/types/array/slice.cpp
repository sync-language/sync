#include "slice.hpp"
#include "../../core/core_internal.h"

void SY_API sy::detail::sliceDebugAssertIndexInRange(size_t index, size_t len) noexcept {
    sy_assert(index < len, "Index out of bounds");
}