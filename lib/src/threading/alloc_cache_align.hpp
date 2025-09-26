#pragma once
#ifndef SY_THREADING_ALLOC_CACHE_ALIGN
#define SY_THREADING_ALLOC_CACHE_ALIGN

#include "../core.h"

#if _MSC_VER
#include <new>
constexpr size_t ALLOC_CACHE_ALIGN = std::hardware_destructive_interference_size;
#else
constexpr size_t ALLOC_CACHE_ALIGN = 64;
#endif

#endif // SY_THREADING_ALLOC_CACHE_ALIGN