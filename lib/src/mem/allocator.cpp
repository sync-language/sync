#include "allocator.h"
#include "../core/core_internal.h"
#include "allocator.hpp"
#include "os_mem.hpp"
#include <cstdlib>
#include <iostream>
#include <utility>

static_assert(sizeof(sy::Allocator) == sizeof(SyAllocator));
static_assert(alignof(sy::Allocator) == alignof(SyAllocator));
static_assert(sizeof(sy::Allocator::VTable) == sizeof(SyAllocatorVTable));
static_assert(alignof(sy::Allocator::VTable) == alignof(SyAllocatorVTable));
static_assert(offsetof(sy::Allocator::VTable, allocFn) == offsetof(SyAllocatorVTable, allocFn));
static_assert(offsetof(sy::Allocator::VTable, freeFn) == offsetof(SyAllocatorVTable, freeFn));

extern "C" {
SY_API void* sy_allocator_alloc(SyAllocator* self, size_t len, size_t align) {
    return self->vtable->allocFn(self->ptr, len, align);
}

SY_API void sy_allocator_free(SyAllocator* self, void* buf, size_t len, size_t align) {
    self->vtable->freeFn(self->ptr, buf, len, align);
}

static void* default_alloc(void* self, size_t len, size_t align) {
    (void)self;
    return aligned_malloc(len, align);
}

static void default_free(void* self, void* buf, size_t len, size_t align) {
    (void)self;
    (void)len;
    (void)align;
    aligned_free(buf);
}

#ifndef SY_CUSTOM_DEFAULT_ALLOCATOR
static SyAllocatorVTable defaultVTable = {&default_alloc, &default_free};
static SyAllocator defaultAllocator = {nullptr, &defaultVTable};
SyAllocator* const sy_defaultAllocator = &defaultAllocator;
#endif
}

sy::Allocator sy::IAllocator::asAllocator() {
    static const Allocator::VTable vtable = {reinterpret_cast<Allocator::VTable::alloc_fn>(IAllocator::allocImpl),
                                             reinterpret_cast<Allocator::VTable::free_fn>(IAllocator::freeImpl)};
    Allocator a;
    a.ptr_ = reinterpret_cast<void*>(this);
    a.vtable_ = &vtable;
    return a;
}

void* sy::IAllocator::allocImpl(IAllocator* self, size_t len, size_t align) noexcept { return self->alloc(len, align); }

void sy::IAllocator::freeImpl(IAllocator* self, void* buf, size_t len, size_t align) noexcept {
    self->free(buf, len, align);
}

sy::Allocator::Allocator() {
    static_assert(offsetof(Allocator, ptr_) == offsetof(SyAllocator, ptr));
    static_assert(offsetof(Allocator, vtable_) == offsetof(SyAllocator, vtable));

    std::atomic_thread_fence(std::memory_order_acquire);
    *this = *reinterpret_cast<Allocator*>(sy_defaultAllocator);
    std::atomic_thread_fence(std::memory_order_release);
}

void* sy::Allocator::allocImpl(size_t len, size_t align) noexcept {
    return sy_allocator_alloc(reinterpret_cast<SyAllocator*>(this), len, align);
}

void sy::Allocator::freeImpl(void* buf, size_t len, size_t align) noexcept {
    sy_allocator_free(reinterpret_cast<SyAllocator*>(this), buf, len, align);
}

void sy::detail::debugAssertNonNull(void* ptr) noexcept { sy_assert(ptr != nullptr, "Expected non-null pointer"); }

void sy::detail::debugAssertHasVal(bool hasVal) noexcept {
    sy_assert(hasVal, "Expected allocator error result object to have a value");
}

#if SYNC_LIB_WITH_TESTS

#include "../doctest.h"

using namespace sy;

TEST_CASE("C default alloc/free object") {
    Allocator a;
    int* p = (int*)sy_allocator_alloc(sy_defaultAllocator, sizeof(int), alignof(int));
    CHECK_NE(p, nullptr);
    *p = 10;
    sy_allocator_free(sy_defaultAllocator, (void*)p, sizeof(int), alignof(int));
}

TEST_CASE("C default alloc/free array") {
    Allocator a;
    int* p = (int*)sy_allocator_alloc(sy_defaultAllocator, sizeof(int) * 10, alignof(int));
    CHECK_NE(p, nullptr);
    for (int i = 0; i < 10; i++) {
        p[i] = i;
    }
    sy_allocator_free(sy_defaultAllocator, (void*)p, sizeof(int) * 10, alignof(int));
}

