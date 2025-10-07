#include "string.h"
#include "../../mem/allocator.hpp"
#include "../../util/assert.hpp"
#include "../../util/os_callstack.hpp"
#include "string.hpp"
#include <cstdlib>
#include <cstring>

#if defined(__AVX512BW__)
// _mm512_cmpeq_epi8_mask
constexpr size_t STRING_ALLOC_ALIGN = 64;
#elif defined(__AVX2__)
// _mm256_cmpeq_epi8
// _mm256_movemask_epi8
constexpr size_t STRING_ALLOC_ALIGN = 32;
#elif defined(__SSE2__)
// _mm_cmpeq_epi8
// _mm_movemask_epi8
constexpr size_t STRING_ALLOC_ALIGN = 16;
#elif defined(__ARM_NEON__)
constexpr size_t STRING_ALLOC_ALIGN = 16;
#else
constexpr size_t STRING_ALLOC_ALIGN = alignof(void*);
#endif

static constexpr size_t SSO_CAPACITY = 3 * sizeof(size_t);
// Not including null terminator
static constexpr size_t MAX_SSO_LEN = SSO_CAPACITY - 1;
constexpr char FLAG_BIT = static_cast<char>(0b10000000);

struct SsoBuffer {
    char arr[SSO_CAPACITY] = {'\0'};
    SsoBuffer() = default;
};

struct AllocBuffer {
    char* ptr = nullptr;
    size_t capacity = 0;
    char _unused[sizeof(size_t) - 1] = {0};
    char flag = 0;
    AllocBuffer() = default;
};

static_assert(sizeof(SsoBuffer) == sizeof(AllocBuffer));
static_assert(sizeof(SsoBuffer) == sizeof(size_t[3]));

static const SsoBuffer* asSso(const size_t* raw) { return reinterpret_cast<const SsoBuffer*>(raw); }

static SsoBuffer* asSsoMut(size_t* raw) { return reinterpret_cast<SsoBuffer*>(raw); }

static const AllocBuffer* asAlloc(const size_t* raw) { return reinterpret_cast<const AllocBuffer*>(raw); }

static AllocBuffer* asAllocMut(size_t* raw) { return reinterpret_cast<AllocBuffer*>(raw); }

/// @param inCapacity will be rounded up to the nearest multiple of `STRING_ALLOC_ALIGN`.
/// @return Non-zeroed memory
sy::Result<char*, sy::AllocErr> sy::detail::mallocStringBuffer(size_t& inCapacity, sy::Allocator alloc) {
    const size_t remainder = inCapacity % STRING_ALLOC_ALIGN;
    if (remainder != 0) {
        inCapacity = inCapacity + (STRING_ALLOC_ALIGN - remainder);
    }
    return alloc.allocAlignedArray<char>(inCapacity, STRING_ALLOC_ALIGN);
}

void sy::detail::freeStringBuffer(char* buff, size_t inCapacity, sy::Allocator alloc) {
    alloc.freeAlignedArray<char>(buff, inCapacity, STRING_ALLOC_ALIGN);
}

/// Calling `memset` on the ENTIRE memory allocation could be expensive for massive strings, when setting their values
/// to zero, as is expected of the SIMD operations on `String`, such as equality comparison. As a result, we only zero
/// out the last bytes required of the SIMD element that will be read up to.
/// @param buffer
/// @param untouchedLength
static void zeroSetLastSIMDElement(char* buffer, const size_t untouchedLength) {
    const size_t remainder = untouchedLength % STRING_ALLOC_ALIGN;
    if (remainder == 0) {
        return;
    }

    const size_t end = untouchedLength + (STRING_ALLOC_ALIGN - remainder);
    for (size_t i = untouchedLength; i < end; i++) {
        buffer[i] = '\0';
    }
}

