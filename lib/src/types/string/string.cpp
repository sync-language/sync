#include "string.h"
#include "string.hpp"
#include "../../mem/allocator.hpp"
#include <cstring>
#include <cstdlib>
#include "../../util/os_callstack.hpp"

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
    char arr[SSO_CAPACITY] = { '\0' };
    SsoBuffer() = default;
};

struct AllocBuffer {
    char*   ptr = nullptr;
    size_t  capacity = 0;
    char    _unused[sizeof(size_t) - 1] = { 0 };
    char    flag = 0;
    AllocBuffer() = default;
};

static_assert(sizeof(SsoBuffer) == sizeof(AllocBuffer));
static_assert(sizeof(SsoBuffer) == sizeof(size_t[3]));

// static const SsoBuffer* asSso(const size_t* raw) {
//     return reinterpret_cast<const SsoBuffer*>(raw);
// }

// static SsoBuffer* asSsoMut(size_t* raw) {
//     return reinterpret_cast<SsoBuffer*>(raw);
// }

static const AllocBuffer* asAlloc(const size_t* raw) {
    return reinterpret_cast<const AllocBuffer*>(raw);
}

static AllocBuffer* asAllocMut(size_t* raw) {
    return reinterpret_cast<AllocBuffer*>(raw);
}

/// @param inCapacity will be rounded up to the nearest multiple of `STRING_ALLOC_ALIGN`.
/// @return Non-zeroed memory
static sy::AllocExpect<char*> mallocHeapBuffer(size_t& inCapacity, sy::Allocator alloc) {
    const size_t remainder = inCapacity % STRING_ALLOC_ALIGN;
    if(remainder != 0){
        inCapacity = inCapacity + (STRING_ALLOC_ALIGN - remainder);
    }
    auto res = alloc.allocAlignedArray<char>(inCapacity, STRING_ALLOC_ALIGN);
    return sy::AllocExpect<char*>(res.get());
}

static void freeHeapBuffer(char* buff, size_t inCapacity, sy::Allocator alloc) {
    alloc.freeAlignedArray<char>(buff, inCapacity, STRING_ALLOC_ALIGN);
}

/// Calling `memset` on the ENTIRE memory allocation could be expensive for massive strings, when setting their values
/// to zero, as is expected of the SIMD operations on `String`, such as equality comparison. As a result, we only zero
/// out the last bytes required of the SIMD element that will be read up to.
/// @param buffer 
/// @param untouchedLength 
static void zeroSetLastSIMDElement(char* buffer, const size_t untouchedLength) {
    const size_t remainder = untouchedLength % STRING_ALLOC_ALIGN;
    if(remainder == 0) {
        return;
    }

    const size_t end = untouchedLength + (STRING_ALLOC_ALIGN - remainder);
    for(size_t i = untouchedLength; i < end; i++) {
        buffer[i] = '\0';
    }
}

sy::StringUnmanaged::~StringUnmanaged() noexcept
{
    // Ensure no leaks
    #if _DEBUG
    if(this->isSso() == false) {
        try {
            std::cerr << "StringUnmanaged not properly destroyed." << std::endl;
            Backtrace bt = Backtrace::generate();
            bt.print();
        } catch(...) {}
        std::abort();
    }
    #endif
}

void sy::StringUnmanaged::destroy(Allocator& alloc) noexcept
{
    if(this->isSso()) return;

    AllocBuffer* heap = asAllocMut(this->raw_);

    freeHeapBuffer(heap->ptr, heap->capacity, alloc);
    this->len_ = 0;
    heap->flag = 0;
}

sy::StringUnmanaged::StringUnmanaged(StringUnmanaged &&other) noexcept
    : len_(other.len_)
{
    for(size_t i = 0; i < 3; i++) {
        this->raw_[i] = other.raw_[i];
    }
    other.len_ = 0;
    asAllocMut(this->raw_)->flag = 0;
}

void sy::StringUnmanaged::moveAssign(StringUnmanaged &&other, Allocator& alloc) noexcept
{
    if(!isSso()) {
        AllocBuffer* heap = asAllocMut(this->raw_);
        freeHeapBuffer(heap->ptr, heap->capacity, alloc);
    }

    this->len_ = other.len_;
    for(size_t i = 0; i < 3; i++) {
        this->raw_[i] = other.raw_[i];   
    }
    other.len_ = 0;
    asAllocMut(this->raw_)->flag = 0;
}

