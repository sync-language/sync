#include "string.h"
#include "string.hpp"
#include "../../mem/allocator.hpp"

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

constexpr size_t SSO_CAPACITY = 3 * sizeof(void*);

struct SsoBuffer {
    char arr[SSO_CAPACITY]={0};
    SsoBuffer() = default;
};

struct HeapBuffer {
    char*   ptr = nullptr;
    size_t  capacity = 0;
    char    _unused[sizeof(void*)-1] = {0};
    char    flag = 0;
    HeapBuffer() = default;
};

static_assert(sizeof(SsoBuffer) == sizeof(HeapBuffer));
union StringImpl {
    /* data */
    SsoBuffer sso;
    HeapBuffer heap;
    StringImpl() :sso() {};
};
