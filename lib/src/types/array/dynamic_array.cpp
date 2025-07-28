#include "dynamic_array.h"
#include "dynamic_array.hpp"
#include "../../mem/allocator.hpp"
#include "../../util/assert.hpp"
#include "../type_info.hpp"
#include "../function/function.hpp"
#include "../../program/program.hpp"

sy::RawDynArrayUnmanaged::~RawDynArrayUnmanaged() noexcept
{
    // Ensure no leaks
    #if _DEBUG
    if(capacity > 0) {
        try {
            std::cerr << "DynArrayUnmanaged not properly destroyed." << std::endl;
            Backtrace bt = Backtrace::generate();
            bt.print();
        } catch(...) {}
        std::abort();
    }
    #endif
}

void sy::RawDynArrayUnmanaged::destroy(Allocator &alloc, void (*destruct)(void *ptr), size_t size, size_t align) noexcept
{
    if(this->capacity_ == 0) return;

    uint8_t* asBytes = reinterpret_cast<uint8_t*>(this->data_);

    for(size_t i = 0; i < this->len_; i++) {
        const size_t offset = i * size;
        void* obj = &asBytes[offset];
        destruct(obj);
    }

    alloc.freeAlignedArray(asBytes, this->capacity_ * size, align);

    this->len_ = 0;
    this->data_ = nullptr;
    this->capacity_ = 0;
    this->alloc_ = nullptr;
}

void sy::RawDynArrayUnmanaged::destroyScript(Allocator &alloc, const sy::Type *typeInfo) noexcept
{    
    if(this->capacity_ == 0) return;

    uint8_t* asBytes = reinterpret_cast<uint8_t*>(this->data_);

    if(typeInfo->optionalDestructor != nullptr) {
        for(size_t i = 0; i < this->len_; i++) {
            const size_t offset = i * typeInfo->sizeType;
            void* obj = &asBytes[offset];

            sy::Function::CallArgs callArgs = typeInfo->optionalDestructor->startCall();
            callArgs.push(obj, typeInfo->mutRef);
            const sy::ProgramRuntimeError err = callArgs.call(nullptr);
            sy_assert(err.ok(), "Destructors should not fail");
        }     
    }

    alloc.freeAlignedArray(asBytes, this->capacity_ * typeInfo->sizeType, typeInfo->alignType);

    this->len_ = 0;
    this->data_ = nullptr;
    this->capacity_ = 0;
    this->alloc_ = nullptr;
}

sy::RawDynArrayUnmanaged::RawDynArrayUnmanaged(RawDynArrayUnmanaged&& other) noexcept
    : len_(other.len_), data_(other.data_), capacity_(other.capacity_), alloc_(other.alloc_)
{   
    other.len_ = 0;
    other.data_ = nullptr;
    other.capacity_ = 0;
    other.alloc_ = nullptr;
}

void sy::RawDynArrayUnmanaged::moveAssign(
    RawDynArrayUnmanaged&& other,
    void (*destruct)(void *ptr),
    Allocator& alloc,
    size_t size,
    size_t align
) noexcept
{
    this->destroy(alloc, destruct, size, align);

    this->len_ = other.len_;
    this->data_ = other.data_;
    this->capacity_ = other.capacity_;
    this->alloc_ = other.alloc_;
    other.len_ = 0;
    other.data_ = nullptr;
    other.capacity_ = 0;
    other.alloc_ = nullptr;
}

#ifndef SYNC_LIB_NO_TESTS

#include "../../doctest.h"

using sy::DynArray;

TEST_CASE("default construction") {
    DynArray<size_t> arr;
    CHECK_EQ(arr.len(), 0);
}

TEST_SUITE("push const T&") {
    TEST_CASE("push 1") {
        DynArray<size_t> arr;
        const size_t element = 2;
        arr.push(element); 

        CHECK_EQ(arr.len(), 1);
        CHECK_EQ(arr[0], element);
    }


    TEST_CASE("push 2") {
        DynArray<size_t> arr;
        const size_t element1 = 5;
        const size_t element2 = 10;
        arr.push(element1); 
        arr.push(element2); 

        CHECK_EQ(arr.len(), 2);
        CHECK_EQ(arr[0], element1);
        CHECK_EQ(arr[1], element2);
    }
}

TEST_SUITE("push move T&&") {
    TEST_CASE("push 1") {
        DynArray<size_t> arr;
        arr.push(2); 

        CHECK_EQ(arr.len(), 1);
        CHECK_EQ(arr[0], 2);
    }


    TEST_CASE("push 2") {
        DynArray<size_t> arr;
        arr.push(5); 
        arr.push(10); 

        CHECK_EQ(arr.len(), 2);
        CHECK_EQ(arr[0], 5);
        CHECK_EQ(arr[1], 10);
    }
}

#endif // SYNC_LIB_NO_TESTS
