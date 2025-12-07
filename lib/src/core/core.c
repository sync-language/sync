#include "core.h"
#include "core_internal.h"
#include <stdlib.h>

#ifndef SYNC_CUSTOM_DEFAULT_FATAL_ERROR_HANDLER
#include <stdio.h>
static void sy_default_fatal_error_handler(const char* message) {
    fprintf(stderr, "%s", message);
#ifndef NDEBUG
#if defined(_WIN32)
    __debugbreak();
#elif defined(__APPLE__) || defined(__GNUC__)
    __builtin_trap();
#endif
#endif // NDEBUG
    abort();
}
#else
extern void sy_default_fatal_error_handler(const char* message);
#endif

void (*defaultFatalErrorHandlerFn)(const char* message) = sy_default_fatal_error_handler;

SYNC_API void sy_set_fatal_error_handler(void (*errHandler)(const char* message)) {
    if (errHandler == NULL) {
        defaultFatalErrorHandlerFn("[sy_set_fatal_error_handler] expected non-null function pointer");
    }
    defaultFatalErrorHandlerFn = errHandler;
}

#ifndef SYNC_CUSTOM_ALIGNED_MALLOC_FREE
void* sy_aligned_malloc(size_t len, size_t align) {
    if ((align & (align - 1)) != 0) {
        defaultFatalErrorHandlerFn("[sy_aligned_malloc] align is not a power of 2");
    }
    if ((len % align) != 0) {
        defaultFatalErrorHandlerFn("[sy_aligned_malloc] len must be multiple of align");
    }
#if defined(_WIN32)
    return _aligned_malloc(len, align);
#else
    return aligned_alloc(align, len);
#endif
}

void sy_aligned_free(void* mem, size_t len, size_t align) {
    if ((align & (align - 1)) != 0) {
        defaultFatalErrorHandlerFn("[sy_aligned_free] align is not a power of 2");
    }
    if ((len % align) != 0) {
        defaultFatalErrorHandlerFn("[sy_aligned_free] len must be multiple of align");
    }
#if defined(_WIN32)
    _aligned_free(mem);
#else
    free(mem);
#endif
}
#endif // SYNC_CUSTOM_ALIGNED_MALLOC_FREE

#ifdef SYNC_NO_PAGES
#ifndef SYNC_DEFAULT_PAGE_ALIGNMENT
#define SYNC_DEFAULT_PAGE_ALIGNMENT 4096
#endif
#endif

#ifndef SYNC_CUSTOM_PAGE_MEMORY
#ifndef SYNC_NO_PAGES // SYNC_NO_PAGES
#if defined(_MSC_VER) || defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
// clang-format off
#include <windows.h>
#include <malloc.h>
#include <memoryapi.h>
// clang-format on
#elif __GNUC__
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#endif // WIN32 / GNUC
#endif // SYNC_NO_PAGES

