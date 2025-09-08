#include "dynamic_array.h"
#include "../../mem/allocator.hpp"
#include "../../program/program.hpp"
#include "../../util/assert.hpp"
#include "../../util/pow_of_2.hpp"
#include "../function/function.hpp"
#include "../type_info.hpp"
#include "dynamic_array.hpp"
#include <cstring>
#include <tuple>


static constexpr size_t initialArrayCapacity = 4;

static size_t capacityIncrease(const size_t inCapacity) {
    if (inCapacity == 0)
        return initialArrayCapacity;

    constexpr size_t lowAmount = 1024;
    // increasing by 1.5 without double conversion is n * 3 / 2.
    // simplified, it is (n * 3) >> 1;
    constexpr size_t superHighAmount = SIZE_MAX / 3;

#ifndef NDEBUG
    if (inCapacity > superHighAmount) {
        try {
            std::cerr << "DynArrayUnmanaged too big" << std::endl;
            Backtrace bt = Backtrace::generate();
            bt.print();
        } catch (...) {
        }
        std::abort();
    }
#endif

    if (inCapacity < lowAmount) {
        return inCapacity << 1;
    } else {
        return (inCapacity * 3) >> 1;
    }
}

static size_t remainingFrontCapacity(const void* data, const void* alloc, const size_t size) {
    const uintptr_t dataAsInt = reinterpret_cast<uintptr_t>(data);
    const uintptr_t allocAsInt = reinterpret_cast<uintptr_t>(alloc);
    sy_assert(dataAsInt > allocAsInt, "Invalid memory");

    const size_t difference = static_cast<size_t>(dataAsInt - allocAsInt);
    return difference / size;
}

static size_t remainingBackCapacity(const size_t len, const size_t fullCapacity, const void* data, const void* alloc,
                                    const size_t size) {
    const size_t frontCapacity = remainingFrontCapacity(data, alloc, size);
    const size_t capacityWithoutFront = fullCapacity - frontCapacity;
    sy_assert(capacityWithoutFront >= len, "Invalid inputs");
    return capacityWithoutFront - len;
}

sy::RawDynArrayUnmanaged::~RawDynArrayUnmanaged() noexcept {
// Ensure no leaks
#ifndef NDEBUG
    if (capacity_ > 0) {
        try {
            std::cerr << "DynArrayUnmanaged not properly destroyed." << std::endl;
            Backtrace bt = Backtrace::generate();
            bt.print();
        } catch (...) {
        }
        std::abort();
    }
#endif
}