sy::StringUnmanaged sy::detail::StringUtils::makeRaw(char*& buf, size_t length, size_t capacity, sy::Allocator alloc) {
    StringUnmanaged self;
    self.len_ = length;

    if (length < SSO_CAPACITY) {
        for (size_t i = 0; i < length; i++) {
            asSsoMut(self.raw_)->arr[i] = buf[i];
        }
        freeStringBuffer(buf, capacity, alloc);

        return self;
    } else {
        zeroSetLastSIMDElement(buf, length);
        self.setHeapFlag();
        AllocBuffer* thisHeap = asAllocMut(self.raw_);
        thisHeap->ptr = buf;
        thisHeap->capacity = capacity;
    }

    buf = nullptr;
    return self;
}

sy::StringUnmanaged::~StringUnmanaged() noexcept {
// Ensure no leaks
#ifndef NDEBUG
    if (this->isSso() == false) {
        try {
            std::cerr << "StringUnmanaged not properly destroyed." << std::endl;
#if SYNC_BACKTRACE_SUPPORTED
            sy::Backtrace bt = sy::Backtrace::generate();
            bt.print();
#endif
        } catch (...) {
        }
        std::abort();
    }
#endif
}

void sy::StringUnmanaged::destroy(Allocator& alloc) noexcept {
    if (this->isSso())
        return;

    AllocBuffer* heap = asAllocMut(this->raw_);

    sy::detail::freeStringBuffer(heap->ptr, heap->capacity, alloc);
    this->len_ = 0;
    heap->flag = 0;
}

sy::StringUnmanaged::StringUnmanaged(StringUnmanaged&& other) noexcept : len_(other.len_) {
    for (size_t i = 0; i < 3; i++) {
        this->raw_[i] = other.raw_[i];
    }
    other.len_ = 0;
    other.setSsoFlag();
}

void sy::StringUnmanaged::moveAssign(StringUnmanaged&& other, Allocator& alloc) noexcept {
    if (!isSso()) {
        AllocBuffer* heap = asAllocMut(this->raw_);
        sy::detail::freeStringBuffer(heap->ptr, heap->capacity, alloc);
    }

    this->len_ = other.len_;
    for (size_t i = 0; i < 3; i++) {
        this->raw_[i] = other.raw_[i];
    }
    other.len_ = 0;
    asAllocMut(this->raw_)->flag = 0;
}

sy::Result<sy::StringUnmanaged, sy::AllocErr> sy::StringUnmanaged::copyConstruct(const StringUnmanaged& other,
                                                                                 Allocator& alloc) noexcept {
    sy::StringUnmanaged self;
    self.len_ = other.len_;

    if (other.isSso()) {
        // This is the same as copying the SSO buffer manually, but with a linear loop, and less copies
        for (size_t i = 0; i < 3; i++) {
            self.raw_[i] = other.raw_[i];
        }
        return self;
    }

    // capacity is length + 1 for \0 (null terminator)
    size_t cap = self.len_ + 1;
    auto res = sy::detail::mallocStringBuffer(cap, alloc);
    if (!res) {
        return Error(AllocErr::OutOfMemory);
    }
    char* buffer = res.value();
    const AllocBuffer* otherHeap = asAlloc(other.raw_);
    for (size_t i = 0; i < self.len_; i++) {
        buffer[i] = otherHeap->ptr[i];
    }
    zeroSetLastSIMDElement(buffer, self.len_);

    self.setHeapFlag();
    AllocBuffer* thisHeap = asAllocMut(self.raw_);
    thisHeap->ptr = buffer;
    thisHeap->capacity = cap;
    return self;
}