sy::AllocExpect<sy::StringUnmanaged> sy::StringUnmanaged::copyConstruct(const StringUnmanaged &other, Allocator &alloc)
{
    sy::StringUnmanaged self;
    self.len_ = other.len_;

    if(other.isSso()) {
        // This is the same as copying the SSO buffer manually, but with a linear loop, and less copies
        for(size_t i = 0; i < 3; i++){
            self.raw_[i] = other.raw_[i];
        }
        return AllocExpect<StringUnmanaged>();
    }
    
    //capacity is length + 1 for \0 (null terminator)
    size_t cap = self.len_ + 1;
    auto res = mallocHeapBuffer(cap, alloc);
    char* buffer = res.value();
    const AllocBuffer* otherHeap = asAlloc(other.raw_);
    for(size_t i = 0; i < self.len_; i++) {
        buffer[i] = otherHeap->ptr[i];
    }
    zeroSetLastSIMDElement(buffer, self.len_);

    self.setHeapFlag();
    AllocBuffer* thisHeap = asAllocMut(self.raw_);
    thisHeap->ptr = buffer;
    thisHeap->capacity = cap;
    return AllocExpect<StringUnmanaged>(std::move(self));
}

bool sy::StringUnmanaged::isSso() const
{
    return !(asAlloc(this->raw_)->flag & FLAG_BIT);
}

void sy::StringUnmanaged::setHeapFlag()
{
    AllocBuffer* h = asAllocMut(this->raw_);
    h->flag = FLAG_BIT;
}

void sy::StringUnmanaged::setSsoFlag()
{
    AllocBuffer* h = asAllocMut(this->raw_);
    h->flag = 0;
}


sy::String::~String() noexcept {
    if(this->isSso()) return;

    HeapBuffer* heap = this->asHeap();

    (void)freeHeapBuffer(heap->ptr, heap->capacity, Allocator());
    this->_length = 0;
    heap->flag = 0;
}

sy::String::String(const String &other) 
    : _length(other._length)
{
    if(other.isSso()) {
        // This is the same as copying the SSO buffer manually, but with a linear loop, and less copies
        for(size_t i = 0; i < 3; i++){
            this->_raw[i] = other._raw[i];
        }
        return;
    }
    
    //capacity is length + 1 for \0 (null terminator)
    size_t cap = this->_length + 1;
    char* buffer = mallocHeapBuffer(cap, Allocator()).value();
    const HeapBuffer* otherHeap = other.asHeap();
    for(size_t i = 0; i < _length; i++) {
        buffer[i] = otherHeap->ptr[i];
    }
    zeroSetLastSIMDElement(buffer, this->_length);

    this->setHeapFlag();
    HeapBuffer* thisHeap = this->asHeap();
    thisHeap->ptr = buffer;
    thisHeap->capacity = cap;
}

sy::String::String(String&& other) noexcept
    : _length(other._length)
{
    for(size_t i = 0; i < 3; i++) {
        this->_raw[i] = other._raw[i];
    }
    other._length = 0;
    this->asHeap()->flag = 0;
}

sy::String& sy::String::operator=(const String& other) {
    this->_length = other._length;
    const size_t requiredCapacity = other._length + 1; // for null terminator
    const StringSlice otherSlice = other.asSlice();

    if(requiredCapacity < SSO_CAPACITY) {
        if(!isSso()) {
            // Don't bother keeping large heap allocation unnecessarily
            HeapBuffer* heap = this->asHeap();
            (void)freeHeapBuffer(heap->ptr, heap->capacity, Allocator());
        }
        // All slices returned by a String object have the same alignment as `alignof(size_t)`.
        // Furthermore all are at least 3 size_t's wide (even if the slice doesn't extend that long).
        const size_t* asSizeTArr = reinterpret_cast<const size_t*>(otherSlice.data());
        for(size_t i = 0; i < 3; i++) {
            this->_raw[i] = asSizeTArr[i];
        }
        // We also set the flag to be as sso, as we want to avoid garbage data accidentally setting the flag bit
        this->setSsoFlag();
        return *this;
    }
    
    // We have determined now that the string to copy exceeds the sso buffer
    // If there is enough capacity already in the existing string allocation, we should just use it
    if(this->hasEnoughCapacity(requiredCapacity)) {
        HeapBuffer* heap = this->asHeap();
        for(size_t i = 0; i < other._length; i++) {
            heap->ptr[i] = otherSlice.data()[i];
        }
        zeroSetLastSIMDElement(heap->ptr, otherSlice.len());
    } else {
        HeapBuffer* heap = asHeap();
        if(!isSso()) {
            // The old buffer isn't big enough, so we free it
            (void)freeHeapBuffer(heap->ptr, heap->capacity, Allocator());
        }

        // Allocate for a new buffer that is big enough here
        size_t newCapacity = requiredCapacity;
        char* buffer = mallocHeapBuffer(newCapacity, Allocator()).value();
        for(size_t i = 0; i < _length; i++) {
            buffer[i] = otherSlice[i];
        }
        zeroSetLastSIMDElement(buffer, otherSlice.len());
       
        this->setHeapFlag();
        HeapBuffer* thisHeap = this->asHeap();
        thisHeap->ptr = buffer;
        thisHeap->capacity = newCapacity;
    }

    return *this;
}