TEST_CASE("C default alloc/free aligned object") {
    Allocator a;
    int* p = (int*)sy_allocator_alloc(sy_defaultAllocator, sizeof(int), 64);
    CHECK_NE(p, nullptr);
    const size_t alignMod64 = reinterpret_cast<size_t>(p) % 64;
    CHECK_EQ(alignMod64, 0);
    *p = 10;
    sy_allocator_free(sy_defaultAllocator, (void*)p, sizeof(int), 64);
}

TEST_CASE("C default alloc/free aligned array") {
    Allocator a;
    int* p = (int*)sy_allocator_alloc(sy_defaultAllocator, sizeof(int) * 10, 64);
    CHECK_NE(p, nullptr);
    const size_t alignMod64 = reinterpret_cast<size_t>(p) % 64;
    CHECK_EQ(alignMod64, 0);
    for (int i = 0; i < 10; i++) {
        p[i] = i;
    }
    sy_allocator_free(sy_defaultAllocator, (void*)p, sizeof(int) * 10, 64);
}

struct CustomCAllocator {
    int* ptr;
    bool freed;
};

static void* customAlloc(CustomCAllocator* self, size_t len, size_t align) {
    self->ptr = (int*)sy_allocator_alloc(sy_defaultAllocator, len, align);
    return self->ptr;
}

static void customFree(CustomCAllocator* self, void* buf, size_t len, size_t align) {
    sy_allocator_free(sy_defaultAllocator, buf, len, align);
    self->freed = true;
    self->ptr = nullptr;
}

static SyAllocatorVTable customVTable = {(sy_allocator_alloc_fn)&customAlloc, (sy_allocator_free_fn)&customFree};

TEST_CASE("C custom allocator") {
    CustomCAllocator obj = {nullptr, false};
    SyAllocator a = {(void*)&obj, &customVTable};

    int* p = (int*)sy_allocator_alloc(&a, sizeof(int), alignof(int));
    CHECK_NE(p, nullptr);
    CHECK_NE(obj.ptr, nullptr);
    *p = 10;
    sy_allocator_free(&a, (void*)p, sizeof(int), alignof(int));

    CHECK(obj.freed);
}

TEST_CASE("C++ default alloc/free object") {
    Allocator a;
    int* p = a.allocObject<int>().value();
    CHECK_NE(p, nullptr);
    *p = 10;
    a.freeObject(p);
}

TEST_CASE("C++ default alloc/free array") {
    Allocator a;
    int* p = a.allocArray<int>(10).value();
    CHECK_NE(p, nullptr);
    for (int i = 0; i < 10; i++) {
        p[i] = i;
    }
    a.freeArray(p, 10);
}

TEST_CASE("C++ default alloc/free aligned object") {
    Allocator a;
    int* p = a.allocAlignedObject<int>(64).value();
    CHECK_NE(p, nullptr);
    const size_t alignMod64 = reinterpret_cast<size_t>(p) % 64;
    CHECK_EQ(alignMod64, 0);
    *p = 10;
    a.freeObject(p);
}

TEST_CASE("C++ default alloc/free aligned array") {
    Allocator a;
    int* p = a.allocAlignedArray<int>(10, 64).value();
    CHECK_NE(p, nullptr);
    const size_t alignMod64 = reinterpret_cast<size_t>(p) % 64;
    CHECK_EQ(alignMod64, 0);
    for (int i = 0; i < 10; i++) {
        p[i] = i;
    }
    a.freeObject(p);
}

class CustomCppAllocator : public IAllocator {
  public:
    CustomCppAllocator() = default;

    CustomCppAllocator(int s) : some(s) {}

    void* alloc(size_t len, size_t align) noexcept {
        this->ptr = (int*)sy_allocator_alloc(sy_defaultAllocator, len, align);
        return this->ptr;
    }

    void free(void* buf, size_t len, size_t align) noexcept {
        sy_allocator_free(sy_defaultAllocator, buf, len, align);
        this->freed = true;
    }

  public:
    int* ptr = nullptr;
    bool freed = false;
    int some = 0;
};

TEST_CASE("C++ custom allocator") {
    CustomCppAllocator backingAllocator{};
    Allocator a = backingAllocator.asAllocator();

    CHECK_EQ(backingAllocator.ptr, nullptr);
    CHECK_EQ(backingAllocator.freed, false);
    CHECK_EQ(backingAllocator.some, 0);

    int* p = a.allocObject<int>().value();
    CHECK_NE(p, nullptr);
    CHECK_NE(backingAllocator.ptr, nullptr);
    *p = 10;
    a.freeObject(p);
    CHECK(backingAllocator.freed);
}

#endif // SYNC_LIB_NO_TESTS
