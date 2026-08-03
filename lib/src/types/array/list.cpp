#include "list.hpp"
#include "../../core/core_internal.h"
#include "../type.hpp"
#include "list.h"
#include <cstring>

using namespace sy;

static size_t extraFrontPadding(const SyList* self, size_t typeSize) noexcept {
    const uint8_t* dataBytes = static_cast<const uint8_t*>(self->data_);
    const uint8_t* allocatedBytes = static_cast<const uint8_t*>(self->allocated_);
    const ptrdiff_t extraFrontBytes = dataBytes - allocatedBytes;
    return extraFrontBytes / typeSize;
}

static size_t listCapacityBack(const SyList* self, size_t typeSize) {
    return self->capacity_ - extraFrontPadding(self, typeSize);
}

static size_t extraBackPadding(const SyList* self, size_t typeSize) noexcept {
    return listCapacityBack(self, typeSize) - self->len;
}

static size_t listCapacityFront(const SyList* self, size_t typeSize) {
    return self->len + extraFrontPadding(self, typeSize);
}

static size_t capacityIncrease(const size_t inCapacity) noexcept {
    constexpr size_t initialArrayCapacity = 4;

    if (inCapacity == 0)
        return initialArrayCapacity;

    constexpr size_t lowAmount = 1024;
    // increasing by 1.5 without double conversion is n * 3 / 2.
    // simplified, it is (n * 3) >> 1;

    constexpr size_t superHighAmount = SIZE_MAX / 3;

    sy_assert(inCapacity <= superHighAmount, "DynArrayUnmanaged too big");
    (void)superHighAmount;

    if (inCapacity < lowAmount) {
        return inCapacity << 1;
    } else {
        return (inCapacity * 3) >> 1;
    }
}

static Result<void, AllocErr> reallocateBack(SyList* self, size_t typeSize,
                                             size_t typeAlign) noexcept {
    const size_t retainFrontPadding = extraFrontPadding(self, typeSize);

    const sy::Allocator* cppallocPtr = reinterpret_cast<const sy::Allocator*>(&self->allocator);
    sy::Allocator alloc = *cppallocPtr;

    const size_t newFullAllocationCapacity = capacityIncrease(retainFrontPadding + self->len);

    auto allocRes =
        alloc.allocAlignedArray<uint8_t>(newFullAllocationCapacity * typeSize, typeAlign);
    if (allocRes.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }
    uint8_t* mem = allocRes.takeValue().take();

    if (self->data_) {
        memcpy(&mem[retainFrontPadding * typeSize], self->data_, self->len * typeSize);
        alloc.freeAlignedArray<uint8_t>(static_cast<uint8_t*>(self->allocated_),
                                        self->capacity_ * typeSize, typeAlign);
    }

    self->data_ = static_cast<void*>(&mem[retainFrontPadding * typeSize]);
    self->allocated_ = static_cast<void*>(mem);
    self->capacity_ = newFullAllocationCapacity;
}

