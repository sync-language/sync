#include "string.h"
#include "../../core/builtin_traits/builtin_traits.hpp"
#include "../../core/core_internal.h"
#include "../../mem/allocator.hpp"
#include "../../util/move_and_leak.hpp"
#include "../function/function.hpp"
#include "../option/option.hpp"
#include "../type_info.hpp"
#include "string.hpp"
#include "string_internal.hpp"
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <string_view>

static_assert(sizeof(sy::String) == sizeof(SyString));
static_assert(alignof(sy::String) == alignof(SyString));
// Must be size and align of a normal and atomic pointer for GenPool stuff
static_assert(sizeof(sy::String) == sizeof(void*));
static_assert(alignof(sy::String) == alignof(void*));
static_assert(sizeof(sy::String) == sizeof(std::atomic<void*>));
static_assert(alignof(sy::String) == alignof(std::atomic<void*>));

static_assert(sizeof(sy::StringBuilder) == sizeof(SyStringBuilder));
static_assert(alignof(sy::StringBuilder) == alignof(SyStringBuilder));

// #if defined(__AVX512BW__)
// // _mm512_cmpeq_epi8_mask
// constexpr size_t STRING_ALLOC_ALIGN = 64;
// #elif defined(__AVX2__)
// // _mm256_cmpeq_epi8
// // _mm256_movemask_epi8
// constexpr size_t STRING_ALLOC_ALIGN = 32;
// #elif defined(__SSE2__)
// // _mm_cmpeq_epi8
// // _mm_movemask_epi8
// constexpr size_t STRING_ALLOC_ALIGN = 16;
// #elif defined(__ARM_NEON__)
// constexpr size_t STRING_ALLOC_ALIGN = 16;
// #else
// constexpr size_t STRING_ALLOC_ALIGN = alignof(void*);
// #endif

// static constexpr size_t SSO_CAPACITY = 3 * sizeof(size_t);
// // Not including null terminator
// static constexpr size_t MAX_SSO_LEN = SSO_CAPACITY - 1;
// constexpr char FLAG_BIT = static_cast<char>(0b10000000);

// struct SsoBuffer {
//     char arr[SSO_CAPACITY] = {'\0'};
//     SsoBuffer() = default;
// };

// struct AllocBuffer {
//     char* ptr = nullptr;
//     size_t capacity = 0;
//     char _unused[sizeof(size_t) - 1] = {0};
//     char flag = 0;
//     AllocBuffer() = default;
// };

// static_assert(sizeof(SsoBuffer) == sizeof(AllocBuffer));
// static_assert(sizeof(SsoBuffer) == sizeof(size_t[3]));

// static const SsoBuffer* asSso(const size_t* raw) { return reinterpret_cast<const
// SsoBuffer*>(raw); }

// static SsoBuffer* asSsoMut(size_t* raw) { return reinterpret_cast<SsoBuffer*>(raw); }

// static const AllocBuffer* asAlloc(const size_t* raw) {
//     return reinterpret_cast<const AllocBuffer*>(raw);
// }

// static AllocBuffer* asAllocMut(size_t* raw) { return reinterpret_cast<AllocBuffer*>(raw); }

// /// @param inCapacity will be rounded up to the nearest multiple of `STRING_ALLOC_ALIGN`.
// /// @return Non-zeroed memory
// sy::Result<char*, sy::AllocErr> sy::detail::mallocStringBuffer(size_t& inCapacity,
//                                                                sy::Allocator alloc) {
//     const size_t remainder = inCapacity % STRING_ALLOC_ALIGN;
//     if (remainder != 0) {
//         inCapacity = inCapacity + (STRING_ALLOC_ALIGN - remainder);
//     }
//     return alloc.allocAlignedArray<char>(inCapacity, STRING_ALLOC_ALIGN);
// }

// void sy::detail::freeStringBuffer(char* buff, size_t inCapacity, sy::Allocator alloc) {
//     alloc.freeAlignedArray<char>(buff, inCapacity, STRING_ALLOC_ALIGN);
// }

// /// Calling `memset` on the ENTIRE memory allocation could be expensive for massive strings, when
// /// setting their values to zero, as is expected of the SIMD operations on `String`, such as
// /// equality comparison. As a result, we only zero out the last bytes required of the SIMD
// element
// /// that will be read up to.
// /// @param buffer
// /// @param untouchedLength
// static void zeroSetLastSIMDElement(char* buffer, const size_t untouchedLength) {
//     const size_t remainder = untouchedLength % STRING_ALLOC_ALIGN;
//     if (remainder == 0) {
//         return;
//     }

//     const size_t end = untouchedLength + (STRING_ALLOC_ALIGN - remainder);
//     for (size_t i = untouchedLength; i < end; i++) {
//         buffer[i] = '\0';
//     }
// }

// sy::StringUnmanaged sy::detail::StringUtils::makeRaw(char*& buf, size_t length, size_t capacity,
//                                                      sy::Allocator alloc) {
//     StringUnmanaged self;
//     self.len_ = length;

//     if (length < SSO_CAPACITY) {
//         for (size_t i = 0; i < length; i++) {
//             asSsoMut(self.raw_)->arr[i] = buf[i];
//         }
//         freeStringBuffer(buf, capacity, alloc);

//         return self;
//     } else {
//         zeroSetLastSIMDElement(buf, length);
//         self.setHeapFlag();
//         AllocBuffer* thisHeap = asAllocMut(self.raw_);
//         thisHeap->ptr = buf;
//         thisHeap->capacity = capacity;
//     }

//     buf = nullptr;
//     return self;
// }

