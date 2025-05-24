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

//two structs, 1. SsoBuffer will have an array of 3 * size of void pointer, 2. 4 members 
//1st member character pointer, 2. size_t capacity, 3. array of characters size of void pointer - 1 ,  4. 1 char called flag 



static char* mallocHeapBuffer(size_t& inCapacity){
    sy::Allocator alloc{};

    const size_t remainder = inCapacity % STRING_ALLOC_ALIGN;
    if(remainder!=0){
        inCapacity = inCapacity + (STRING_ALLOC_ALIGN - remainder);
    }
    char* buffer = alloc.allocAlignedArray<char>(inCapacity, STRING_ALLOC_ALIGN).get();
    return buffer;
};

static void freeHeapBuffer(char* buff, size_t inCapacity){
    sy::Allocator alloc{};

    alloc.freeAlignedArray<char>(buff, inCapacity, STRING_ALLOC_ALIGN);
};


constexpr char FLAG_BIT = static_cast<char>(0b10000000);

bool sy::String::isSso()const{
    return !(this->_impl.heap.flag & FLAG_BIT);
}

void sy::String::setHeapFlag(){
    this->_impl.heap.flag = FLAG_BIT;
}

sy::String::~String() noexcept {
    if(this->isSso()) return;

    freeHeapBuffer(this->_impl.heap.ptr, this->_impl.heap.capacity);
    this->_length = 0;
    this->_impl.heap.flag = 0;
}

sy::String::String(String&& other) noexcept
    : _length(other._length)
{
    for(size_t i = 0; i < 3; i++) {
        this->_impl.raw[i] = other._impl.raw[i];
        
    }
    other._length = 0;
    other._impl.heap.flag = 0;
}

sy::String::String(const String &other) 
    : _length(other._length)
{
    if(other.isSso()){
        //copy as many as the length 
        //copy each sso manually
        for(size_t i = 0; i < _length; i++){
            _impl.sso.arr[i] = other._impl.sso.arr[i];
        }
        _impl.sso.arr[_length] = '\0';
        return;
    }
    
    //capacity is length+1 for \0
    //make a capacity variable 
    size_t cap = this->_length + 1;
    char* buffer = mallocHeapBuffer(cap);
    for(size_t i = 0; i < _length; i++) {
        buffer[i] = this->_impl.heap.ptr[i];
    }
    buffer[this->_length] = '\0';
    this->_impl.heap = HeapBuffer();
    setHeapFlag();
    this->_impl.heap.ptr = buffer;
    this->_impl.heap.capacity = cap;
}

#ifdef SYNC_LIB_TEST

#include "../../doctest.h"

using sy::String;
using sy::StringSlice;

TEST_CASE("Default constructor") {
    String s;
    CHECK_EQ(s.len(), 0);
}

#endif // SYNC_LIB_TEST