/// Gets a pointer to the next element to add onto the back of this list. May re-allocate the entire
/// list.
static Result<void*, AllocErr> addOneElementToListBack(SyList* self, size_t typeSize,
                                                       size_t typeAlign) noexcept {
    if (self->len < listCapacityBack(self, typeSize)) {
        return static_cast<void*>(&static_cast<uint8_t*>(self->data_)[self->len * typeSize]);
    }

    if (reallocateBack(self, typeSize, typeAlign).hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    return static_cast<void*>(&static_cast<uint8_t*>(self->data_)[self->len * typeSize]);
}

/// Gets a pointer to the next element to add onto the front of this list. May re-allocate the
/// entire list. Does NOT update `self->data` to decrement the offset. Set `self->data` to the
/// return value of this function on success.
/// @return If `nullptr`, allocation failed, otherwise the element to write to.
static Result<void*, AllocErr> addOneElementToListFront(SyList* self, size_t typeSize,
                                                        size_t typeAlign) noexcept {
    if (self->len < listCapacityFront(self, typeSize)) {
        return static_cast<void*>(static_cast<uint8_t*>(self->data_) - (self->len * typeSize));
    }

    const size_t retainBackPadding = extraBackPadding(self, typeSize);

    const sy::Allocator* cppallocPtr = reinterpret_cast<const sy::Allocator*>(&self->allocator);
    sy::Allocator alloc = *cppallocPtr;

    const size_t newFullAllocationCapacity = capacityIncrease(retainBackPadding + self->len);

    auto allocRes =
        alloc.allocAlignedArray<uint8_t>(newFullAllocationCapacity * typeSize, typeAlign);
    if (allocRes.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }
    uint8_t* mem = allocRes.takeValue().take();

    // starting the new data from here should give enough front space
    const size_t startOffsetFromAllocBeginning =
        newFullAllocationCapacity - (retainBackPadding + self->len);

    if (self->data_) {
        memcpy(&mem[startOffsetFromAllocBeginning * typeSize], self->data_, self->len * typeSize);
        alloc.freeAlignedArray<uint8_t>(static_cast<uint8_t*>(self->allocated_),
                                        self->capacity_ * typeSize, typeAlign);
    }

    self->data_ = static_cast<void*>(&mem[self->len * typeSize]);
    self->allocated_ = static_cast<void*>(mem);
    self->capacity_ = newFullAllocationCapacity;

    return static_cast<void*>(
        &static_cast<uint8_t*>(self->data_)[(startOffsetFromAllocBeginning - 1) * typeSize]);
}

static Result<void*, AllocErr> addOneElementToInsertAt(SyList* self, size_t typeSize,
                                                       size_t typeAlign, size_t index) noexcept {
    sy_assert_release(index < self->len, "Index out of bounds in List");

    if (index == 0) {
        return addOneElementToListFront(self, typeSize, typeAlign);
    } else if (index == (self->len - 1)) {
        return addOneElementToListBack(self, typeSize, typeAlign);
    }

    if (self->len == listCapacityBack(self, typeSize)) {
        if (reallocateBack(self, typeSize, typeAlign).hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
    }

    uint8_t* dataBytes = reinterpret_cast<uint8_t*>(self->data_);
    memmove(&dataBytes[(index + 1) * typeSize], &dataBytes[index * typeSize],
            (self->len - index) * typeSize);

    return static_cast<void*>(&dataBytes[index * typeSize]);
}

template <typename DestructFunc>
static void listDestroyTyped(SyList* self, size_t typeSize, size_t typeAlign,
                             DestructFunc* destructFunc) noexcept {
#ifndef NDEBUG
    if (self->capacity_ == 0) {
        sy_assert(self->data_ == nullptr, "Should have no list memory");
        sy_assert(self->allocated_ == nullptr, "Should have no list memory");
    } else {
        sy_assert(self->data_ != nullptr, "Should have list memory");
        sy_assert(self->allocated_ != nullptr, "Should have list memory");
    }
#endif
    if (self->capacity_ == 0) {
        return;
    }

    uint8_t* dataBytes = static_cast<uint8_t*>(self->data_);
    if (destructFunc != nullptr) {
        for (size_t i = 0; i < self->len; i++) {
            (*destructFunc)(&dataBytes[i * typeSize]);
        }
    }

    sy_allocator_free(&self->allocator, self->allocated_, self->capacity_ * typeSize, typeAlign);

    self->data_ = nullptr;
    self->len = 0;
    self->capacity_ = 0;
    self->allocated_ = nullptr;
}

template <typename CloneFunc, typename DestructFunc>
static SyExceptional listCloneTyped(const SyList* self, SyList* out, size_t typeSize,
                                    size_t typeAlign, CloneFunc cloneFunc,
                                    DestructFunc* destructFunc) {
    const sy::Allocator* cppallocPtr = reinterpret_cast<const sy::Allocator*>(&self->allocator);
    sy::Allocator alloc = *cppallocPtr;

    auto fullAllocationRes = alloc.allocAlignedArray<uint8_t>(typeSize * self->len, typeAlign);
    if (fullAllocationRes.hasErr()) {
        return SyExceptional::SY_EXCEPTIONAL_OOM;
    }

    auto fullAllocation = fullAllocationRes.takeValue();
    uint8_t* mem = fullAllocation.get();

    SyExceptional outErr = SyExceptional::SY_EXCEPTIONAL_NONE;
    size_t copied = 0;
    for (copied = 0; copied < self->len; copied++) {
        outErr = cloneFunc(&mem[copied * typeSize],
                           &static_cast<const uint8_t*>(self->data_)[copied * typeSize]);
        if (outErr != SyExceptional::SY_EXCEPTIONAL_NONE) { // TODO obviously this is slow, so also
                                                            // make a memcpy variant in the future
            break;
        }
    }

    if (outErr != SyExceptional::SY_EXCEPTIONAL_NONE) { // cleanup
        if (destructFunc) {
            for (size_t i = 0; i < copied; i++) {
                (*destructFunc)(&mem[i * typeSize]);
            }
        }
        return outErr;
    }

    (void)fullAllocation.take();

    out->data_ = static_cast<void*>(mem);
    out->len = self->len;
    out->capacity_ = self->len;
    out->allocated_ = static_cast<void*>(mem);
    out->allocator = self->allocator;
    return SyExceptional::SY_EXCEPTIONAL_NONE;
}

template <typename DestructFunc>
static void listRemoveAt(SyList* self, size_t index, size_t typeSize,
                         DestructFunc* destructFunc) noexcept {
    sy_assert_release(index < self->len, "Index out of bounds in List");

    uint8_t* dataBytes = static_cast<uint8_t*>(self->data_);
    if (destructFunc) {
        (*destructFunc)(&dataBytes[index * typeSize]);
    }

    if (index != (self->len - 1)) { // not the end element
        memmove(&dataBytes[index * typeSize], &dataBytes[(index + 1) * typeSize],
                self->len - 1 - index);
        return;
    }

    self->len -= 1;
}

#ifdef __cplusplus
extern "C" {
#endif

SY_API void sy_list_destroy(SyList* self, size_t typeSize, size_t typeAlign,
                            SyNativeDestructorFn typeDestruct) {
    auto destructLambda = [typeDestruct](void* obj) { typeDestruct(obj); };
    listDestroyTyped(self, typeSize, typeAlign, &destructLambda);
}

SY_API void sy_list_destroy_script(SyList* self, const SyType* dataType) {
    const sy::Type* asCppType = reinterpret_cast<const sy::Type*>(dataType);
    auto destructLambda = [dataType, asCppType](void* obj) {
        (void)asCppType->destroyUnchecked(obj);
    }; // TODO what if fail?
    listDestroyTyped(self, asCppType->byteSize(), asCppType->byteAlign(), &destructLambda);
}

SY_API SyExceptional sy_list_clone(const SyList* self, SyList* out, size_t typeSize,
                                   size_t typeAlign, SyNativeCloneFn typeClone,
                                   SyNativeDestructorFn typeDestruct) {
    auto cloneLambda = [typeClone](void* out, const void* src) { return typeClone(out, src); };
    auto destructLambda = [typeDestruct](void* obj) { typeDestruct(obj); };
    return listCloneTyped(self, out, typeSize, typeAlign, cloneLambda, &destructLambda);
}

SY_API SyExceptional sy_list_clone_script(const SyList* self, SyList* out,
                                          const struct SyType* dataType) {
    const sy::Type* asCppType = reinterpret_cast<const sy::Type*>(dataType);
    auto cloneLambda = [dataType, asCppType](void* out, const void* src) {
        auto res = asCppType->cloneUnchecked(out, src);
        if (res.hasErr()) {
            auto err = res.takeErr();
            if (auto exc = err.exceptional(); exc.hasValue()) {
                return static_cast<SyExceptional>(exc.value());
            }
            return SyExceptional::SY_EXCEPTIONAL_OTHER;
        }
        return SyExceptional::SY_EXCEPTIONAL_NONE;
    };
    auto destructLambda = [dataType, asCppType](void* obj) {
        (void)asCppType->destroyUnchecked(obj);
    }; // TODO what if fail?
    return listCloneTyped(self, out, asCppType->byteSize(), asCppType->byteAlign(), cloneLambda,
                          &destructLambda);
}

SY_API SyAllocErr sy_list_push(SyList* self, void* obj, size_t typeSize, size_t typeAlign) {
    auto res = addOneElementToListBack(self, typeSize, typeAlign);
    if (res.hasErr()) {
        return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
    }
    memcpy(res.value(), obj, typeSize);
    return SyAllocErr::SY_ALLOC_ERR_NONE;
}

SY_API SyAllocErr sy_list_push_script(SyList* self, void* obj, const SyType* dataType) {
    const sy::Type* asCppType = reinterpret_cast<const sy::Type*>(dataType);
    return sy_list_push(self, obj, asCppType->byteSize(), asCppType->byteAlign());
}

SY_API SyAllocErr sy_list_push_front(SyList* self, void* obj, size_t typeSize, size_t typeAlign) {
    auto res = addOneElementToListFront(self, typeSize, typeAlign);
    if (res.hasErr()) {
        return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
    }
    memcpy(res.value(), obj, typeSize);
    return SyAllocErr::SY_ALLOC_ERR_NONE;
}

SY_API SyAllocErr sy_list_push_front_script(SyList* self, void* obj, const SyType* dataType) {
    const sy::Type* asCppType = reinterpret_cast<const sy::Type*>(dataType);
    return sy_list_push_front(self, obj, asCppType->byteSize(), asCppType->byteAlign());
}

SY_API SyAllocErr sy_list_insert_at(SyList* self, void* obj, size_t index, size_t typeSize,
                                    size_t typeAlign) {
    auto res = addOneElementToInsertAt(self, typeSize, typeAlign, index);
    if (res.hasErr()) {
        return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
    }

    memcpy(res.value(), obj, typeSize);
    return SyAllocErr::SY_ALLOC_ERR_NONE;
}

SY_API SyAllocErr sy_list_insert_at_script(SyList* self, void* obj, size_t index,
                                           const SyType* dataType) {
    const sy::Type* asCppType = reinterpret_cast<const sy::Type*>(dataType);
    return sy_list_insert_at(self, obj, index, asCppType->byteSize(), asCppType->byteAlign());
}

SY_API void sy_list_remove_at(SyList* self, size_t index, size_t typeSize,
                              SyNativeDestructorFn typeDestruct) {
    auto destructLambda = [typeDestruct](void* obj) { typeDestruct(obj); };
    listRemoveAt(self, index, typeSize, &destructLambda);
}

SY_API void sy_list_remove_at_script(SyList* self, size_t index, const SyType* dataType) {
    const sy::Type* asCppType = reinterpret_cast<const sy::Type*>(dataType);
    auto destructLambda = [dataType, asCppType](void* obj) {
        (void)asCppType->destroyUnchecked(obj);
    }; // TODO what if fail?
    listRemoveAt(self, index, asCppType->byteSize(), &destructLambda);
}

SY_API SyAllocErr sy_list_reserve(SyList* self, size_t minCapacity, size_t typeSize,
                                  size_t typeAlign) {
    if (listCapacityBack(self, typeSize) > minCapacity) {
        return SyAllocErr::SY_ALLOC_ERR_NONE;
    }

    const size_t retainFrontPadding = extraFrontPadding(self, typeSize);

    const sy::Allocator* cppallocPtr = reinterpret_cast<const sy::Allocator*>(&self->allocator);
    sy::Allocator alloc = *cppallocPtr;

    const size_t newFullAllocationCapacity = [self, minCapacity, retainFrontPadding]() {
        const size_t potentialNewCap = capacityIncrease(retainFrontPadding + self->len);
        if (minCapacity > potentialNewCap) {
            return minCapacity;
        }
        return potentialNewCap;
    }();

    auto allocRes =
        alloc.allocAlignedArray<uint8_t>(newFullAllocationCapacity * typeSize, typeAlign);
    if (allocRes.hasErr()) {
        return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
    }
    uint8_t* mem = allocRes.takeValue().take();

    if (self->data_) {
        memcpy(&mem[retainFrontPadding * typeSize], self->data_, self->len * typeSize);
        alloc.freeAlignedArray<uint8_t>(static_cast<uint8_t*>(self->allocated_),
                                        self->capacity_ * typeSize, typeAlign);
    }

    self->data_ = static_cast<void*>(&mem[retainFrontPadding * typeSize]);
    self->allocated_ = static_cast<void*>(mem);
    self->capacity_ = newFullAllocationCapacity;
    return SyAllocErr::SY_ALLOC_ERR_NONE;
}

SY_API SyAllocErr sy_list_reserve_script(SyList* self, size_t minCapacity, const SyType* dataType) {
    const sy::Type* asCppType = reinterpret_cast<const sy::Type*>(dataType);
    return sy_list_reserve(self, minCapacity, asCppType->byteSize(), asCppType->byteAlign());
}

SY_API SyAllocErr sy_list_reserve_front(SyList* self, size_t minCapacity, size_t typeSize,
                                        size_t typeAlign) {

    if (listCapacityFront(self, typeSize) > minCapacity) {
        return SyAllocErr::SY_ALLOC_ERR_NONE;
    }

    const size_t retainBackPadding = extraBackPadding(self, typeSize);

    const sy::Allocator* cppallocPtr = reinterpret_cast<const sy::Allocator*>(&self->allocator);
    sy::Allocator alloc = *cppallocPtr;

    const size_t newFullAllocationCapacity = [self, minCapacity, retainBackPadding]() {
        const size_t potentialNewCap = capacityIncrease(retainBackPadding + self->len);
        if (minCapacity > potentialNewCap) {
            return minCapacity;
        }
        return potentialNewCap;
    }();

    auto allocRes =
        alloc.allocAlignedArray<uint8_t>(newFullAllocationCapacity * typeSize, typeAlign);
    if (allocRes.hasErr()) {
        return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
    }
    uint8_t* mem = allocRes.takeValue().take();

    // starting the new data from here should give enough front space
    const size_t startOffsetFromAllocBeginning =
        newFullAllocationCapacity - (retainBackPadding + self->len);

    if (self->data_) {
        memcpy(&mem[startOffsetFromAllocBeginning * typeSize], self->data_, self->len * typeSize);
        alloc.freeAlignedArray<uint8_t>(static_cast<uint8_t*>(self->allocated_),
                                        self->capacity_ * typeSize, typeAlign);
    }

    self->data_ = static_cast<void*>(&mem[self->len * typeSize]);
    self->allocated_ = static_cast<void*>(mem);
    self->capacity_ = newFullAllocationCapacity;
    return SyAllocErr::SY_ALLOC_ERR_NONE;
}

SY_API SyAllocErr sy_list_reserve_front_script(SyList* self, size_t minCapacity,
                                               const SyType* dataType) {
    const sy::Type* asCppType = reinterpret_cast<const sy::Type*>(dataType);
    return sy_list_reserve_front(self, minCapacity, asCppType->byteSize(), asCppType->byteAlign());
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

SY_API Result<void, Exceptional> sy_list_clone_impl(const void* srcList, void* dstList,
                                                    size_t typeSize, size_t typeAlign,
                                                    NativeCloneFn clone,
                                                    NativeDestructorFn destruct) noexcept {
    const SyList* asListSrc = static_cast<const SyList*>(srcList);
    SyList* asListDst = static_cast<SyList*>(dstList);
    auto res = sy_list_clone(asListSrc, asListDst, typeSize, typeAlign,
                             reinterpret_cast<SyNativeCloneFn>(clone), destruct);
    if (res != SyExceptional::SY_EXCEPTIONAL_NONE) {
        return Error(static_cast<Exceptional>(static_cast<int>(res)));
    }
    return {};
}

SY_API void sy_list_assert_release_index_in_range(const void* list, size_t index) noexcept {
    const SyList* asList = static_cast<const SyList*>(list);
    sy_assert_release(index < asList->len, "Index out of bounds in List");
}

SY_API Result<void*, AllocErr> sy_list_add_one_front_impl(void* list, size_t typeSize,
                                                          size_t typeAlign) noexcept {
    SyList* asList = static_cast<SyList*>(list);
    return addOneElementToListBack(asList, typeSize, typeAlign);
}

SY_API Result<void*, AllocErr> sy_list_add_one_back_impl(void* list, size_t typeSize,
                                                         size_t typeAlign) noexcept {
    SyList* asList = static_cast<SyList*>(list);
    return addOneElementToListFront(asList, typeSize, typeAlign);
}

SY_API Result<void*, AllocErr> sy_list_add_one_insert_at_impl(void* list, size_t typeSize,
                                                              size_t typeAlign,
                                                              size_t index) noexcept {
    SyList* asList = static_cast<SyList*>(list);
    return addOneElementToInsertAt(asList, typeSize, typeAlign, index);
}

SY_API void sy_list_remove_at_impl(void* list, size_t index, size_t typeSize,
                                   NativeDestructorFn destruct) noexcept {
    SyList* asList = static_cast<SyList*>(list);
    sy_list_remove_at(asList, index, typeSize, destruct);
}

SY_API Result<void, AllocErr> sy_list_reserve_back_impl(void* list, size_t minCapacity,
                                                        size_t typeSize,
                                                        size_t typeAlign) noexcept {

    SyList* asList = static_cast<SyList*>(list);
    auto res = sy_list_reserve(asList, minCapacity, typeSize, typeAlign);
    if (res == SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY) {
        return Error(AllocErr::OutOfMemory);
    }
    return {};
}

SY_API Result<void, AllocErr> sy_list_reserve_front_impl(void* list, size_t minCapacity,
                                                         size_t typeSize,
                                                         size_t typeAlign) noexcept {
    SyList* asList = static_cast<SyList*>(list);
    auto res = sy_list_reserve_front(asList, minCapacity, typeSize, typeAlign);
    if (res == SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY) {
        return Error(AllocErr::OutOfMemory);
    }
    return {};
}
} // namespace internal
} // namespace sy