// sy::StringUnmanaged::~StringUnmanaged() noexcept {
//     sy_assert_release(this->isSso(), "StringUnmanaged leaked memory");
// }

// void sy::StringUnmanaged::destroy(Allocator& alloc) noexcept {
//     if (this->isSso())
//         return;

//     AllocBuffer* heap = asAllocMut(this->raw_);

//     sy::detail::freeStringBuffer(heap->ptr, heap->capacity, alloc);
//     this->len_ = 0;
//     heap->flag = 0;
// }

// sy::StringUnmanaged::StringUnmanaged(StringUnmanaged&& other) noexcept : len_(other.len_) {
//     for (size_t i = 0; i < 3; i++) {
//         this->raw_[i] = other.raw_[i];
//     }
//     other.len_ = 0;
//     other.setSsoFlag();
// }

// void sy::StringUnmanaged::moveAssign(StringUnmanaged&& other, Allocator& alloc) noexcept {
//     if (!isSso()) {
//         AllocBuffer* heap = asAllocMut(this->raw_);
//         sy::detail::freeStringBuffer(heap->ptr, heap->capacity, alloc);
//     }

//     this->len_ = other.len_;
//     for (size_t i = 0; i < 3; i++) {
//         this->raw_[i] = other.raw_[i];
//     }
//     other.len_ = 0;
//     other.setSsoFlag();
// }

// sy::Result<sy::StringUnmanaged, sy::AllocErr>
// sy::StringUnmanaged::copyConstruct(const StringUnmanaged& other, Allocator& alloc) noexcept {
//     sy::StringUnmanaged self;
//     self.len_ = other.len_;

//     if (other.isSso()) {
//         // This is the same as copying the SSO buffer manually, but with a linear loop, and less
//         // copies
//         for (size_t i = 0; i < 3; i++) {
//             self.raw_[i] = other.raw_[i];
//         }
//         return self;
//     }

//     // capacity is length + 1 for \0 (null terminator)
//     size_t cap = self.len_ + 1;
//     auto res = sy::detail::mallocStringBuffer(cap, alloc);
//     if (!res) {
//         return Error(AllocErr::OutOfMemory);
//     }
//     char* buffer = res.value();
//     const AllocBuffer* otherHeap = asAlloc(other.raw_);
//     for (size_t i = 0; i < self.len_; i++) {
//         buffer[i] = otherHeap->ptr[i];
//     }
//     zeroSetLastSIMDElement(buffer, self.len_);
//     buffer[self.len_] = '\0';

//     self.setHeapFlag();
//     AllocBuffer* thisHeap = asAllocMut(self.raw_);
//     thisHeap->ptr = buffer;
//     thisHeap->capacity = cap;
//     return self;
// }

// sy::Result<void, sy::AllocErr> sy::StringUnmanaged::copyAssign(const StringUnmanaged& other,
//                                                                Allocator& alloc) noexcept {
//     const size_t requiredCapacity = other.len_ + 1; // for null terminator
//     const StringSlice otherSlice = other.asSlice();

//     if (requiredCapacity < SSO_CAPACITY) {
//         if (!isSso()) {
//             // Don't bother keeping large heap allocation unnecessarily
//             AllocBuffer* heap = asAllocMut(this->raw_);
//             sy::detail::freeStringBuffer(heap->ptr, heap->capacity, alloc);
//         }
//         // All slices returned by a String object have the same alignment as `alignof(size_t)`.
//         // Furthermore all are at least 3 size_t's wide (even if the slice doesn't extend that
//         // long).
//         const size_t* asSizeTArr = reinterpret_cast<const size_t*>(otherSlice.data());
//         for (size_t i = 0; i < 3; i++) {
//             this->raw_[i] = asSizeTArr[i];
//         }
//         // We also set the flag to be as sso, as we want to avoid garbage data accidentally
//         setting
//         // the flag bit
//         this->setSsoFlag();
//         return {};
//     }

//     if (this->hasEnoughCapacity(requiredCapacity)) {
//         AllocBuffer* heap = asAllocMut(this->raw_);
//         this->len_ = other.len_;
//         for (size_t i = 0; i < other.len_; i++) {
//             heap->ptr[i] = otherSlice.data()[i];
//         }
//         zeroSetLastSIMDElement(heap->ptr, otherSlice.len());
//     } else {
//         size_t newCapacity = requiredCapacity;
//         auto res = sy::detail::mallocStringBuffer(newCapacity, alloc);
//         if (!res) {
//             return Error(AllocErr::OutOfMemory);
//         }

//         char* buffer = res.value();
//         this->len_ = other.len_;
//         AllocBuffer* heap = asAllocMut(this->raw_);
//         if (!isSso()) {
//             sy::detail::freeStringBuffer(heap->ptr, heap->capacity, alloc);
//         }

//         for (size_t i = 0; i < other.len_; i++) {
//             buffer[i] = otherSlice[i];
//         }
//         zeroSetLastSIMDElement(buffer, otherSlice.len());

//         this->setHeapFlag();
//         heap->ptr = buffer;
//         heap->capacity = newCapacity;
//     }

//     return {};
// }

// sy::Result<sy::StringUnmanaged, sy::AllocErr>
// sy::StringUnmanaged::copyConstructSlice(const StringSlice& slice, Allocator& alloc) noexcept {
//     StringUnmanaged self;
//     self.len_ = slice.len();

//     if (slice.len() <= MAX_SSO_LEN) {
//         for (size_t i = 0; i < slice.len(); i++) {
//             asSsoMut(self.raw_)->arr[i] = slice[i];
//         }
//         return self;
//     }