sy::Result<void, sy::AllocErr> sy::StringUnmanaged::copyAssign(const StringUnmanaged& other,
                                                               Allocator& alloc) noexcept {
    const size_t requiredCapacity = other.len_ + 1; // for null terminator
    const StringSlice otherSlice = other.asSlice();

    if (requiredCapacity < SSO_CAPACITY) {
        if (!isSso()) {
            // Don't bother keeping large heap allocation unnecessarily
            AllocBuffer* heap = asAllocMut(this->raw_);
            sy::detail::freeStringBuffer(heap->ptr, heap->capacity, alloc);
        }
        // All slices returned by a String object have the same alignment as `alignof(size_t)`.
        // Furthermore all are at least 3 size_t's wide (even if the slice doesn't extend that long).
        const size_t* asSizeTArr = reinterpret_cast<const size_t*>(otherSlice.data());
        for (size_t i = 0; i < 3; i++) {
            this->raw_[i] = asSizeTArr[i];
        }
        // We also set the flag to be as sso, as we want to avoid garbage data accidentally setting the flag bit
        this->setSsoFlag();
        return {};
    }

    if (this->hasEnoughCapacity(requiredCapacity)) {
        AllocBuffer* heap = asAllocMut(this->raw_);
        this->len_ = other.len_;
        for (size_t i = 0; i < other.len_; i++) {
            heap->ptr[i] = otherSlice.data()[i];
        }
        zeroSetLastSIMDElement(heap->ptr, otherSlice.len());
    } else {
        size_t newCapacity = requiredCapacity;
        auto res = sy::detail::mallocStringBuffer(newCapacity, alloc);
        if (!res) {
            return Error(AllocErr::OutOfMemory);
        }

        char* buffer = res.value();
        this->len_ = other.len_;
        AllocBuffer* heap = asAllocMut(this->raw_);
        if (!isSso()) {
            sy::detail::freeStringBuffer(heap->ptr, heap->capacity, alloc);
        }

        for (size_t i = 0; i < other.len_; i++) {
            buffer[i] = otherSlice[i];
        }
        zeroSetLastSIMDElement(buffer, otherSlice.len());

        this->setHeapFlag();
        heap->ptr = buffer;
        heap->capacity = newCapacity;
    }

    return {};
}

sy::Result<sy::StringUnmanaged, sy::AllocErr> sy::StringUnmanaged::copyConstructSlice(const StringSlice& slice,
                                                                                      Allocator& alloc) noexcept {
    StringUnmanaged self;
    self.len_ = slice.len();

    if (slice.len() <= MAX_SSO_LEN) {
        for (size_t i = 0; i < slice.len(); i++) {
            asSsoMut(self.raw_)->arr[i] = slice[i];
        }
        return self;
    }

    // capacity is length + 1 for \0 (null terminator)
    size_t cap = slice.len() + 1;
    auto res = sy::detail::mallocStringBuffer(cap, alloc);
    if (!res) {
        return Error(AllocErr::OutOfMemory);
    }

    char* buffer = res.value();
    for (size_t i = 0; i < slice.len(); i++) {
        buffer[i] = slice[i];
    }
    zeroSetLastSIMDElement(buffer, slice.len());

    self.setHeapFlag();
    AllocBuffer* thisHeap = asAllocMut(self.raw_);
    thisHeap->ptr = buffer;
    thisHeap->capacity = cap;
    return self;
}

sy::Result<void, sy::AllocErr> sy::StringUnmanaged::copyAssignSlice(const StringSlice& slice,
                                                                    Allocator& alloc) noexcept {
    const size_t requiredCapacity = slice.len() + 1; // for null terminator

    if (requiredCapacity < SSO_CAPACITY) {
        if (!isSso()) {
            // Don't bother keeping large heap allocation unnecessarily
            AllocBuffer* heap = asAllocMut(this->raw_);
            sy::detail::freeStringBuffer(heap->ptr, heap->capacity, alloc);
        }

        for (size_t i = 0; i < slice.len(); i++) {
            asSsoMut(this->raw_)->arr[i] = slice[i];
        }
        // We also set the flag to be as sso, as we want to avoid garbage data accidentally setting the flag bit
        this->setSsoFlag();
        return {};
    }

    if (this->hasEnoughCapacity(requiredCapacity)) {
        AllocBuffer* heap = asAllocMut(this->raw_);
        this->len_ = slice.len();
        for (size_t i = 0; i < slice.len(); i++) {
            heap->ptr[i] = slice[i];
        }
        zeroSetLastSIMDElement(heap->ptr, slice.len());
    } else {
        size_t newCapacity = requiredCapacity;
        auto res = sy::detail::mallocStringBuffer(newCapacity, alloc);
        if (!res) {
            return Error(AllocErr::OutOfMemory);
        }

        char* buffer = res.value();
        this->len_ = slice.len();
        AllocBuffer* heap = asAllocMut(this->raw_);
        if (!isSso()) {
            sy::detail::freeStringBuffer(heap->ptr, heap->capacity, alloc);
        }

        for (size_t i = 0; i < slice.len(); i++) {
            buffer[i] = slice[i];
        }
        zeroSetLastSIMDElement(buffer, slice.len());

        this->setHeapFlag();
        heap->ptr = buffer;
        heap->capacity = newCapacity;
    }

    return {};
}

