#include "list.hpp"
#include "../../core/core_internal.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

void sy_list_destroy(SyList* self, size_t typeSize, size_t typeAlign,
                     SyNativeDestructorFn destruct) {
#ifndef NDEBUG
    if (self->len == 0) {
        sy_assert(self->data_ == nullptr, "Should have no list memory");
        sy_assert(self->allocated_ == nullptr, "Should have no list memory");
    } else {
        sy_assert(self->data_ != nullptr, "Should have list memory");
        sy_assert(self->allocated_ != nullptr, "Should have list memory");
    }
#endif
    if (self->len == 0) {
        return;
    }

    uint8_t* dataBytes = static_cast<uint8_t*>(self->data_);
    if (destruct != nullptr) {
        for (size_t i = 0; i < self->len; i++) {
            destruct(&dataBytes[self->len * typeSize]);
        }
    }

    uint8_t* allocatedBytes = static_cast<uint8_t*>(self->allocated_);

    const ptrdiff_t extraFrontPadding = dataBytes - allocatedBytes;
    sy_assert(extraFrontPadding >= 0, "list data should be at or ahead of the raw allocation");

    sy_allocator_free(&self->allocator, self->allocated_,
                      (self->capacity_ * typeSize) + static_cast<size_t>(extraFrontPadding),
                      typeAlign);

    self->data_ = nullptr;
    self->len = 0;
    self->capacity_ = 0;
    self->allocated_ = nullptr;
}

#ifdef __cplusplus
} // extern "C"
#endif

namespace sy {
namespace internal {
SY_API void sy_list_free_impl(void* list, size_t typeSize, size_t typeAlign,
                              NativeDestructorFn destruct) noexcept {
    SyList* asList = static_cast<SyList*>(list);
    sy_list_destroy(asList, typeSize, typeAlign, destruct);
}
} // namespace internal
} // namespace sy