//     // capacity is length + 1 for \0 (null terminator)
//     size_t cap = slice.len() + 1;
//     auto res = sy::detail::mallocStringBuffer(cap, alloc);
//     if (!res) {
//         return Error(AllocErr::OutOfMemory);
//     }

//     char* buffer = res.value();
//     for (size_t i = 0; i < slice.len(); i++) {
//         buffer[i] = slice[i];
//     }
//     zeroSetLastSIMDElement(buffer, slice.len());

//     self.setHeapFlag();
//     AllocBuffer* thisHeap = asAllocMut(self.raw_);
//     thisHeap->ptr = buffer;
//     thisHeap->capacity = cap;
//     return self;
// }

// sy::Result<void, sy::AllocErr> sy::StringUnmanaged::copyAssignSlice(const StringSlice& slice,
//                                                                     Allocator& alloc) noexcept {
//     const size_t requiredCapacity = slice.len() + 1; // for null terminator

//     if (requiredCapacity < SSO_CAPACITY) {
//         if (!isSso()) {
//             // Don't bother keeping large heap allocation unnecessarily
//             AllocBuffer* heap = asAllocMut(this->raw_);
//             sy::detail::freeStringBuffer(heap->ptr, heap->capacity, alloc);
//         }

//         for (size_t i = 0; i < slice.len(); i++) {
//             asSsoMut(this->raw_)->arr[i] = slice[i];
//         }
//         // We also set the flag to be as sso, as we want to avoid garbage data accidentally
//         setting
//         // the flag bit
//         this->setSsoFlag();
//         return {};
//     }

//     if (this->hasEnoughCapacity(requiredCapacity)) {
//         AllocBuffer* heap = asAllocMut(this->raw_);
//         this->len_ = slice.len();
//         for (size_t i = 0; i < slice.len(); i++) {
//             heap->ptr[i] = slice[i];
//         }
//         zeroSetLastSIMDElement(heap->ptr, slice.len());
//     } else {
//         size_t newCapacity = requiredCapacity;
//         auto res = sy::detail::mallocStringBuffer(newCapacity, alloc);
//         if (!res) {
//             return Error(AllocErr::OutOfMemory);
//         }

//         char* buffer = res.value();
//         this->len_ = slice.len();
//         AllocBuffer* heap = asAllocMut(this->raw_);
//         if (!isSso()) {
//             sy::detail::freeStringBuffer(heap->ptr, heap->capacity, alloc);
//         }

//         for (size_t i = 0; i < slice.len(); i++) {
//             buffer[i] = slice[i];
//         }
//         zeroSetLastSIMDElement(buffer, slice.len());

//         this->setHeapFlag();
//         heap->ptr = buffer;
//         heap->capacity = newCapacity;
//     }

//     return {};
// }

// sy::Result<sy::StringUnmanaged, sy::AllocErr>
// sy::StringUnmanaged::copyConstructCStr(const char* str, Allocator& alloc) noexcept {
//     const size_t len = std::strlen(str);
//     return StringUnmanaged::copyConstructSlice(StringSlice(str, len), alloc);
// }

// sy::Result<void, sy::AllocErr> sy::StringUnmanaged::copyAssignCStr(const char* str,
//                                                                    Allocator& alloc) noexcept {
//     const size_t len = std::strlen(str);
//     return this->copyAssignSlice(StringSlice(str, len), alloc);
// }

// sy::Result<sy::StringUnmanaged, sy::AllocErr>
// sy::StringUnmanaged::fillConstruct(Allocator& alloc, size_t size, char toFill) {
//     StringUnmanaged self;
//     self.len_ = size;

//     if (size <= MAX_SSO_LEN) {
//         for (size_t i = 0; i < size; i++) {
//             asSsoMut(self.raw_)->arr[i] = toFill;
//         }
//         return self;
//     }

//     // capacity is length + 1 for \0 (null terminator)
//     size_t cap = size + 1;
//     auto res = sy::detail::mallocStringBuffer(cap, alloc);
//     if (!res) {
//         return Error(AllocErr::OutOfMemory);
//     }

//     char* buffer = res.value();
//     for (size_t i = 0; i < size; i++) {
//         buffer[i] = toFill;
//     }
//     zeroSetLastSIMDElement(buffer, size);

//     self.setHeapFlag();
//     AllocBuffer* thisHeap = asAllocMut(self.raw_);
//     thisHeap->ptr = buffer;
//     thisHeap->capacity = cap;
//     return self;
// }

// sy::StringSlice sy::StringUnmanaged::asSlice() const noexcept {
//     return StringSlice(this->cstr(), this->len_);
// }

// const char* sy::StringUnmanaged::cstr() const noexcept {
//     if (this->isSso()) {
//         return asSso(this->raw_)->arr;
//     }
//     return asAlloc(this->raw_)->ptr;
// }

// char* sy::StringUnmanaged::data() noexcept {
//     if (this->isSso()) {
//         return asSsoMut(this->raw_)->arr;
//     }
//     return asAllocMut(this->raw_)->ptr;
// }

// size_t sy::StringUnmanaged::hash() const noexcept { return this->asSlice().hash(); }

// sy::Result<void, sy::AllocErr> sy::StringUnmanaged::append(StringSlice slice,
//                                                            Allocator alloc) noexcept {
//     size_t newCapacity = this->len_ + slice.len() + 1;
//     if (!hasEnoughCapacity(newCapacity)) {
//         auto res = detail::mallocStringBuffer(newCapacity, alloc);
//         if (res.hasErr()) {
//             return Error(AllocErr::OutOfMemory);
//         }
//         if (!this->isSso()) {
//             AllocBuffer* oldHeap = asAllocMut(this->raw_);
//             detail::freeStringBuffer(oldHeap->ptr, oldHeap->capacity, alloc);
//         }
//         memcpy(res.value(), this->cstr(), this->len_);
//         this->setHeapFlag();
//         AllocBuffer* thisHeap = asAllocMut(this->raw_);
//         thisHeap->ptr = res.value();
//         thisHeap->capacity = newCapacity;
//     }

