#include "string.h"
#include "string.hpp"
#include "../../mem/allocator.hpp"
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

static_assert(STRING_ALLOC_ALIGN);

/// @param inCapacity will be rounded up to the nearest multiple of `STRING_ALLOC_ALIGN`.
/// @return Non-zeroed memory
static char* mallocHeapBuffer(size_t& inCapacity){
    sy::Allocator alloc{};

    const size_t remainder = inCapacity % STRING_ALLOC_ALIGN;
    if(remainder != 0){
        inCapacity = inCapacity + (STRING_ALLOC_ALIGN - remainder);
    }
    char* buffer = alloc.allocAlignedArray<char>(inCapacity, STRING_ALLOC_ALIGN).get();
    return buffer;
}

static void freeHeapBuffer(char* buff, size_t inCapacity){
    sy::Allocator alloc{};
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

constexpr char FLAG_BIT = static_cast<char>(0b10000000);

sy::String::~String() noexcept {
    if(this->isSso()) return;

    HeapBuffer* heap = this->asHeap();

    freeHeapBuffer(heap->ptr, heap->capacity);
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
    char* buffer = mallocHeapBuffer(cap);
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
            freeHeapBuffer(heap->ptr, heap->capacity);
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
            freeHeapBuffer(heap->ptr, heap->capacity);
        }

        // Allocate for a new buffer that is big enough here
        size_t newCapacity = requiredCapacity;
        char* buffer = mallocHeapBuffer(newCapacity);
        const HeapBuffer* otherHeap = other.asHeap();
        for(size_t i = 0; i < _length; i++) {
            buffer[i] = otherSlice.data()[i];
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
        freeHeapBuffer(heap->ptr, heap->capacity);
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
    char* buffer = mallocHeapBuffer(cap);
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
            freeHeapBuffer(heap->ptr, heap->capacity);
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
            freeHeapBuffer(heap->ptr, heap->capacity);
        }

        // Allocate for a new buffer that is big enough here
        size_t newCapacity = requiredCapacity;
        char* buffer = mallocHeapBuffer(newCapacity);
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

#ifdef SYNC_LIB_TEST

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

#endif // SYNC_LIB_TEST