void sy::RawDynArrayUnmanaged::destroy(Allocator& alloc, void (*destruct)(void* ptr), size_t size,
                                       size_t align) noexcept {
    if (this->capacity_ == 0)
        return;

    uint8_t* asBytes = reinterpret_cast<uint8_t*>(this->data_);

    for (size_t i = 0; i < this->len_; i++) {
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

void sy::RawDynArrayUnmanaged::destroyScript(Allocator& alloc, const sy::Type* typeInfo) noexcept {
    if (this->capacity_ == 0)
        return;

    uint8_t* asBytes = reinterpret_cast<uint8_t*>(this->data_);

    if (typeInfo->optionalDestructor != nullptr) {
        for (size_t i = 0; i < this->len_; i++) {
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
    : len_(other.len_), data_(other.data_), capacity_(other.capacity_), alloc_(other.alloc_) {
    other.len_ = 0;
    other.data_ = nullptr;
    other.capacity_ = 0;
    other.alloc_ = nullptr;
}

void sy::RawDynArrayUnmanaged::moveAssign(RawDynArrayUnmanaged&& other, void (*destruct)(void* ptr), Allocator& alloc,
                                          size_t size, size_t align) noexcept {
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

sy::Result<sy::RawDynArrayUnmanaged, sy::AllocErr>
sy::RawDynArrayUnmanaged::copyConstruct(const RawDynArrayUnmanaged& other, Allocator& alloc,
                                        void (*copyConstructFn)(void* dst, const void* src), size_t size,
                                        size_t align) noexcept {
    RawDynArrayUnmanaged self;
    if (other.len_ == 0) {
        return self;
    }

    auto res = alloc.allocAlignedArray<uint8_t>(other.len_ * size, align);
    if (res.hasValue() == false) {
        return Error(AllocErr::OutOfMemory);
    }

    self.len_ = other.len_;
    self.data_ = res.value(); // start without any space in the front of the array
    self.alloc_ = self.data_;
    self.capacity_ = other.len_;

    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(self.data_);
    const uint8_t* otherAsBytes = reinterpret_cast<const uint8_t*>(other.data_);

    for (size_t i = 0; i < other.len_; i++) {
        const size_t offset = i * size;
        void* dst = &selfAsBytes[offset];
        const void* src = &otherAsBytes[offset];
        copyConstructFn(dst, src);
    }

    return self;
}

sy::Result<void, sy::AllocErr> sy::RawDynArrayUnmanaged::copyAssign(const RawDynArrayUnmanaged& other, Allocator& alloc,
                                                                    void (*destruct)(void* ptr),
                                                                    void (*copyConstructFn)(void* dst, const void* src),
                                                                    size_t size, size_t align) noexcept {
    const uint8_t* otherAsBytes = reinterpret_cast<const uint8_t*>(other.data_);

    if (this->capacity_ >= other.len_) {
        uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);

        for (size_t i = 0; i < this->len_; i++) {
            const size_t offset = i * size;
            void* obj = &selfAsBytes[offset];
            destruct(obj);
        }

        // start without any space in the front of the array
        this->data_ = this->alloc_;
        selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);

        for (size_t i = 0; i < other.len_; i++) {
            const size_t offset = i * size;
            void* dst = &selfAsBytes[offset];
            const void* src = &otherAsBytes[offset];
            copyConstructFn(dst, src);
        }

        this->len_ = other.len_;

        return {};
    } else {
        auto res = alloc.allocAlignedArray<uint8_t>(other.len_ * size, align);
        if (res.hasValue() == false) {
            return Error(AllocErr::OutOfMemory);
        }

        this->destroy(alloc, destruct, size, align);
        this->len_ = other.len_;
        this->data_ = res.value(); // start without any space in the front of the array
        this->alloc_ = this->data_;
        this->capacity_ = other.len_;

        uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);

        for (size_t i = 0; i < other.len_; i++) {
            const size_t offset = i * size;
            void* dst = &selfAsBytes[offset];
            const void* src = &otherAsBytes[offset];
            copyConstructFn(dst, src);
        }
    }

    return {};
}

const void* sy::RawDynArrayUnmanaged::at(size_t index, size_t size) const {
    sy_assert(index < this->len_, "Index out of bounds");

    const size_t byteOffset = index * size;
    const uint8_t* selfAsBytes = reinterpret_cast<const uint8_t*>(this->data_);
    return &selfAsBytes[byteOffset];
}

void* sy::RawDynArrayUnmanaged::at(size_t index, size_t size) {
    sy_assert(index < this->len_, "Index out of bounds");

    const size_t byteOffset = index * size;
    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);
    return &selfAsBytes[byteOffset];
}

sy::Result<void, sy::AllocErr> sy::RawDynArrayUnmanaged::pushBack(void* element, Allocator& alloc, size_t size,
                                                                  size_t align) noexcept {
    if (remainingBackCapacity(this->len_, this->capacity_, this->data_, this->alloc_, size) == 0) {
        if (this->reallocateBack(alloc, size, align) == false) {
            return Error(AllocErr::OutOfMemory);
        }
    }

    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);
    const size_t byteOffset = this->len_ * size;
    memcpy(&selfAsBytes[byteOffset], element, size);
    this->len_ += 1;
    return {};
}

sy::Result<void, sy::AllocErr>
sy::RawDynArrayUnmanaged::pushBackCustomMove(void* element, Allocator& alloc, size_t size, size_t align,
                                             void (*moveConstructFn)(void* dst, void* src)) noexcept {
    if (remainingBackCapacity(this->len_, this->capacity_, this->data_, this->alloc_, size) == 0) {
        if (this->reallocateBackCustomMove(alloc, size, align, moveConstructFn) == false) {
            return Error(AllocErr::OutOfMemory);
        }
    }

    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);
    const size_t byteOffset = this->len_ * size;
    moveConstructFn(&selfAsBytes[byteOffset], element);
    this->len_ += 1;
    return {};
}