//     char* thisData = this->data();
//     memcpy(thisData + this->len_, slice.data(), slice.len());
//     this->len_ += slice.len();
//     if (!isSso()) {
//         zeroSetLastSIMDElement(thisData, this->len_);
//     }
//     return {};
// }

// bool sy::StringUnmanaged::isSso() const { return !(asAlloc(this->raw_)->flag & FLAG_BIT); }

// void sy::StringUnmanaged::setHeapFlag() {
//     AllocBuffer* h = asAllocMut(this->raw_);
//     h->flag = FLAG_BIT;
// }

// void sy::StringUnmanaged::setSsoFlag() {
//     AllocBuffer* h = asAllocMut(this->raw_);
//     h->flag = 0;
// }

// bool sy::StringUnmanaged::hasEnoughCapacity(const size_t requiredCapacity) const {
//     if (this->isSso()) {
//         return requiredCapacity <= SSO_CAPACITY;
//     } else {
//         return requiredCapacity <= asAlloc(this->raw_)->capacity;
//     }
// }

// sy::String::~String() noexcept { this->inner_.destroy(this->alloc_); }

// sy::String::String(const String& other) {
//     auto res = String::copyConstruct(other);
//     sy_assert(res.hasValue(), "Memory allocation failed");
//     new (this) String(std::move(res.takeValue()));
// }

// sy::Result<sy::String, sy::AllocErr> sy::String::copyConstruct(const String& other) {
//     Allocator alloc = other.alloc_;
//     auto result = StringUnmanaged::copyConstruct(other.inner_, alloc);
//     if (result.hasErr()) {
//         return Error(AllocErr::OutOfMemory);
//     }
//     return String(result.takeValue(), alloc);
// }

// sy::String::String(String&& other) noexcept
//     : inner_(std::move(other.inner_)), alloc_(other.alloc_) {}

// sy::String& sy::String::operator=(const String& other) {
//     auto res = this->copyAssign(other);
//     sy_assert(res.hasValue(), "Memory allocation failed");
//     return *this;
// }

// sy::Result<void, sy::AllocErr> sy::String::copyAssign(const String& other) {
//     return this->inner_.copyAssign(other.inner_, this->alloc_);
// }

// sy::String& sy::String::operator=(String&& other) noexcept {
//     this->inner_.moveAssign(std::move(other.inner_), this->alloc_);
//     return *this;
// }

// sy::String::String(const StringSlice& str) {
//     auto res = StringUnmanaged::copyConstructSlice(str, this->alloc_);
//     sy_assert(res.hasValue(), "Memory allocation failed");
//     new (&this->inner_) StringUnmanaged(std::move(res.takeValue()));
// }

// sy::Result<sy::String, sy::AllocErr> sy::String::copyConstructSlice(const StringSlice& str,
//                                                                     Allocator alloc) {
//     auto result = StringUnmanaged::copyConstructSlice(str, alloc);
//     if (result.hasErr()) {
//         return Error(AllocErr::OutOfMemory);
//     }
//     return String(result.takeValue(), alloc);
// }

// sy::String& sy::String::operator=(const StringSlice& str) {
//     auto res = this->inner_.copyAssignSlice(str, this->alloc_);
//     sy_assert(res.hasValue(), "Memory allocation failed");
//     return *this;
// }

// sy::String::String(const char* str) {
//     auto res = StringUnmanaged::copyConstructCStr(str, this->alloc_);
//     sy_assert(res.hasValue(), "Memory allocation failed");
//     new (&this->inner_) StringUnmanaged(std::move(res.takeValue()));
// }

// sy::String& sy::String::operator=(const char* str) {
//     auto res = inner_.copyAssignCStr(str, this->alloc_);
//     sy_assert(res.hasValue(), "Memory allocation failed");
//     return *this;
// }

// sy::Result<void, sy::AllocErr> sy::String::append(StringSlice slice) noexcept {
//     return this->inner_.append(slice, this->alloc_);
// }

sy::String::~String() noexcept {
    if (this->impl_ == nullptr)
        return;

    const size_t prevRefCount = this->impl_->refCount.fetch_sub(1);
    if (prevRefCount == 1) {
        Allocator alloc = this->impl_->allocator;
        const size_t totalAllocationSize = this->impl_->allocationSizeFor(this->impl_->len);
        alloc.freeAlignedArray(
            reinterpret_cast<uint8_t*>(const_cast<internal::AtomicStringHeader*>(this->impl_)),
            totalAllocationSize, ALLOC_CACHE_ALIGN);
    }

    this->impl_ = nullptr;
}

sy::String::String(const String& other) noexcept {
    if (other.impl_ == nullptr)
        return;

    const size_t prevRefCount = other.impl_->refCount.fetch_add(1);
    sy_assert(prevRefCount < (SIZE_MAX - 1), "ref count too big");
    (void)prevRefCount;
    this->impl_ = other.impl_;
}