void* sy_page_malloc(size_t len) {
    const size_t pageSize = sy_page_size();
    if ((len % pageSize) != 0) {
        defaultFatalErrorHandlerFn("[sy_page_malloc] len must be multiple of sy_page_size");
    }
#if defined(SYNC_NO_PAGES)
    return aligned_malloc(len, pageSize);
#elif defined(_WIN32)
    return VirtualAlloc(NULL, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#elif defined(__GNUC__)
    return mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#else
#error "Improperly configured on whether to use page memory operations or not. Please define 'SYNC_NO_PAGES'"
#endif
}

void sy_page_free(void* pagesStart, size_t len) {
    const size_t pageSize = sy_page_size();
    if ((len % pageSize) != 0) {
        defaultFatalErrorHandlerFn("[sy_page_malloc] len must be multiple of sy_page_size");
    }
#if defined(SYNC_NO_PAGES)
    return sy_aligned_free(pagesStart, len, pageSize);
#elif defined(_WIN32)
    auto result = VirtualFree(pagesStart, 0, MEM_RELEASE);
    if (result == 0) {
        defaultFatalErrorHandlerFn("[sy_page_free] failed to free pages");
    }
#elif defined(__GNUC__)
    int result = munmap(pagesStart, len);
    if (result == -1) {
        defaultFatalErrorHandlerFn("[sy_page_free] failed to free pages");
    }
#else
#error "Improperly configured on whether to use page memory operations or not. Please define 'SYNC_NO_PAGES'"
#endif
}

size_t sy_page_size(void) {
#if defined(SYNC_NO_PAGES)
    return SYNC_DEFAULT_PAGE_ALIGNMENT;
#elif defined(_WIN32)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return (size_t)(sysInfo.dwPageSize);
#elif defined(__GNUC__)
    long sz = sysconf(_SC_PAGESIZE);
    return (size_t)(sz);
#else
#error "Improperly configured on whether to use page memory operations or not. Please define 'SYNC_NO_PAGES'"
#endif
}

void sy_make_pages_read_only(void* pagesStart, size_t len) {
    const size_t pageSize = sy_page_size();
    if ((len % pageSize) != 0) {
        defaultFatalErrorHandlerFn("[sy_make_pages_read_only] len must be multiple of sy_page_size");
    }
#if defined(SYNC_NO_PAGES)
    (void)pagesStart;
    (void)len;
#elif defined(_WIN32)
    const DWORD newProtect = PAGE_READONLY;
    DWORD oldProtect;
    const bool success = VirtualProtect(baseAddress, size, newProtect, &oldProtect);
    if (success == false) {
        defaultFatalErrorHandlerFn("[sy_make_pages_read_only] failed to make pages read only");
    }
#elif defined(__APPLE__) || defined(__GNUC__)
    const int success = mprotect(pagesStart, len, PROT_READ);
    if (success != 0) {
        defaultFatalErrorHandlerFn("[sy_make_pages_read_only] failed to make pages read only");
    }
#else
#error "Improperly configured on whether to use page memory operations or not. Please define 'SYNC_NO_PAGES'"
#endif
}

void sy_make_pages_read_write(void* pagesStart, size_t len) {
    const size_t pageSize = sy_page_size();
    if ((len % pageSize) != 0) {
        defaultFatalErrorHandlerFn("[sy_make_pages_read_write] len must be multiple of sy_page_size");
    }
#if defined(SYNC_NO_PAGES)
    (void)pagesStart;
    (void)len;
#elif defined(_WIN32)
    const DWORD newProtect = PAGE_READWRITE;
    DWORD oldProtect;
    const bool success = VirtualProtect(baseAddress, size, newProtect, &oldProtect);
    if (success == false) {
        defaultFatalErrorHandlerFn("[sy_make_pages_read_only] failed to make pages read / write");
    }
#elif defined(__APPLE__) || defined(__GNUC__)
    const int success = mprotect(pagesStart, len, PROT_READ | PROT_WRITE);
    if (success != 0) {
        defaultFatalErrorHandlerFn("[sy_make_pages_read_only] failed to make pages read / write");
    }
#else
#error "Improperly configured on whether to use page memory operations or not. Please define 'SYNC_NO_PAGES'"
#endif
}
#endif // SYNC_CUSTOM_PAGE_MEMORY

#if defined(_WIN32)
#include <vcruntime_c11_atomic_support.h>
#else
#include <stdatomic.h>

static enum memory_order sy_memory_order_to_std(SyMemoryOrder order) {
    // let compiler optimize this.
    // On my MacBook, these values are the same.
    switch (order) {
    case SY_MEMORY_ORDER_RELAXED:
        return memory_order_relaxed;
    case SY_MEMORY_ORDER_CONSUME:
        return memory_order_consume;
    case SY_MEMORY_ORDER_ACQUIRE:
        return memory_order_acquire;
    case SY_MEMORY_ORDER_RELEASE:
        return memory_order_release;
    case SY_MEMORY_ORDER_ACQ_REL:
        return memory_order_acq_rel;
    case SY_MEMORY_ORDER_SEQ_CST:
        return memory_order_seq_cst;
    default:
        unreachable();
    }
}

#endif

size_t sy_atomic_size_t_load(const SyAtomicSizeT* self, SyMemoryOrder order) {
#if defined(_WIN32)
    return 0;
#else
    return atomic_load_explicit((const _Atomic volatile size_t*)&self->value, sy_memory_order_to_std(order));
#endif
}

void sy_atomic_size_t_store(SyAtomicSizeT* self, size_t newValue, SyMemoryOrder order) {
#if defined(_WIN32)
    return;
#else
    atomic_store_explicit((_Atomic volatile size_t*)&self->value, newValue, sy_memory_order_to_std(order));
#endif
}

size_t sy_atomic_size_t_fetch_add(SyAtomicSizeT* self, size_t toAdd, SyMemoryOrder order) {
#if defined(_WIN32)
    return 0;
#else
    return atomic_fetch_add_explicit((_Atomic volatile size_t*)&self->value, toAdd, sy_memory_order_to_std(order));
#endif
}

size_t sy_atomic_size_t_fetch_sub(SyAtomicSizeT* self, size_t toSub, SyMemoryOrder order) {
#if defined(_WIN32)
    return 0;
#else
    return atomic_fetch_sub_explicit((_Atomic volatile size_t*)&self->value, toSub, sy_memory_order_to_std(order));
#endif
}

size_t sy_atomic_size_t_exchange(SyAtomicSizeT* self, size_t newValue, SyMemoryOrder order) {
#if defined(_WIN32)
    return 0;
#else
    return atomic_exchange_explicit((_Atomic volatile size_t*)&self->value, newValue, sy_memory_order_to_std(order));
#endif
}

bool sy_atomic_bool_load(const SyAtomicBool* self, SyMemoryOrder order) {
#if defined(_WIN32)
    return false;
#else
    return atomic_load_explicit((const _Atomic volatile bool*)&self->value, sy_memory_order_to_std(order));
#endif
}

void sy_atomic_bool_store(SyAtomicBool* self, bool newValue, SyMemoryOrder order) {
#if defined(_WIN32)
    return;
#else
    atomic_store_explicit((_Atomic volatile bool*)&self->value, newValue, sy_memory_order_to_std(order));
#endif
}

bool sy_atomic_bool_exchange(SyAtomicBool* self, bool newValue, SyMemoryOrder order) {
#if defined(_WIN32)
    return 0;
#else
    return atomic_exchange_explicit((_Atomic volatile bool*)&self->value, newValue, sy_memory_order_to_std(order));
#endif
}