sy::Result<void, sy::AllocErr> sy::RawDynArrayUnmanaged::pushBackScript(void* element, Allocator& alloc,
                                                                        const Type* typeInfo) noexcept {
    if (remainingBackCapacity(this->len_, this->capacity_, this->data_, this->alloc_, typeInfo->sizeType) == 0) {
        if (this->reallocateBack(alloc, typeInfo->sizeType, typeInfo->alignType) == false) {
            return Error(AllocErr::OutOfMemory);
        }
    }

    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);
    const size_t byteOffset = this->len_ * typeInfo->sizeType;
    memcpy(&selfAsBytes[byteOffset], element, typeInfo->sizeType);
    this->len_ += 1;
    return {};
}

sy::Result<void, sy::AllocErr> sy::RawDynArrayUnmanaged::pushFront(void* element, Allocator& alloc, size_t size,
                                                                   size_t align) noexcept {
    if (remainingFrontCapacity(this->data_, this->alloc_, size) == 0) {
        if (this->reallocateFront(alloc, size, align) == false) {
            return Error(AllocErr::OutOfMemory);
        }
    }

    memcpy(this->beforeFront(size), element, size);
    this->len_ += 1;
    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);
    this->data_ = selfAsBytes - size;
    return {};
}

sy::Result<void, sy::AllocErr>
sy::RawDynArrayUnmanaged::pushFrontCustomMove(void* element, Allocator& alloc, size_t size, size_t align,
                                              void (*moveConstructFn)(void* dst, void* src)) noexcept {
    if (remainingFrontCapacity(this->data_, this->alloc_, size) == 0) {
        if (this->reallocateFrontCustomMove(alloc, size, align, moveConstructFn) == false) {
            return Error(AllocErr::OutOfMemory);
        }
    }

    moveConstructFn(this->beforeFront(size), element);
    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);
    this->len_ += 1;
    this->data_ = selfAsBytes - size;
    return {};
}

sy::Result<void, sy::AllocErr> sy::RawDynArrayUnmanaged::pushFrontScript(void* element, Allocator& alloc,
                                                                         const Type* typeInfo) noexcept {
    if (remainingFrontCapacity(this->data_, this->alloc_, typeInfo->sizeType) == 0) {
        if (this->reallocateFront(alloc, typeInfo->sizeType, typeInfo->alignType) == false) {
            return Error(AllocErr::OutOfMemory);
        }
    }

    memcpy(this->beforeFront(typeInfo->sizeType), element, typeInfo->sizeType);
    this->len_ += 1;
    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);
    this->data_ = selfAsBytes - typeInfo->sizeType;
    return {};
}

sy::Result<void, sy::AllocErr> sy::RawDynArrayUnmanaged::reallocateBack(Allocator& alloc, const size_t size,
                                                                        const size_t align) noexcept {
    const size_t newCapacity = capacityIncrease(this->capacity_);
    auto res = alloc.allocAlignedArray<uint8_t>(newCapacity * size, align);
    if (res.hasValue() == false) {
        return Error(AllocErr::OutOfMemory);
    }

    const size_t frontCapacity = remainingFrontCapacity(this->data_, this->alloc_, size);
    uint8_t* newAlloc = res.value();
    uint8_t* newData = &newAlloc[frontCapacity];

    const uint8_t* selfAsBytes = reinterpret_cast<const uint8_t*>(this->data_);
    for (size_t i = 0; i < this->len_; i++) {
        const size_t byteOffset = i * size;
        memcpy(&newData[byteOffset], &selfAsBytes[byteOffset], size);
    }

    uint8_t* selfAlloc = reinterpret_cast<uint8_t*>(this->alloc_);
    alloc.freeAlignedArray(selfAlloc, this->capacity_ * size, align);

    this->data_ = newData;
    this->alloc_ = newAlloc;
    this->capacity_ = newCapacity;
    return {};
}