sy::String& sy::String::operator=(const String& other) noexcept {
    if (this != &other) {
        if (this->impl_ != nullptr) {
            const size_t prevRefCount = this->impl_->refCount.fetch_sub(1);
            if (prevRefCount == 1) {
                Allocator alloc = this->impl_->allocator;
                const size_t totalAllocationSize = this->impl_->allocationSizeFor(this->impl_->len);
                alloc.freeAlignedArray(reinterpret_cast<uint8_t*>(
                                           const_cast<internal::AtomicStringHeader*>(this->impl_)),
                                       totalAllocationSize, ALLOC_CACHE_ALIGN);
            }
        }

        if (other.impl_ == nullptr) {
            this->impl_ = nullptr;
        } else {
            const size_t prevRefCount = other.impl_->refCount.fetch_add(1);
            sy_assert(prevRefCount < (SIZE_MAX - 1), "ref count too big");
            (void)prevRefCount;
            this->impl_ = other.impl_;
        }
    }

    return *this;
}

sy::String::String(String&& other) noexcept : impl_(other.impl_) { other.impl_ = nullptr; }

sy::String& sy::String::operator=(String&& other) noexcept {
    if (this != &other) {
        if (this->impl_ != nullptr) {
            const size_t prevRefCount = this->impl_->refCount.fetch_sub(1);
            if (prevRefCount == 1) {
                Allocator alloc = this->impl_->allocator;
                const size_t totalAllocationSize = this->impl_->allocationSizeFor(this->impl_->len);
                alloc.freeAlignedArray(reinterpret_cast<uint8_t*>(
                                           const_cast<internal::AtomicStringHeader*>(this->impl_)),
                                       totalAllocationSize, ALLOC_CACHE_ALIGN);
            }
        }

        this->impl_ = other.impl_;
        other.impl_ = nullptr;
    }

    return *this;
}

sy::Result<sy::String, sy::AllocErr> sy::String::init(StringSlice str, Allocator alloc) noexcept {
    if (str.len() == 0) {
        return String();
    }

    const size_t totalAllocationSize = internal::AtomicStringHeader::allocationSizeFor(str.len());

    auto res = alloc.allocAlignedArray<uint8_t>(totalAllocationSize, ALLOC_CACHE_ALIGN);
    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    uint8_t* data = res.value();
    internal::AtomicStringHeader* header = reinterpret_cast<internal::AtomicStringHeader*>(data);
    new (header) internal::AtomicStringHeader(alloc);
    header->len = str.len();

    // actually copy over the string.
    memcpy(header->inlineString, str.data(), str.len());

    // zero out the rest, setting null terminator and fun SIMD stuff.
    const size_t byteStart = internal::AtomicStringHeader::HEADER_NON_STRING_BYTES_USED + str.len();
    for (size_t i = byteStart; i < totalAllocationSize; i++) {
        data[i] = static_cast<uint8_t>('\0');
    }

    String out;
    out.impl_ = header;
    return out;
}

sy::String::String(StringSlice str, Allocator alloc) noexcept {
    auto res = String::init(str, alloc);
    sy_assert_release(res.hasValue(), "String allocation failed");
    this->impl_ = res.value().impl_;
    internal::moveAndLeak(std::move(res));
}

size_t sy::String::len() const noexcept {
    if (this->impl_ == nullptr)
        return 0;

    return this->impl_->len;
}

sy::StringSlice sy::String::asSlice() const noexcept {
    if (this->impl_ == nullptr)
        return StringSlice();

    return StringSlice(this->impl_->inlineString, this->impl_->len);
}

const char* sy::String::cstr() const noexcept {
    if (this->impl_ == nullptr)
        return nullptr;

    return this->impl_->inlineString;
}

size_t sy::String::hash() const noexcept {
    if (this->impl_ == nullptr) {
        const std::string_view empty = "";
        return std::hash<std::string_view>{}(empty);
    }

    if (this->impl_->hasHashStored.load(std::memory_order_acquire)) {
        return this->impl_->hashCode.load(std::memory_order_relaxed);
    }

    const size_t hashCode = std::hash<std::string_view>{}(
        std::string_view(this->impl_->inlineString, this->impl_->len));
    const_cast<std::atomic<size_t>*>(&this->impl_->hashCode)
        ->store(hashCode, std::memory_order_relaxed);
    const_cast<std::atomic<bool>*>(&this->impl_->hasHashStored)
        ->store(true, std::memory_order_release);
    return hashCode;
}

sy::String::operator std::string_view() const noexcept {
    StringSlice thisStr = this->asSlice();
    return std::string_view(thisStr.data(), thisStr.len());
}

bool sy::String::operator==(const String& other) const noexcept {
    // TODO simd all of this one
    if (this->impl_ == other.impl_)
        return true;

    return this->asSlice() == other.asSlice();
}

bool sy::String::operator==(StringSlice other) const noexcept { return this->asSlice() == other; }

bool sy::operator==(StringSlice lhs, const String& rhs) noexcept { return lhs == rhs.asSlice(); }

sy::Result<sy::String, sy::AllocErr> sy::String::concat(StringSlice str) const noexcept {
    StringSlice thisStr = this->asSlice();

    if ((thisStr.len() + str.len()) == 0) {
        return String();
    }

    if (str.len() == 0) {
        return *this; // copy construct
    }

    const size_t totalAllocationSize =
        internal::AtomicStringHeader::allocationSizeFor(thisStr.len() + str.len());

    auto res =
        this->impl_->allocator.allocAlignedArray<uint8_t>(totalAllocationSize, ALLOC_CACHE_ALIGN);
    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    uint8_t* data = res.value();
    internal::AtomicStringHeader* header = reinterpret_cast<internal::AtomicStringHeader*>(data);
    new (header) internal::AtomicStringHeader(this->impl_->allocator);
    header->len = thisStr.len() + str.len();

    // copy over this string data
    memcpy(header->inlineString, thisStr.data(), thisStr.len());
    // copy over other string data
    memcpy(header->inlineString + thisStr.len(), str.data(), str.len());

    // zero out the rest, setting null terminator and fun SIMD stuff.
    const size_t byteStart =
        internal::AtomicStringHeader::HEADER_NON_STRING_BYTES_USED + header->len;
    for (size_t i = byteStart; i < totalAllocationSize; i++) {
        data[i] = static_cast<uint8_t>('\0');
    }

    String out;
    out.impl_ = header;
    return out;
}