sy::String& sy::String::operator=(String&& other) noexcept {
    if(!isSso()) {
        HeapBuffer* heap = asHeap();
        (void)freeHeapBuffer(heap->ptr, heap->capacity, Allocator());
    }

    this->_length = other._length;
    for(size_t i = 0; i < 3; i++) {
        this->_raw[i] = other._raw[i];   
    }
    other._length = 0;
    this->asHeap()->flag = 0;
    return *this;
}

sy::String::String(const StringSlice &str)
    : _length(str.len())
{
    if(str.len() <= MAX_SSO_LEN) {
        for(size_t i = 0; i < str.len(); i++){
            this->asSso()->arr[i] = str[i];
        }
        return;
    }

    //capacity is length + 1 for \0 (null terminator)
    size_t cap = this->_length + 1;
    char* buffer = mallocHeapBuffer(cap, Allocator()).value();
    for(size_t i = 0; i < _length; i++) {
        buffer[i] = str[i];
    }
    zeroSetLastSIMDElement(buffer, this->_length);

    this->setHeapFlag();
    HeapBuffer* thisHeap = this->asHeap();
    thisHeap->ptr = buffer;
    thisHeap->capacity = cap;
}

sy::String& sy::String::operator=(const StringSlice& str) {
    this->_length = str.len();
    const size_t requiredCapacity = str.len() + 1; // for null terminator

    if(requiredCapacity < SSO_CAPACITY) {
        if(!isSso()) {
            // Don't bother keeping large heap allocation unnecessarily
            HeapBuffer* heap = this->asHeap();
            (void)freeHeapBuffer(heap->ptr, heap->capacity, Allocator());
        }

        for(size_t i = 0; i < str.len(); i++) {
            this->asSso()->arr[i] = str[i];
        }
        // We also set the flag to be as sso, as we want to avoid garbage data accidentally setting the flag bit
        this->setSsoFlag();
        return *this;
    }

    // We have determined now that the string to copy exceeds the sso buffer
    // If there is enough capacity already in the existing string allocation, we should just use it
    if(this->hasEnoughCapacity(requiredCapacity)) {
        HeapBuffer* heap = this->asHeap();
        for(size_t i = 0; i < str.len(); i++) {
            heap->ptr[i] = str[i];
        }
        zeroSetLastSIMDElement(heap->ptr, str.len());
    } else {
        HeapBuffer* heap = asHeap();
        if(!isSso()) {
            // The old buffer isn't big enough, so we free it
            (void)freeHeapBuffer(heap->ptr, heap->capacity, Allocator());
        }

        // Allocate for a new buffer that is big enough here
        size_t newCapacity = requiredCapacity;
        char* buffer = mallocHeapBuffer(newCapacity, Allocator()).value();
        for(size_t i = 0; i < _length; i++) {
            buffer[i] = str[i];
        }
        zeroSetLastSIMDElement(buffer, str.len());
       
        this->setHeapFlag();
        HeapBuffer* thisHeap = this->asHeap();
        thisHeap->ptr = buffer;
        thisHeap->capacity = newCapacity;
    }

    return *this;
}

sy::String::String(const char* str) 
    : String(StringSlice(str, std::strlen(str)))
{}

sy::String& sy::String::operator=(const char* str) {
    const size_t len = std::strlen(str);
    StringSlice strSlice(str, len);
    return String::operator=(strSlice);
}

sy::StringSlice sy::String::asSlice() const
{
    return StringSlice(this->cstr(), this->_length);
}

const char *sy::String::cstr() const
{
    if(this->isSso()) {
        return this->asSso()->arr;
    }
    return this->asHeap()->ptr;
}

bool sy::String::isSso() const
{
    return !(this->asHeap()->flag & FLAG_BIT);
}

void sy::String::setHeapFlag() {
    HeapBuffer* h = this->asHeap();
    h->flag = FLAG_BIT;
}

void sy::String::setSsoFlag()
{    
    HeapBuffer* h = this->asHeap();
    h->flag = 0;
}

const sy::String::SsoBuffer *sy::String::asSso() const {
    return reinterpret_cast<const SsoBuffer*>(this->_raw);
}

sy::String::SsoBuffer *sy::String::asSso() {
    return reinterpret_cast<SsoBuffer*>(this->_raw);
}

const sy::String::HeapBuffer *sy::String::asHeap() const {
    return reinterpret_cast<const HeapBuffer*>(this->_raw);
}

sy::String::HeapBuffer *sy::String::asHeap() {
    return reinterpret_cast<HeapBuffer*>(this->_raw);
}

bool sy::String::hasEnoughCapacity(const size_t requiredCapacity) const
{
    if(this->isSso()) {
        return requiredCapacity <= SSO_CAPACITY;
    } else {
        return requiredCapacity <= this->asHeap()->capacity;
    }
}

#ifndef SYNC_LIB_NO_TESTS

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
        String s = String("1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890");
        CHECK_EQ(s.len(), 130);
        CHECK_EQ(std::strcmp(s.cstr(), "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"), 0);
    }
}

#endif // SYNC_LIB_NO_TESTS