sy::Result<void, sy::AllocErr>
sy::RawDynArrayUnmanaged::reallocateBackCustomMove(Allocator& alloc, const size_t size, const size_t align,
                                                   void (*moveConstructFn)(void* dst, void* src)) noexcept {
    const size_t newCapacity = capacityIncrease(this->capacity_);
    auto res = alloc.allocAlignedArray<uint8_t>(newCapacity * size, align);
    if (res.hasValue() == false) {
        return Error(AllocErr::OutOfMemory);
    }

    const size_t frontCapacity = remainingFrontCapacity(this->data_, this->alloc_, size);
    uint8_t* newAlloc = res.value();
    uint8_t* newData = &newAlloc[frontCapacity];

    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);
    for (size_t i = 0; i < this->len_; i++) {
        const size_t byteOffset = i * size;
        moveConstructFn(&newData[byteOffset], &selfAsBytes[byteOffset]);
    }

    uint8_t* selfAlloc = reinterpret_cast<uint8_t*>(this->alloc_);
    alloc.freeAlignedArray(selfAlloc, this->capacity_ * size, align);

    this->data_ = newData;
    this->alloc_ = newAlloc;
    this->capacity_ = newCapacity;
    return {};
}

sy::Result<void, sy::AllocErr> sy::RawDynArrayUnmanaged::reallocateFront(Allocator& alloc, const size_t size,
                                                                         const size_t align) noexcept {
    const size_t newCapacity = capacityIncrease(this->capacity_);
    auto res = alloc.allocAlignedArray<uint8_t>(newCapacity * size, align);
    if (res.hasValue() == false) {
        return Error(AllocErr::OutOfMemory);
    }

    const size_t frontCapacity = remainingFrontCapacity(this->data_, this->alloc_, size);
    const size_t newFrontCapacity = capacityIncrease(frontCapacity);
    uint8_t* newAlloc = res.value();
    uint8_t* newData = &newAlloc[newFrontCapacity];

    const uint8_t* selfAsBytes = reinterpret_cast<const uint8_t*>(this->data_);
    for (size_t i = 0; i < this->len_; i++) {
        const size_t byteOffset = i * size;
        memcpy(&newData[byteOffset], &selfAsBytes[byteOffset], size);
    }

    uint8_t* selfAlloc = reinterpret_cast<uint8_t*>(this->alloc_);
    alloc.freeAlignedArray(selfAlloc, this->capacity_ * size, align);

    this->data_ = newData;
    this->alloc_ = newAlloc;
    this->capacity_ = newCapacity;
    return {};
}

sy::Result<void, sy::AllocErr>
sy::RawDynArrayUnmanaged::reallocateFrontCustomMove(Allocator& alloc, const size_t size, const size_t align,
                                                    void (*moveConstructFn)(void* dst, void* src)) noexcept {
    const size_t newCapacity = capacityIncrease(this->capacity_);
    auto res = alloc.allocAlignedArray<uint8_t>(newCapacity * size, align);
    if (res.hasValue() == false) {
        return Error(AllocErr::OutOfMemory);
    }

    const size_t frontCapacity = remainingFrontCapacity(this->data_, this->alloc_, size);
    const size_t newFrontCapacity = capacityIncrease(frontCapacity);
    uint8_t* newAlloc = res.value();
    uint8_t* newData = &newAlloc[newFrontCapacity];

    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);
    for (size_t i = 0; i < this->len_; i++) {
        const size_t byteOffset = i * size;
        moveConstructFn(&newData[byteOffset], &selfAsBytes[byteOffset]);
    }

    uint8_t* selfAlloc = reinterpret_cast<uint8_t*>(this->alloc_);
    alloc.freeAlignedArray(selfAlloc, this->capacity_ * size, align);

    this->data_ = newData;
    this->alloc_ = newAlloc;
    this->capacity_ = newCapacity;
    return {};
}

void* sy::RawDynArrayUnmanaged::beforeFront(const size_t size) {
    sy_assert(remainingFrontCapacity(this->data_, this->alloc_, size) > 0,
              "Cannot access before front. Out of bounds memory.");

    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);
    return selfAsBytes - size;
}

#if SYNC_LIB_WITH_TESTS

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
