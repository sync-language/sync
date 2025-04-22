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
        return sy::aligned_malloc(len, align);
    }

    static void default_free(void* self, void* buf, size_t len, size_t align) {
        (void)self;
        (void)len;
        (void)align;
        sy::aligned_free(buf);
    }

    static void default_destructor(void* self) {
        (void)self;
    }

    static SyAllocatorVTable defaultVTable = {&default_alloc, &default_free, &default_destructor};
    static SyAllocator defaultAllocator = {nullptr, &defaultVTable};
    SyAllocator* sy_defaultAllocator = &defaultAllocator;
}

#include "allocator.hpp"

sy::Allocator::Allocator() : _allocator(defaultAllocator) {}

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