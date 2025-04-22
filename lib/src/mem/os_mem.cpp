#include "os_mem.hpp"

#if defined(_WIN32) || defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <malloc.h>
#include <memoryapi.h>
#elif __GNUC__
#include <stdlib.h>
#include "mem.h"
#include <sys/mman.h>
#endif

/*
C11 standard

The pointer returned if the allocation succeeds is suitably aligned so that it may be assigned to a
pointer to any type of object with a fundamental alignment requirement and then used to access such
an object or an array of such objects in the space allocated (until the space is explicitly deallocated).

ISO/IEC 9899:201x (aka ISO C11)
*/

void *sy::aligned_malloc(size_t len, size_t align) {
    #if defined(_WIN32) || defined(WIN32)
    // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-malloc?view=msvc-170&viewFallbackFrom=vs-2019
    return _aligned_malloc(len, (size_t)align);
    #elif __GNUC__
    return aligned_alloc(align, len);
    #endif
}

void sy::aligned_free(void *buf) {
    #if defined(_WIN32) || defined(WIN32)
    // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-free?view=msvc-170
    _aligned_free(buf);
    #elif __GNUC__
    free(buf);
    #endif
}