std::ostream& sy::operator<<(std::ostream& os, const String& s) {
    StringSlice thisStr = s.asSlice();
    return os.write(thisStr.data(), thisStr.len());
}

sy::StringBuilder::~StringBuilder() noexcept {
    if (this->impl_ == nullptr)
        return;

    Allocator allocator = this->impl_->allocator;
    allocator.freeAlignedArray(reinterpret_cast<uint8_t*>(this->impl_),
                               this->fullAllocatedCapacity_ +
                                   internal::AtomicStringHeader::HEADER_NON_STRING_BYTES_USED,
                               ALLOC_CACHE_ALIGN);
    this->impl_ = nullptr;
}

sy::StringBuilder::StringBuilder(StringBuilder&& other) noexcept
    : impl_(other.impl_), fullAllocatedCapacity_(other.fullAllocatedCapacity_) {
    other.impl_ = nullptr;
    other.fullAllocatedCapacity_ = 0;
}

sy::StringBuilder& sy::StringBuilder::operator=(StringBuilder&& other) noexcept {
    if (this != &other) {
        if (this->impl_ != nullptr) {
            Allocator allocator = this->impl_->allocator;
            allocator.freeAlignedArray(
                reinterpret_cast<uint8_t*>(this->impl_),
                this->fullAllocatedCapacity_ +
                    internal::AtomicStringHeader::HEADER_NON_STRING_BYTES_USED,
                ALLOC_CACHE_ALIGN);
        }

        this->impl_ = other.impl_;
        this->fullAllocatedCapacity_ = other.fullAllocatedCapacity_;
        other.impl_ = nullptr;
        other.fullAllocatedCapacity_ = 0;
    }

    return *this;
}

sy::Result<sy::StringBuilder, sy::AllocErr> sy::StringBuilder::init(Allocator alloc) noexcept {
    return StringBuilder::initWithCapacity(0, alloc);
}

sy::Result<sy::StringBuilder, sy::AllocErr>
sy::StringBuilder::initWithCapacity(size_t inCapacity, Allocator alloc) noexcept {
    const size_t totalAllocationSize = internal::AtomicStringHeader::allocationSizeFor(inCapacity);

    auto res = alloc.allocAlignedArray<uint8_t>(totalAllocationSize, ALLOC_CACHE_ALIGN);
    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    uint8_t* data = res.value();
    internal::AtomicStringHeader* header = reinterpret_cast<internal::AtomicStringHeader*>(data);
    new (header) internal::AtomicStringHeader(alloc);

    const size_t fullAllocatedStringCapacity =
        totalAllocationSize - internal::AtomicStringHeader::HEADER_NON_STRING_BYTES_USED;

    const size_t byteStart = internal::AtomicStringHeader::HEADER_NON_STRING_BYTES_USED;
    for (size_t i = byteStart; i < totalAllocationSize; i++) {
        data[i] = static_cast<uint8_t>('\0');
    }

    StringBuilder self;
    self.impl_ = header;
    self.fullAllocatedCapacity_ = fullAllocatedStringCapacity;
    return self;
}

sy::Result<sy::String, sy::AllocErr> sy::StringBuilder::build() noexcept {
    if (this->impl_->len == 0) {
        return String();
    }

    const size_t actualNeededAllocationSize =
        internal::AtomicStringHeader::allocationSizeFor(this->impl_->len);
    // subtract one cause capacity already reserves for null terminator
    const size_t currentAllocationSize = internal::AtomicStringHeader::allocationSizeFor(
        this->fullAllocatedCapacity_ + internal::AtomicStringHeader::HEADER_NON_STRING_BYTES_USED -
        1);

    String out;

    if (actualNeededAllocationSize == currentAllocationSize) {
        out.impl_ = this->impl_;
        this->impl_ = nullptr;
        return out;
    }

    auto res = this->impl_->allocator.allocAlignedArray<uint8_t>(actualNeededAllocationSize,
                                                                 ALLOC_CACHE_ALIGN);
    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    uint8_t* data = res.value();
    internal::AtomicStringHeader* header = reinterpret_cast<internal::AtomicStringHeader*>(data);
    new (header) internal::AtomicStringHeader(this->impl_->allocator);
    header->len = this->impl_->len;

    // copy over this string data
    memcpy(header->inlineString, this->impl_->inlineString, this->impl_->len);

    const size_t byteStart =
        internal::AtomicStringHeader::HEADER_NON_STRING_BYTES_USED + this->impl_->len;
    for (size_t i = byteStart; i < actualNeededAllocationSize; i++) {
        data[i] = static_cast<uint8_t>('\0');
    }

    out.impl_ = header;
    // let the StringBuilder destructor handle cleanup
    return out;
}

