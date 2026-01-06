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
#ifdef SYNC_BACKTRACE_SUPPORTED
            sy::Backtrace bt = sy::Backtrace::generate();
            bt.print();
#endif
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
    sy_assert(dataAsInt >= allocAsInt, "Invalid memory");

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
#ifdef SYNC_BACKTRACE_SUPPORTED
            Backtrace bt = Backtrace::generate();
            bt.print();
#endif
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

    if (destruct) {
        for (size_t i = 0; i < this->len_; i++) {
            const size_t offset = i * size;
            void* obj = &asBytes[offset];
            destruct(obj);
        }
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

    if (typeInfo->destructor.hasValue()) {
        for (size_t i = 0; i < this->len_; i++) {
            const size_t offset = i * typeInfo->sizeType;
            void* obj = &asBytes[offset];

            sy::RawFunction::CallArgs callArgs = typeInfo->destructor.value()->startCall();
            callArgs.push(obj, typeInfo->mutRef);
            const auto err = callArgs.call(nullptr);
            sy_assert(err.hasErr() == false, "Destructors should not fail");
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
sy::RawDynArrayUnmanaged::copyConstruct(const RawDynArrayUnmanaged& other, Allocator alloc,
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

sy::Result<void, sy::AllocErr> sy::RawDynArrayUnmanaged::push(void* element, Allocator& alloc, size_t size,
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
sy::RawDynArrayUnmanaged::pushCustomMove(void* element, Allocator& alloc, size_t size, size_t align,
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

sy::Result<void, sy::AllocErr> sy::RawDynArrayUnmanaged::pushScript(void* element, Allocator& alloc,
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

sy::Result<void, sy::AllocErr> sy::RawDynArrayUnmanaged::insertAt(void* element, Allocator& alloc, size_t index,
                                                                  size_t size, size_t align) noexcept {
    sy_assert(index < this->len_, "Index out of bounds");

    if (index == 0) {
        return this->pushFront(element, alloc, size, align);
    } else if (index == (this->len_ - 1)) {
        return this->push(element, alloc, size, align);
    }

    if (remainingBackCapacity(this->len_, this->capacity_, this->data_, this->alloc_, size) == 0) {
        if (this->reallocateBack(alloc, size, align) == false) {
            return Error(AllocErr::OutOfMemory);
        }
    }

    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);

    for (size_t i = index; i < this->len_; i++) {
        const size_t srcByteOffset = i * size;
        const size_t dstByteOffset = (i + 1) * size;
        memcpy(&selfAsBytes[dstByteOffset], &selfAsBytes[srcByteOffset], size);
    }

    const size_t newElementByteOffset = index * size;
    memcpy(&selfAsBytes[newElementByteOffset], element, size);
    this->len_ += 1;
    return {};
}

sy::Result<void, sy::AllocErr>
sy::RawDynArrayUnmanaged::insertAtCustomMove(void* element, Allocator& alloc, size_t index, size_t size, size_t align,
                                             void (*moveConstructFn)(void* dst, void* src)) noexcept {
    sy_assert(index < this->len_, "Index out of bounds");

    if (index == 0) {
        return this->pushFront(element, alloc, size, align);
    } else if (index == (this->len_ - 1)) {
        return this->push(element, alloc, size, align);
    }

    if (remainingBackCapacity(this->len_, this->capacity_, this->data_, this->alloc_, size) == 0) {
        if (this->reallocateBackCustomMove(alloc, size, align, moveConstructFn) == false) {
            return Error(AllocErr::OutOfMemory);
        }
    }

    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);

    for (size_t i = index; i < this->len_; i++) {
        const size_t srcByteOffset = i * size;
        const size_t dstByteOffset = (i + 1) * size;
        moveConstructFn(&selfAsBytes[dstByteOffset], &selfAsBytes[srcByteOffset]);
    }

    const size_t newElementByteOffset = index * size;
    moveConstructFn(&selfAsBytes[newElementByteOffset], element);
    this->len_ += 1;
    return {};
}

sy::Result<void, sy::AllocErr> sy::RawDynArrayUnmanaged::insertAtScript(void* element, Allocator& alloc, size_t index,
                                                                        const Type* typeInfo) noexcept {
    return this->insertAt(element, alloc, index, typeInfo->sizeType, typeInfo->alignType);
}

void sy::RawDynArrayUnmanaged::removeAt(size_t index, void (*destruct)(void* ptr), size_t size) noexcept {
    sy_assert(this->len_ > 0, "Nothing to remove");
    sy_assert(index < this->len_, "Index out of bounds");

    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);

    const size_t removedElementOffset = index * size;
    destruct(&selfAsBytes[removedElementOffset]);

    this->len_ -= 1;

    for (size_t i = index; i < this->len_; i++) {
        const size_t srcByteOffset = (i + 1) * size;
        const size_t dstByteOffset = i * size;
        memcpy(&selfAsBytes[dstByteOffset], &selfAsBytes[srcByteOffset], size);
    }
}

void sy::RawDynArrayUnmanaged::removeAtCustomMove(size_t index, void (*destruct)(void* ptr), size_t size,
                                                  void (*moveConstructFn)(void* dst, void* src)) noexcept {
    sy_assert(this->len_ > 0, "Nothing to remove");
    sy_assert(index < this->len_, "Index out of bounds");

    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);

    const size_t removedElementOffset = index * size;
    destruct(&selfAsBytes[removedElementOffset]);

    this->len_ -= 1;

    for (size_t i = index; i < this->len_; i++) {
        const size_t srcByteOffset = (i + 1) * size;
        const size_t dstByteOffset = i * size;
        moveConstructFn(&selfAsBytes[dstByteOffset], &selfAsBytes[srcByteOffset]);
    }
}

void sy::RawDynArrayUnmanaged::removeAtScript(size_t index, const Type* typeInfo) noexcept {
    sy_assert(this->len_ > 0, "Nothing to remove");
    sy_assert(index < this->len_, "Index out of bounds");

    uint8_t* selfAsBytes = reinterpret_cast<uint8_t*>(this->data_);

    const size_t removedElementOffset = index * typeInfo->sizeType;
    typeInfo->destroyObject(&selfAsBytes[removedElementOffset]);

    this->len_ -= 1;

    for (size_t i = (index + 1); i < this->len_; i++) {
        const size_t srcByteOffset = (i + 1) * typeInfo->sizeType;
        const size_t dstByteOffset = i * typeInfo->sizeType;
        memcpy(&selfAsBytes[dstByteOffset], &selfAsBytes[srcByteOffset], typeInfo->sizeType);
    }
}

sy::Result<void, sy::AllocErr> sy::RawDynArrayUnmanaged::reserve(Allocator& alloc, size_t minCapacity, size_t size,
                                                                 size_t align) noexcept {
    const size_t frontCapacity = remainingFrontCapacity(this->data_, this->alloc_, size);
    if (minCapacity <= (this->capacity_ + frontCapacity)) {
        return {};
    }

    const size_t newCapacity = [this, minCapacity]() {
        size_t capacityLowerBound = capacityIncrease(this->capacity_);
        if (minCapacity < capacityLowerBound)
            return capacityLowerBound;
        return minCapacity;
    }();

    auto res = alloc.allocAlignedArray<uint8_t>(newCapacity * size, align);
    if (res.hasValue() == false) {
        return Error(AllocErr::OutOfMemory);
    }

    uint8_t* newAlloc = res.value();
    uint8_t* newData = &newAlloc[frontCapacity];

    const uint8_t* selfAsBytes = reinterpret_cast<const uint8_t*>(this->data_);
    for (size_t i = 0; i < this->len_; i++) {
        const size_t byteOffset = i * size;
        memcpy(&newData[byteOffset], &selfAsBytes[byteOffset], size);
    }

    uint8_t* selfAlloc = reinterpret_cast<uint8_t*>(this->alloc_);
    if (selfAlloc != nullptr) {
        alloc.freeAlignedArray(selfAlloc, this->capacity_ * size, align);
    }

    this->data_ = newData;
    this->alloc_ = newAlloc;
    this->capacity_ = newCapacity;
    return {};
}

bool sy::RawDynArrayUnmanaged::Iterator::operator!=(const Iterator& other) noexcept {
    return this->current != other.current;
}

void* sy::RawDynArrayUnmanaged::Iterator::operator*() const noexcept { return current; }

sy::RawDynArrayUnmanaged::Iterator& sy::RawDynArrayUnmanaged::Iterator::operator++() noexcept {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(this->current);
    this->current = ptr - this->size;
    return *this;
}

sy::RawDynArrayUnmanaged::Iterator sy::RawDynArrayUnmanaged::begin(size_t size) noexcept {
    return Iterator{this->data_, size};
}

sy::RawDynArrayUnmanaged::Iterator sy::RawDynArrayUnmanaged::end(size_t size) noexcept {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(this->data_);
    void* endPtr = ptr + (size * this->len_);
    return Iterator{endPtr, size};
}

bool sy::RawDynArrayUnmanaged::ConstIterator::operator!=(const ConstIterator& other) noexcept {
    return this->current != other.current;
}

const void* sy::RawDynArrayUnmanaged::ConstIterator::operator*() const noexcept { return current; }

sy::RawDynArrayUnmanaged::ConstIterator& sy::RawDynArrayUnmanaged::ConstIterator::operator++() noexcept {
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(this->current);
    this->current = ptr - this->size;
    return *this;
}

sy::RawDynArrayUnmanaged::ConstIterator sy::RawDynArrayUnmanaged::begin(size_t size) const noexcept {
    return ConstIterator{this->data_, size};
}

sy::RawDynArrayUnmanaged::ConstIterator sy::RawDynArrayUnmanaged::end(size_t size) const noexcept {
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(this->data_);
    const void* endPtr = ptr + (size * this->len_);
    return ConstIterator{endPtr, size};
}

bool sy::RawDynArrayUnmanaged::ReverseIterator::operator!=(const ReverseIterator& other) noexcept {
    return this->current != other.current;
}

void* sy::RawDynArrayUnmanaged::ReverseIterator::operator*() const noexcept { return current; }

sy::RawDynArrayUnmanaged::ReverseIterator& sy::RawDynArrayUnmanaged::ReverseIterator::operator++() noexcept {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(this->current);
    this->current = ptr - this->size;
    return *this;
}

sy::RawDynArrayUnmanaged::ReverseIterator sy::RawDynArrayUnmanaged::rbegin(size_t size) noexcept {
    if (this->data_ == nullptr) {
        return ReverseIterator{nullptr, size};
    }
    uint8_t* ptr = reinterpret_cast<uint8_t*>(this->data_);
    void* startPtr = ptr + (size * (this->len_ - 1));
    return ReverseIterator{startPtr, size};
}

sy::RawDynArrayUnmanaged::ReverseIterator sy::RawDynArrayUnmanaged::rend(size_t size) noexcept {
    if (this->data_ == nullptr) {
        return ReverseIterator{nullptr, size};
    }
    uint8_t* ptr = reinterpret_cast<uint8_t*>(this->data_);
    void* endPtr = ptr - size;
    return ReverseIterator{endPtr, size};
}

bool sy::RawDynArrayUnmanaged::ReverseConstIterator::operator!=(const ReverseConstIterator& other) noexcept {
    return this->current != other.current;
}

const void* sy::RawDynArrayUnmanaged::ReverseConstIterator::operator*() const noexcept { return current; }

sy::RawDynArrayUnmanaged::ReverseConstIterator& sy::RawDynArrayUnmanaged::ReverseConstIterator::operator++() noexcept {
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(this->current);
    this->current = ptr - this->size;
    return *this;
}

sy::RawDynArrayUnmanaged::ReverseConstIterator sy::RawDynArrayUnmanaged::rbegin(size_t size) const noexcept {
    if (this->data_ == nullptr) {
        return ReverseConstIterator{nullptr, size};
    }
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(this->data_);
    const void* startPtr = ptr + (size * (this->len_ - 1));
    return ReverseConstIterator{startPtr, size};
}

sy::RawDynArrayUnmanaged::ReverseConstIterator sy::RawDynArrayUnmanaged::rend(size_t size) const noexcept {
    if (this->data_ == nullptr) {
        return ReverseConstIterator{nullptr, size};
    }
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(this->data_);
    const void* endPtr = ptr - size;
    return ReverseConstIterator{endPtr, size};
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

void SY_API sy::detail::dynArrayDebugAssertNoErr(bool hasErr) { sy_assert(!hasErr, "Expected no dynamic array error"); }

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