sy::Result<sy::StringUnmanaged, sy::AllocErr> sy::StringUnmanaged::copyConstructCStr(const char* str,
                                                                                     Allocator& alloc) noexcept {
    const size_t len = std::strlen(str);
    return StringUnmanaged::copyConstructSlice(StringSlice(str, len), alloc);
}

sy::Result<void, sy::AllocErr> sy::StringUnmanaged::copyAssignCStr(const char* str, Allocator& alloc) noexcept {
    const size_t len = std::strlen(str);
    return this->copyAssignSlice(StringSlice(str, len), alloc);
}

sy::Result<sy::StringUnmanaged, sy::AllocErr> sy::StringUnmanaged::fillConstruct(Allocator& alloc, size_t size,
                                                                                 char toFill) {
    StringUnmanaged self;
    self.len_ = size;

    if (size <= MAX_SSO_LEN) {
        for (size_t i = 0; i < size; i++) {
            asSsoMut(self.raw_)->arr[i] = toFill;
        }
        return self;
    }

    // capacity is length + 1 for \0 (null terminator)
    size_t cap = size + 1;
    auto res = sy::detail::mallocStringBuffer(cap, alloc);
    if (!res) {
        return Error(AllocErr::OutOfMemory);
    }

    char* buffer = res.value();
    for (size_t i = 0; i < size; i++) {
        buffer[i] = toFill;
    }
    zeroSetLastSIMDElement(buffer, size);

    self.setHeapFlag();
    AllocBuffer* thisHeap = asAllocMut(self.raw_);
    thisHeap->ptr = buffer;
    thisHeap->capacity = cap;
    return self;
}

sy::StringSlice sy::StringUnmanaged::asSlice() const noexcept { return StringSlice(this->cstr(), this->len_); }

const char* sy::StringUnmanaged::cstr() const noexcept {
    if (this->isSso()) {
        return asSso(this->raw_)->arr;
    }
    return asAlloc(this->raw_)->ptr;
}

char* sy::StringUnmanaged::data() noexcept {
    if (this->isSso()) {
        return asSsoMut(this->raw_)->arr;
    }
    return asAllocMut(this->raw_)->ptr;
}

size_t sy::StringUnmanaged::hash() const noexcept { return this->asSlice().hash(); }