sy::Result<void, sy::AllocErr> sy::StringBuilder::write(StringSlice str) noexcept {
    if (str.len() == 0) {
        return {};
    }

    if ((this->impl_->len + str.len() + 1) <= this->fullAllocatedCapacity_) {
        memcpy(this->impl_->inlineString + this->impl_->len, str.data(), str.len());
        this->impl_->inlineString[this->impl_->len + str.len()] = '\0';
        this->impl_->len += str.len();
        return {};
    }

    Allocator allocator = this->impl_->allocator;

    // over-allocate
    const size_t totalAllocationSize =
        internal::AtomicStringHeader::allocationSizeFor((this->impl_->len + str.len() + 1) * 2);

    auto res = allocator.allocAlignedArray<uint8_t>(totalAllocationSize, ALLOC_CACHE_ALIGN);
    if (res.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    uint8_t* data = res.value();
    internal::AtomicStringHeader* header = reinterpret_cast<internal::AtomicStringHeader*>(data);
    new (header) internal::AtomicStringHeader(allocator);
    header->len = this->impl_->len + str.len();

    // copy over this string data
    memcpy(header->inlineString, this->impl_->inlineString, this->impl_->len);
    // copy over other string data
    memcpy(header->inlineString + this->impl_->len, str.data(), str.len());

    // zero out the rest, setting null terminator and fun SIMD stuff.
    const size_t byteStart =
        internal::AtomicStringHeader::HEADER_NON_STRING_BYTES_USED + this->impl_->len + str.len();
    for (size_t i = byteStart; i < totalAllocationSize; i++) {
        data[i] = static_cast<uint8_t>('\0');
    }

    allocator.freeAlignedArray(reinterpret_cast<uint8_t*>(this->impl_),
                               this->fullAllocatedCapacity_ +
                                   internal::AtomicStringHeader::HEADER_NON_STRING_BYTES_USED,
                               ALLOC_CACHE_ALIGN);
    this->impl_ = header;
    return {};
}

static decltype(sy::BuiltInCoherentTraits::clone) STRING_BUILTIN_COHERENT_CLONE =
    sy::BuiltInCoherentTraits::makeClone<sy::String>();
static decltype(sy::BuiltInCoherentTraits::equal) STRING_BUILTIN_COHERENT_EQUALITY =
    sy::BuiltInCoherentTraits::makeEqualityFunction<sy::String>();
static decltype(sy::BuiltInCoherentTraits::hash) STRING_BUILTIN_COHERENT_HASH =
    sy::BuiltInCoherentTraits::makeHashFunction<sy::String>();

static sy::Function<void(void* self)> STRING_BUILTIN_COHERENT_ATOMIC_DESTROY_FN = +[](void* self) {
    sy::String* tSelf = reinterpret_cast<sy::String*>(self);
    sy::internal::AtomicStringHeader::atomicStringDestroy(tSelf);
};
static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicDestroy)
    STRING_BUILTIN_COHERENT_ATOMIC_DESTROY = &STRING_BUILTIN_COHERENT_ATOMIC_DESTROY_FN;

static sy::Function<void(void* dst, const void* src)> STRING_BUILTIN_COHERENT_ATOMIC_LOAD_FN =
    +[](void* out, const void* src) {
        sy::String* tDst = reinterpret_cast<sy::String*>(out);
        const sy::String* tSrc = reinterpret_cast<const sy::String*>(src);
        sy::internal::AtomicStringHeader::atomicStringClone(tDst, tSrc);
    };
static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicLoad)
    STRING_BUILTIN_COHERENT_ATOMIC_LOAD = &STRING_BUILTIN_COHERENT_ATOMIC_LOAD_FN;

static sy::Function<void(void* dst, const void* src)> STRING_BUILTIN_COHERENT_ATOMIC_STORE_FN =
    +[](void* overwrite, const void* src) {
        sy::String* tDst = reinterpret_cast<sy::String*>(overwrite);
        const sy::String* tSrc = reinterpret_cast<const sy::String*>(src);
        sy::internal::AtomicStringHeader::atomicStringSet(tDst, tSrc);
    };
static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicStore)
    STRING_BUILTIN_COHERENT_ATOMIC_STORE = &STRING_BUILTIN_COHERENT_ATOMIC_STORE_FN;

static sy::BuiltInCoherentTraits STRING_BUILTIN_COHERENT_TRAITS = {
    STRING_BUILTIN_COHERENT_CLONE,          STRING_BUILTIN_COHERENT_EQUALITY,
    STRING_BUILTIN_COHERENT_HASH,           {},
    STRING_BUILTIN_COHERENT_ATOMIC_DESTROY, STRING_BUILTIN_COHERENT_ATOMIC_LOAD,
    STRING_BUILTIN_COHERENT_ATOMIC_STORE,
};

static sy::Function<void(void* self)> STRING_DESTRUCTOR_FN = +[](void* self) {
    sy::String* tSelf = reinterpret_cast<sy::String*>(self);
    tSelf->~String();
};

static sy::Function<void(void* self)> STRING_REF_EMPTY_DESTRUCTOR = +[](void* self) { (void)self; };

static decltype(sy::BuiltInCoherentTraits::clone) STRING_REF_BUILTIN_COHERENT_CLONE =
    sy::BuiltInCoherentTraits::makeClone<sy::String*>();
static decltype(sy::BuiltInCoherentTraits::equal) STRING_REF_BUILTIN_COHERENT_EQUALITY =
    sy::BuiltInCoherentTraits::makeEqualityFunction<sy::String*>();

static sy::BuiltInCoherentTraits STRING_REF_BUILTIN_COHERENT_TRAITS = {
    STRING_REF_BUILTIN_COHERENT_CLONE, STRING_REF_BUILTIN_COHERENT_EQUALITY, {}, {}, {}, {}, {},
};

static sy::Type STRING_TYPE;

