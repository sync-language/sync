#include "string.h"
#include "string.hpp"

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
