#include "box.hpp"
#include "../../util/os_callstack.hpp"
#include "../type_info.hpp"
#include <cstring>
#include <iostream>

using sy::AllocErr;
using sy::RawBox;
using sy::Result;

Result<RawBox, AllocErr> sy::RawBox::init(Allocator alloc, void* obj, size_t size, size_t align) noexcept {
    auto result = alloc.allocAlignedArray<uint8_t>(size, align);
    if (result.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    void* mem = reinterpret_cast<void*>(result.value());
    memcpy(mem, obj, size);
    RawBox self(mem, alloc);
    return self;
}

Result<RawBox, AllocErr> sy::RawBox::initCustomMove(Allocator alloc, void* obj, size_t size, size_t align,
                                                    void (*moveConstructFn)(void* dst, void* src)) noexcept {
    auto result = alloc.allocAlignedArray<uint8_t>(size, align);
    if (result.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    void* mem = reinterpret_cast<void*>(result.value());
    moveConstructFn(mem, obj);
    RawBox self(mem, alloc);
    return self;
}

Result<RawBox, AllocErr> sy::RawBox::initScript(Allocator alloc, void* obj, const Type* typeInfo) noexcept {
    return RawBox::init(alloc, obj, typeInfo->sizeType, typeInfo->alignType);
}

sy::RawBox::~RawBox() noexcept {
    // Ensure no leaks
#ifndef NDEBUG
    if (this->obj_ != nullptr) {
        try {
            std::cerr << "Box not properly destroyed." << std::endl;
            Backtrace bt = Backtrace::generate();
            bt.print();
        } catch (...) {
        }
        std::abort();
    }
#endif
}

void sy::RawBox::destroy(void (*destruct)(void* ptr), size_t size, size_t align) noexcept {
    destruct(this->obj_);
    this->alloc_.freeAlignedArray<uint8_t>(reinterpret_cast<uint8_t*>(this->obj_), size, align);
    this->obj_ = nullptr;
}

void sy::RawBox::destroyScript(const Type* typeInfo) noexcept {
    typeInfo->destroyObject(this->obj_);
    this->alloc_.freeAlignedArray<uint8_t>(reinterpret_cast<uint8_t*>(this->obj_), typeInfo->sizeType,
                                           typeInfo->alignType);
    this->obj_ = nullptr;
}

RawBox::RawBox(RawBox&& other) noexcept : obj_(other.obj_), alloc_(other.alloc_) { other.obj_ = nullptr; }

void sy::RawBox::moveAssign(RawBox&& other, void (*destruct)(void* ptr), size_t size, size_t align) noexcept {
    destruct(this->obj_);
    this->alloc_.freeAlignedArray<uint8_t>(reinterpret_cast<uint8_t*>(this->obj_), size, align);
    this->obj_ = other.obj_;
    this->alloc_ = other.alloc_;
    other.obj_ = nullptr;
}

void sy::RawBox::moveAssignScript(RawBox&& other, const Type* typeInfo) noexcept {
    typeInfo->destroyObject(this->obj_);
    this->alloc_.freeAlignedArray<uint8_t>(reinterpret_cast<uint8_t*>(this->obj_), typeInfo->sizeType,
                                           typeInfo->alignType);
    this->obj_ = other.obj_;
    this->alloc_ = other.alloc_;
    other.obj_ = nullptr;
}

#if SYNC_LIB_WITH_TESTS

using sy::Box;

#include "../../doctest.h"

TEST_CASE("Box<int>") {
    {
        Box<int> a(5);
        CHECK_EQ(*a, 5);
        CHECK_EQ(*(a.get()), 5);
    }
    {
        Box<int> a(10, sy::Allocator());
        CHECK_EQ(*a, 10);
        CHECK_EQ(*(a.get()), 10);
    }
    {
        sy::Allocator alloc{};
        int* p = alloc.allocObject<int>().value();
        *p = 11;
        Box<int> a(p, alloc);
        CHECK_EQ(*a, 11);
        CHECK_EQ(*(a.get()), 11);
    }
}

#endif // SYNC_LIB_WITH_TESTS