static sy::Type STRING_CONST_REF_TYPE = {
    sizeof(const sy::String*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const sy::String*)),
    sy::StringSlice("*String"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &STRING_TYPE}),
    &STRING_REF_EMPTY_DESTRUCTOR,
    &STRING_REF_BUILTIN_COHERENT_TRAITS,
    nullptr,
    nullptr,
};

static sy::Type STRING_MUT_REF_TYPE = {
    sizeof(sy::String*),
    static_cast<decltype(sy::Type::alignType)>(alignof(sy::String*)),
    sy::StringSlice("*mut String"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{true, &STRING_TYPE}),
    &STRING_REF_EMPTY_DESTRUCTOR,
    &STRING_REF_BUILTIN_COHERENT_TRAITS,
    nullptr,
    nullptr,
};

namespace sy {
namespace internal {
sy::Type STRING_TYPE = {
    sizeof(sy::String),
    static_cast<decltype(sy::Type::alignType)>(alignof(sy::String)),
    sy::StringSlice("String"),
    sy::Type::Tag::String,
    sy::Type::ExtraInfo(),
    &STRING_DESTRUCTOR_FN,
    &STRING_BUILTIN_COHERENT_TRAITS,
    &STRING_CONST_REF_TYPE,
    &STRING_MUT_REF_TYPE,
};
}
} // namespace sy

const sy::Type* sy::Reflect<sy::String>::get() noexcept { return &sy::internal::STRING_TYPE; }

#ifdef __cplusplus
extern "C" {
#endif

SY_API SyAllocErr sy_string_init(SyStringSlice str, SyAllocator alloc, SyString* out) {
    auto res = sy::String::init(sy::StringSlice(str.ptr, str.len),
                                *reinterpret_cast<sy::Allocator*>(&alloc));
    if (res.hasErr()) {
        return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
    }

    sy::String s = res.takeValue();
    *out = *reinterpret_cast<SyString*>(&s);
    sy::internal::moveAndLeak(std::move(s));
    return SyAllocErr::SY_ALLOC_ERR_NONE;
}

SY_API size_t sy_string_len(const SyString* self) {
    return reinterpret_cast<const sy::String*>(self)->len();
}

SY_API SyStringSlice sy_string_as_slice(const SyString* self) {
    sy::StringSlice s = reinterpret_cast<const sy::String*>(self)->asSlice();
    return SyStringSlice{s.data(), s.len()};
}

SY_API const char* sy_string_cstr(const SyString* self) {
    return reinterpret_cast<const sy::String*>(self)->cstr();
}

SY_API size_t sy_string_hash(const SyString* self) {
    return reinterpret_cast<const sy::String*>(self)->hash();
}

SY_API bool sy_string_eq(const SyString* lhs, const SyString* rhs) {
    return *reinterpret_cast<const sy::String*>(lhs) == *reinterpret_cast<const sy::String*>(rhs);
}

SY_API bool sy_string_eq_slice(const SyString* lhs, SyStringSlice rhs) {
    return *reinterpret_cast<const sy::String*>(lhs) ==
           *reinterpret_cast<const sy::StringSlice*>(&rhs);
}

SY_API SyAllocErr sy_string_concat(const SyString* self, SyStringSlice str, SyString* out) {
    auto res = reinterpret_cast<const sy::String*>(self)->concat(sy::StringSlice(str.ptr, str.len));
    if (res.hasErr()) {
        return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
    }

    sy::String s = res.takeValue();
    *out = *reinterpret_cast<SyString*>(&s);
    sy::internal::moveAndLeak(std::move(s));
    return SyAllocErr::SY_ALLOC_ERR_NONE;
}

SY_API SyAllocErr sy_string_builder_init(SyAllocator alloc, SyStringBuilder* out) {
    auto res = sy::StringBuilder::init(*reinterpret_cast<sy::Allocator*>(&alloc));
    if (res.hasErr()) {
        return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
    }

    sy::StringBuilder s = res.takeValue();
    *out = *reinterpret_cast<SyStringBuilder*>(&s);
    sy::internal::moveAndLeak(std::move(s));
    return SyAllocErr::SY_ALLOC_ERR_NONE;
}

SY_API SyAllocErr sy_string_builder_init_with_capacity(size_t inCapacity, SyAllocator alloc,
                                                       SyStringBuilder* out) {
    auto res =
        sy::StringBuilder::initWithCapacity(inCapacity, *reinterpret_cast<sy::Allocator*>(&alloc));
    if (res.hasErr()) {
        return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
    }

    sy::StringBuilder s = res.takeValue();
    *out = *reinterpret_cast<SyStringBuilder*>(&s);
    sy::internal::moveAndLeak(std::move(s));
    return SyAllocErr::SY_ALLOC_ERR_NONE;
}

SY_API SyAllocErr sy_string_builder_build(SyStringBuilder* self, SyString* out) {
    auto res = reinterpret_cast<sy::StringBuilder*>(self)->build();
    if (res.hasErr()) {
        return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
    }

    sy::String s = res.takeValue();
    *out = *reinterpret_cast<SyString*>(&s);
    sy::internal::moveAndLeak(std::move(s));
    return SyAllocErr::SY_ALLOC_ERR_NONE;
}

SY_API SyAllocErr sy_string_builder_write(SyStringBuilder* self, SyStringSlice str) {
    auto res = reinterpret_cast<sy::StringBuilder*>(self)->write(sy::StringSlice(str.ptr, str.len));
    if (res.hasErr()) {
        return SyAllocErr::SY_ALLOC_ERR_OUT_OF_MEMORY;
    }

    return SyAllocErr::SY_ALLOC_ERR_NONE;
}

#ifdef __cplusplus
} // extern "C"
#endif