sy::Result<void, sy::AllocErr> sy::StringUnmanaged::append(StringSlice slice, Allocator alloc) noexcept {
    size_t newCapacity = this->len_ + slice.len() + 1;
    if (!hasEnoughCapacity(newCapacity)) {
        auto res = detail::mallocStringBuffer(newCapacity, alloc);
        if (res.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        memcpy(res.value(), this->cstr(), this->len_);
        this->setHeapFlag();
        AllocBuffer* thisHeap = asAllocMut(this->raw_);
        thisHeap->ptr = res.value();
        thisHeap->capacity = newCapacity;
    }

    char* thisData = this->data();
    memcpy(thisData + this->len_, slice.data(), slice.len());
    this->len_ += slice.len();
    if (!isSso()) {
        zeroSetLastSIMDElement(thisData, slice.len());
    }
    return {};
}

bool sy::StringUnmanaged::isSso() const { return !(asAlloc(this->raw_)->flag & FLAG_BIT); }

void sy::StringUnmanaged::setHeapFlag() {
    AllocBuffer* h = asAllocMut(this->raw_);
    h->flag = FLAG_BIT;
}

void sy::StringUnmanaged::setSsoFlag() {
    AllocBuffer* h = asAllocMut(this->raw_);
    h->flag = 0;
}

bool sy::StringUnmanaged::hasEnoughCapacity(const size_t requiredCapacity) const {
    if (this->isSso()) {
        return requiredCapacity <= SSO_CAPACITY;
    } else {
        return requiredCapacity <= asAlloc(this->raw_)->capacity;
    }
}

sy::String::~String() noexcept { this->inner_.destroy(this->alloc_); }

sy::String::String(const String& other) {
    auto res = String::copyConstruct(other);
    sy_assert(res.hasValue(), "Memory allocation failed");
    new (this) String(std::move(res.takeValue()));
}

sy::Result<sy::String, sy::AllocErr> sy::String::copyConstruct(const String& other) {
    Allocator alloc = other.alloc_;
    auto result = StringUnmanaged::copyConstruct(other.inner_, alloc);
    if (result.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }
    return String(result.takeValue(), alloc);
}

sy::String::String(String&& other) noexcept : inner_(std::move(other.inner_)), alloc_(other.alloc_) {}

sy::String& sy::String::operator=(const String& other) {
    auto res = this->copyAssign(other);
    sy_assert(res.hasValue(), "Memory allocation failed");
    return *this;
}

sy::Result<void, sy::AllocErr> sy::String::copyAssign(const String& other) {
    return this->inner_.copyAssign(other.inner_, this->alloc_);
}

sy::String& sy::String::operator=(String&& other) noexcept {
    this->inner_.moveAssign(std::move(other.inner_), this->alloc_);
    return *this;
}

sy::String::String(const StringSlice& str) {
    auto res = StringUnmanaged::copyConstructSlice(str, this->alloc_);
    sy_assert(res.hasValue(), "Memory allocation failed");
    new (&this->inner_) StringUnmanaged(std::move(res.takeValue()));
}

sy::Result<sy::String, sy::AllocErr> sy::String::copyConstructSlice(const StringSlice& str, Allocator alloc) {
    auto result = StringUnmanaged::copyConstructSlice(str, alloc);
    if (result.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }
    return String(result.takeValue(), alloc);
}

sy::String& sy::String::operator=(const StringSlice& str) {
    auto res = this->inner_.copyAssignSlice(str, this->alloc_);
    sy_assert(res.hasValue(), "Memory allocation failed");
    return *this;
}

sy::String::String(const char* str) {
    auto res = StringUnmanaged::copyConstructCStr(str, this->alloc_);
    sy_assert(res.hasValue(), "Memory allocation failed");
    new (&this->inner_) StringUnmanaged(std::move(res.takeValue()));
}

sy::String& sy::String::operator=(const char* str) {
    auto res = inner_.copyAssignCStr(str, this->alloc_);
    sy_assert(res.hasValue(), "Memory allocation failed");
    return *this;
}

sy::Result<void, sy::AllocErr> sy::String::append(StringSlice slice) noexcept {
    return this->inner_.append(slice, this->alloc_);
}

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"

using sy::String;
using sy::StringSlice;

TEST_CASE("Default constructor") {
    String s;
    CHECK_EQ(s.len(), 0);
}

TEST_SUITE("const char* constructor") {
    TEST_CASE("small") {
        String s = String("hello");
        CHECK_EQ(s.len(), 5);
        CHECK_EQ(std::strcmp(s.cstr(), "hello"), 0);
    }

    TEST_CASE("max sso len 64 bit arch") {
        String s = String("hello world how are you");
        CHECK_EQ(s.len(), 23);
        CHECK_EQ(std::strcmp(s.cstr(), "hello world how are you"), 0);
    }

    TEST_CASE("bigger than sso len") {
        String s = String("hello world how are you today");
        CHECK_EQ(s.len(), 29);
        CHECK_EQ(std::strcmp(s.cstr(), "hello world how are you today"), 0);
    }

    TEST_CASE("massive string") {
        String s = String("12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012"
                          "34567890123456789012345678901234567890");
        CHECK_EQ(s.len(), 130);
        CHECK_EQ(std::strcmp(s.cstr(), "1234567890123456789012345678901234567890123456789012345678901234567890123456789"
                                       "012345678901234567890123456789012345678901234567890"),
                 0);
    }
}

#endif // SYNC_LIB_WITH_TESTS
