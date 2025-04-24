#include <cstdlib>
#include "os_mem.hpp"
#include <utility>

extern "C" {
    #include "allocator.h"

    SY_API void* sy_allocator_alloc(SyAllocator* self, size_t len, size_t align) {
        return self->vtable->allocFn(self->ptr, len, align);
    }

    SY_API void sy_allocator_free(SyAllocator* self, void* buf, size_t len, size_t align) {
        self->vtable->freeFn(self->ptr, buf, len, align);
    }

    SY_API void sy_allocator_destructor(SyAllocator *self) {
        self->vtable->destructorFn(self->ptr);
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

    static void default_destructor(void* self) {
        (void)self;
    }

#ifndef SY_CUSTOM_DEFAULT_ALLOCATOR
    static SyAllocatorVTable defaultVTable = {&default_alloc, &default_free, &default_destructor};
    static SyAllocator defaultAllocator = {nullptr, &defaultVTable};
    SyAllocator* const sy_defaultAllocator = &defaultAllocator;
#endif
}

#include "allocator.hpp"

sy::Allocator::Allocator() : _allocator(*sy_defaultAllocator) {}

sy::Allocator::Allocator(sy::IAllocator* ownedAllocator) {
    this->_allocator.ptr = reinterpret_cast<void*>(ownedAllocator);
    this->_allocator.vtable = &cppVTable;
}

sy::Allocator::Allocator(c::SyAllocator &&ownedAllocator) : _allocator(std::move(ownedAllocator)) 
{}

sy::Allocator::~Allocator()
{
    if(this->_allocator.ptr == nullptr) {
        return;
    }

    c::sy_allocator_destructor(&this->_allocator);
    this->_allocator.ptr = nullptr;
    this->_allocator.vtable = nullptr;
}

sy::Allocator::Allocator(Allocator &&other) : _allocator(other._allocator)
{
    other._allocator.ptr = nullptr;
    other._allocator.vtable = nullptr;
}

sy::Allocator &sy::Allocator::operator=(Allocator &&other)
{
    if(this->_allocator.ptr != nullptr) {
        c::sy_allocator_destructor(&this->_allocator);
    }

    this->_allocator = other._allocator;
    other._allocator.ptr = nullptr;
    other._allocator.vtable = nullptr;
    return *this;
}

void* sy::Allocator::cppAllocFn(void* self, size_t len, size_t align) {
    IAllocator* ialloc = reinterpret_cast<IAllocator*>(self);
    return ialloc->alloc(len, align);
}

void sy::Allocator::cppFreeFn(void *self, void *buf, size_t len, size_t align)
{
    IAllocator* ialloc = reinterpret_cast<IAllocator*>(self);
    ialloc->free(buf, len, align);
}

void sy::Allocator::cppDestructorFn(void *self)
{
    IAllocator* ialloc = reinterpret_cast<IAllocator*>(self);
    ialloc->~IAllocator();
}

sy::c::SyAllocatorVTable sy::Allocator::cppVTable = {&cppAllocFn, &cppFreeFn, &cppDestructorFn};

#ifdef SYNC_LIB_TEST

#include "../doctest.h"

using namespace sy;
using namespace sy::c;


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
    for(int i = 0; i < 10; i++) {
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
    for(int i = 0; i < 10; i++) {
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
}

static void customDestructor(CustomCAllocator* self) {
    CHECK(self->freed);
    self->ptr = nullptr;
}

static SyAllocatorVTable customVTable = {
    (sy_allocator_alloc_fn)&customAlloc, 
    (sy_allocator_free_fn)&customFree, 
    (sy_allocator_destructor_fn)&customDestructor
};

TEST_CASE("C custom allocator") {
    CustomCAllocator obj = {nullptr, false};
    SyAllocator a = {(void*)&obj, &customVTable};

    int* p = (int*)sy_allocator_alloc(&a, sizeof(int), alignof(int));
    CHECK_NE(p, nullptr);
    CHECK_NE(obj.ptr, nullptr);
    *p = 10;
    sy_allocator_free(&a, (void*)p, sizeof(int), alignof(int));
    
    CHECK_NE(obj.ptr, nullptr);
    CHECK(obj.freed);

    sy_allocator_destructor(&a);
    
    CHECK_EQ(obj.ptr, nullptr);
}

TEST_CASE("C++ default alloc/free object") {
    Allocator a;
    int* p = a.allocObject<int>();
    CHECK_NE(p, nullptr);
    *p = 10;
    a.freeObject(p);
    CHECK_EQ(p, nullptr);
}

TEST_CASE("C++ default alloc/free array") {
    Allocator a;
    int* p = a.allocArray<int>(10);
    CHECK_NE(p, nullptr);
    for(int i = 0; i < 10; i++) {
        p[i] = i;
    }
    a.freeArray(p, 10);
    CHECK_EQ(p, nullptr);
}

TEST_CASE("C++ default alloc/free aligned object") {
    Allocator a;
    int* p = a.allocAlignedObject<int>(64);
    CHECK_NE(p, nullptr);
    const size_t alignMod64 = reinterpret_cast<size_t>(p) % 64;
    CHECK_EQ(alignMod64, 0);
    *p = 10;
    a.freeObject(p);
    CHECK_EQ(p, nullptr);
}

TEST_CASE("C++ default alloc/free aligned array") {
    Allocator a;
    int* p = a.allocAlignedArray<int>(10, 64);
    CHECK_NE(p, nullptr);
    const size_t alignMod64 = reinterpret_cast<size_t>(p) % 64;
    CHECK_EQ(alignMod64, 0);
    for(int i = 0; i < 10; i++) {
        p[i] = i;
    }
    a.freeObject(p);
    CHECK_EQ(p, nullptr);
}

class CustomCppAllocator : public IAllocator {
public:
    CustomCppAllocator() = default;

    CustomCppAllocator(int s) : some(s) {}

    virtual ~CustomCppAllocator() {
        operator delete(this);
    }

    void* alloc(size_t len, size_t align) {
        this->ptr = (int*)sy_allocator_alloc(sy_defaultAllocator, len, align);
        return this->ptr;
    }

    void free(void* buf, size_t len, size_t align) {
        sy_allocator_free(sy_defaultAllocator, buf, len, align);
        this->freed = true;
    }

    public:
    int* ptr = nullptr;
    bool freed = false;
    int some = 0;
};

TEST_CASE("C++ custom allocator") {
    Allocator a = Allocator::initWith<CustomCppAllocator>();
    CustomCppAllocator* ref = reinterpret_cast<CustomCppAllocator*>(a.cAllocator().ptr);

    CHECK_EQ(ref->ptr, nullptr);
    CHECK_EQ(ref->freed, false);
    CHECK_EQ(ref->some, 0);
    
    int* p = a.allocObject<int>();
    CHECK_NE(p, nullptr);
    CHECK_NE(ref->ptr, nullptr);
    *p = 10;
    a.freeObject(p);
    CHECK_EQ(p, nullptr);  
    CHECK(ref->freed);
}

TEST_CASE("C++ custom allocator with constructor args") {
    Allocator a = Allocator::initWith<CustomCppAllocator>(5);
    CustomCppAllocator* ref = reinterpret_cast<CustomCppAllocator*>(a.cAllocator().ptr);

    CHECK_EQ(ref->ptr, nullptr);
    CHECK_EQ(ref->freed, false);
    CHECK_EQ(ref->some, 5);
}

#endif
