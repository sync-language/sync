#if defined(_WIN32) || defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <malloc.h>
#include <memoryapi.h>
#elif __GNUC__
#include <stdlib.h>
#include <sys/mman.h>
#include <cstdlib>
#include <errno.h>
#include <unistd.h>
#endif

/*
C11 standard

The pointer returned if the allocation succeeds is suitably aligned so that it may be assigned to a
pointer to any type of object with a fundamental alignment requirement and then used to access such
an object or an array of such objects in the space allocated (until the space is explicitly deallocated).

ISO/IEC 9899:201x (aka ISO C11)
*/

#include <iostream>
#include "os_mem.hpp"

extern "C" {
    void *aligned_malloc(size_t len, size_t align) {
        #if defined(_WIN32) || defined(WIN32)
        // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-malloc?view=msvc-170&viewFallbackFrom=vs-2019
        return _aligned_malloc(len, (size_t)align);
        #elif __GNUC__
        void* mem = nullptr;
        if(align <= sizeof(void*)) {
            mem = std::malloc(len);
        } else {
            size_t allocSize = len;
            if(len < align) allocSize = align;
            if(allocSize % align != 0) allocSize += align - (allocSize % align);
            mem = aligned_alloc(align, allocSize);
        }
        return mem;
        #endif
    }
    
    void aligned_free(void *buf) {
        #if defined(_WIN32) || defined(WIN32)
        // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-free?view=msvc-170
        _aligned_free(buf);
        #elif __GNUC__
        free(buf);
        #endif
    }

    void* page_malloc(size_t len) {    
        #if defined(_WIN32) || defined(WIN32)
        return VirtualAlloc(NULL, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        #elif __GNUC__
        return mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        #endif
    }

    void page_free(void *pagesStart, size_t len) {
        #if defined(_WIN32) || defined(WIN32)
        VirtualFree(pagesStart, len, MEM_DECOMMIT | MEM_RELEASE);
        #elif __GNUC__
        munmap(pagesStart, len);
        #endif    
    }
}

size_t page_size()
{
    static size_t pageSize = []() -> size_t {
        #if defined(_WIN32) || defined(WIN32)
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        return static_cast<size_t>(sysInfo.dwPageSize);
        #elif __GNUC__
        long sz = sysconf(_SC_PAGESIZE);
        return static_cast<size_t>(sz);
        #else
        static_assert(false, "Unknown page size for target");
        #endif
    }();
    return pageSize;
}

#if SYNC_LIB_WITH_TESTS

#include "../doctest.h"

TEST_CASE("page size") {
    const size_t systemPageSize = page_size();
    CHECK_GE(systemPageSize, 4096);
}

#endif // SYNC_LIB_NO_TESTS