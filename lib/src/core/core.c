#if defined(__linux__) || (defined(__GNUC__) && !defined(__APPLE__) && !defined(_WIN32) && !defined(__EMSCRIPTEN__))
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include "core.h"
#include "core_internal.h"
#include <limits.h>
#include <stdlib.h>

#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

// WHAT
#if defined(__SANITIZE_THREAD__)
#define SYNC_TSAN_ENABLED 1
#elif defined(__clang__)
#if __has_feature(thread_sanitizer)
#define SYNC_TSAN_ENABLED 1
#endif
#endif

#ifdef SYNC_TSAN_ENABLED
extern void __tsan_mutex_destroy(void* addr, unsigned flags);
extern void __tsan_mutex_pre_lock(void* addr, unsigned flags);
extern void __tsan_mutex_post_lock(void* addr, unsigned flags);
extern void __tsan_mutex_pre_unlock(void* addr, unsigned flags);
extern void __tsan_mutex_post_unlock(void* addr, unsigned flags);
#define __tsan_mutex_not_static 0x1
#endif

#ifdef SYNC_LIB_CODE_COVERAGE
volatile int _sy_coverage_no_optimize = 0;
#endif

#ifndef SYNC_CUSTOM_DEFAULT_FATAL_ERROR_HANDLER
static void sy_default_fatal_error_handler(const char* message) {
    sy_print_callstack();
    syncWriteStringError(message);
#ifndef NDEBUG
#if defined(__EMSCRIPTEN__)
    emscripten_debugger();
#elif defined(_WIN32)
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

void (*syncFatalErrorHandlerFn)(const char* message) = sy_default_fatal_error_handler;

#ifndef SYNC_CUSTOM_DEFAULT_WRITE_STRING_ERROR
#include <stdio.h>
static void sy_default_write_string_error(const char* message) {
    (void)fprintf(stderr, "%s\n", message);
    (void)fflush(stderr);
}
#else
extern void sy_default_write_string_error(const char* message);
#endif

void (*syncWriteStringError)(const char* message) = sy_default_write_string_error;

SY_API void sy_set_fatal_error_handler(void (*errHandler)(const char* message)) {
    sy_assert_release(errHandler != NULL, "[sy_set_fatal_error_handler] expected non-null function pointer");
    syncFatalErrorHandlerFn = errHandler;
}

SY_API void sy_set_write_string_error(void (*writeStrErr)(const char* message)) {
    sy_assert_release(writeStrErr != NULL, "[sy_set_write_string_error] expected non-null function pointer");
    syncWriteStringError = writeStrErr;
}

#ifndef SYNC_CUSTOM_ALIGNED_MALLOC_FREE
void* sy_aligned_malloc(size_t len, size_t align) {
    sy_assert_release((align & (align - 1)) == 0, "[sy_aligned_malloc] align is not a power of 2");
    sy_assert_release((len % align) == 0, "[sy_aligned_malloc] len must be multiple of align");
#if defined(_WIN32)
    return _aligned_malloc(len, align);
#else
    return aligned_alloc(align, len);
#endif
}

void sy_aligned_free(void* mem, size_t len, size_t align) {
    sy_assert_release((align & (align - 1)) == 0, "[sy_aligned_free] align is not a power of 2");
    sy_assert_release((len % align) == 0, "[sy_aligned_free] len must be multiple of align");
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
    sy_assert_release((len % pageSize) == 0, "[sy_page_malloc] len must be multiple of sy_page_size");
#if defined(SYNC_NO_PAGES)
    return sy_aligned_malloc(len, pageSize);
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
    sy_assert_release((len % pageSize) == 0, "[sy_page_free] len must be multiple of sy_page_size");
#if defined(SYNC_NO_PAGES)
    sy_aligned_free(pagesStart, len, pageSize);
#elif defined(_WIN32)
    bool result = VirtualFree(pagesStart, 0, MEM_RELEASE);
    sy_assert_release(result == true, "[sy_page_free] failed to free pages");
#elif defined(__GNUC__)
    int result = munmap(pagesStart, len);
    sy_assert_release(result != -1, "[sy_page_free] failed to free pages");
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
    sy_assert_release((len % pageSize) == 0, "[sy_make_pages_read_only] len must be multiple of sy_page_size");
#if defined(SYNC_NO_PAGES)
    (void)pagesStart;
    (void)len;
#elif defined(_WIN32)
    const DWORD newProtect = PAGE_READONLY;
    DWORD oldProtect;
    const bool success = VirtualProtect(pagesStart, len, newProtect, &oldProtect);
    sy_assert_release(success == true, "[sy_make_pages_read_only] failed to make pages read only");
#elif defined(__APPLE__) || defined(__GNUC__)
    const int success = mprotect(pagesStart, len, PROT_READ);
    sy_assert_release(success == 0, "[sy_make_pages_read_only] failed to make pages read only");
#else
#error "Improperly configured on whether to use page memory operations or not. Please define 'SYNC_NO_PAGES'"
#endif
}

void sy_make_pages_read_write(void* pagesStart, size_t len) {
    const size_t pageSize = sy_page_size();
    sy_assert_release((len % pageSize) == 0, "[sy_make_pages_read_write] len must be multiple of sy_page_size");
#if defined(SYNC_NO_PAGES)
    (void)pagesStart;
    (void)len;
#elif defined(_WIN32)
    const DWORD newProtect = PAGE_READWRITE;
    DWORD oldProtect;
    const bool success = VirtualProtect(pagesStart, len, newProtect, &oldProtect);
    sy_assert_release(success == true, "[sy_make_pages_read_only] failed to make pages read / write");
#elif defined(__APPLE__) || defined(__GNUC__)
    const int success = mprotect(pagesStart, len, PROT_READ | PROT_WRITE);
    sy_assert_release(success == 0, "[sy_make_pages_read_only] failed to make pages read / write");
#else
#error "Improperly configured on whether to use page memory operations or not. Please define 'SYNC_NO_PAGES'"
#endif
}
#endif // SYNC_CUSTOM_PAGE_MEMORY

#if defined(_MSC_VER) && defined(__STDC_NO_ATOMICS__)
// If someone wants to build Sync using only the source files, and not the provided build scripts, they should be able
// to do that. To avoid `/experimental:c11atomics`, lets just used the Interlocked functions
#else
#include <stdatomic.h>
static int sy_memory_order_to_std(SyMemoryOrder order) {
    switch (order) {
    case SY_MEMORY_ORDER_RELAXED:
        return (int)memory_order_relaxed;
    case SY_MEMORY_ORDER_CONSUME:
        return (int)memory_order_consume;
    case SY_MEMORY_ORDER_ACQUIRE:
        return (int)memory_order_acquire;
    case SY_MEMORY_ORDER_RELEASE:
        return (int)memory_order_release;
    case SY_MEMORY_ORDER_ACQ_REL:
        return (int)memory_order_acq_rel;
    case SY_MEMORY_ORDER_SEQ_CST:
        return (int)memory_order_seq_cst;
    default:
        unreachable();
    }
}
#endif

size_t sy_atomic_size_t_load(const SyAtomicSizeT* self, SyMemoryOrder order) {
#if defined(_MSC_VER) && defined(__STDC_NO_ATOMICS__)
    (void)order; // Interlocked functions use full memory barrier (seq_cst)
    return (size_t)_InterlockedOr64((volatile LONG64*)&self->value, 0);
#else
    return atomic_load_explicit((const _Atomic volatile size_t*)&self->value, sy_memory_order_to_std(order));
#endif
}

void sy_atomic_size_t_store(SyAtomicSizeT* self, size_t newValue, SyMemoryOrder order) {
#if defined(_MSC_VER) && defined(__STDC_NO_ATOMICS__)
    (void)order;
    _InterlockedExchange64((volatile LONG64*)&self->value, (LONG64)newValue);
#else
    atomic_store_explicit((_Atomic volatile size_t*)&self->value, newValue, sy_memory_order_to_std(order));
#endif
}

size_t sy_atomic_size_t_fetch_add(SyAtomicSizeT* self, size_t toAdd, SyMemoryOrder order) {
#if defined(_MSC_VER) && defined(__STDC_NO_ATOMICS__)
    (void)order;
    return (size_t)_InterlockedExchangeAdd64((volatile LONG64*)&self->value, (LONG64)toAdd); // no overflow?
#else
    return atomic_fetch_add_explicit((_Atomic volatile size_t*)&self->value, toAdd, sy_memory_order_to_std(order));
#endif
}

size_t sy_atomic_size_t_fetch_sub(SyAtomicSizeT* self, size_t toSub, SyMemoryOrder order) {
#if defined(_MSC_VER) && defined(__STDC_NO_ATOMICS__)
    (void)order;
    return (size_t)_InterlockedExchangeAdd64((volatile LONG64*)&self->value, -(LONG64)toSub); // no overflow?
#else
    return atomic_fetch_sub_explicit((_Atomic volatile size_t*)&self->value, toSub, sy_memory_order_to_std(order));
#endif
}

size_t sy_atomic_size_t_exchange(SyAtomicSizeT* self, size_t newValue, SyMemoryOrder order) {
#if defined(_MSC_VER) && defined(__STDC_NO_ATOMICS__)
    (void)order;
    return (size_t)_InterlockedExchange64((volatile LONG64*)&self->value, (LONG64)newValue);
#else
    return atomic_exchange_explicit((_Atomic volatile size_t*)&self->value, newValue, sy_memory_order_to_std(order));
#endif
}

bool sy_atomic_size_t_compare_exchange_weak(SyAtomicSizeT* self, size_t* expected, size_t desired,
                                            SyMemoryOrder order) {
#if defined(_MSC_VER) && defined(__STDC_NO_ATOMICS__)
    (void)order;
    LONG64 prev = _InterlockedCompareExchange64((volatile LONG64*)&self->value, (LONG64)desired, (LONG64)*expected);
    if (prev == (LONG64)*expected) {
        return true;
    } else {
        *expected = (size_t)prev;
        return false;
    }
#else
    return atomic_compare_exchange_weak_explicit((_Atomic volatile size_t*)&self->value, expected, desired,
                                                 sy_memory_order_to_std(order), memory_order_relaxed);
#endif
}

bool sy_atomic_bool_load(const SyAtomicBool* self, SyMemoryOrder order) {
#if defined(_MSC_VER) && defined(__STDC_NO_ATOMICS__)
    (void)order;
    return (_InterlockedOr8((volatile char*)&self->value, 0) != 0);
#else
    return atomic_load_explicit((const _Atomic volatile bool*)&self->value, sy_memory_order_to_std(order));
#endif
}

void sy_atomic_bool_store(SyAtomicBool* self, bool newValue, SyMemoryOrder order) {
#if defined(_MSC_VER) && defined(__STDC_NO_ATOMICS__)
    (void)order;
    _InterlockedExchange8((volatile char*)&self->value, (char)(newValue ? 1 : 0));
#else
    atomic_store_explicit((_Atomic volatile bool*)&self->value, newValue, sy_memory_order_to_std(order));
#endif
}

bool sy_atomic_bool_exchange(SyAtomicBool* self, bool newValue, SyMemoryOrder order) {
#if defined(_MSC_VER) && defined(__STDC_NO_ATOMICS__)
    (void)order;
    return (_InterlockedExchange8((volatile char*)&self->value, (char)(newValue ? 1 : 0)) != 0);
#else
    return atomic_exchange_explicit((_Atomic volatile bool*)&self->value, newValue, sy_memory_order_to_std(order));
#endif
}

bool sy_atomic_bool_compare_exchange_weak(SyAtomicBool* self, bool* expected, bool desired, SyMemoryOrder order) {
#if defined(_MSC_VER) && defined(__STDC_NO_ATOMICS__)
    (void)order;
    char expectedChar = (char)(*expected ? 1 : 0);
    char desiredChar = (char)(desired ? 1 : 0);
    char prev = _InterlockedCompareExchange8((volatile char*)&self->value, desiredChar, expectedChar);
    if (prev == expectedChar) {
        return true;
    } else {
        *expected = (prev != 0);
        return false;
    }
#else
    return atomic_compare_exchange_weak_explicit((_Atomic volatile bool*)&self->value, expected, desired,
                                                 sy_memory_order_to_std(order), memory_order_relaxed);
#endif
}

#ifndef SYNC_CUSTOM_THREAD_YIELD

#if defined(_WIN32)
// SwitchToThread()
#elif defined(_POSIX_VERSION)
#if defined __has_include
#if __has_include(<sched.h>)
#include <sched.h>
#endif
#endif // defined __has_include
#elif defined(__x86_64__)
#include <immintrin.h> // _mm_pause
#elif defined(__aarch64__)
#include <intrin.h> // __yield()
#endif

void sy_thread_yield(void) {
#if defined(_WIN32)
    (void)SwitchToThread();
#elif defined(__EMSCRIPTEN__)
    __sync_synchronize();
#elif defined(__unix__) || defined(__APPLE__)
    (void)sched_yield();
#elif defined(__x86_64__)
    _mm_pause();
#elif defined(__aarch64__)
    __yield();
#elif defined(__riscv)
    __builtin_riscv_pause();
#else
#if __has_include(<sched.h>)
    (void)sched_yield();
#endif
#endif
}
#endif // SYNC_CUSTOM_THREAD_YIELD

/// Not real thread ids, but "fake" ones to do custom mutex properly.
static SyAtomicSizeT globalThreadIdGenerator;
#if (__STDC_VERSION__ >= 202311L) // C23
/// Not actual thread id
thread_local size_t threadLocalThreadId = 0;
#define RAW_RWLOCK_ALLOC_ALIGN alignof(size_t)
#else
/// Not actual thread id
_Thread_local size_t threadLocalThreadId = 0;
#define RAW_RWLOCK_ALLOC_ALIGN _Alignof(size_t)
#endif

static void initializeThisThreadId(void) {
    if (threadLocalThreadId != 0)
        return;

    size_t fetched = sy_atomic_size_t_fetch_add(&globalThreadIdGenerator, 1, SY_MEMORY_ORDER_SEQ_CST);
    sy_assert_release(fetched < (SIZE_MAX - 1),
                      "[initializeThisThreadId] reached max value for thread id generator (how?)");

    threadLocalThreadId = fetched + 1; // don't start at 0
}

/// @return `false` if out of memory, otherwise `true`
static bool addThisThreadToReaders(SyRawRwLock* self) {
    initializeThisThreadId();
    const size_t threadId = threadLocalThreadId;

    const int32_t currentLen = self->readerLen;
    const int32_t currentCapacity = self->readerCapacity;

    if (currentLen == currentCapacity) { // reallocate if necessary
        sy_assert_release(currentCapacity <= ((INT32_MAX / 2) - 1),
                          "[addThisThreadToReaders] reached max value for reader capacity (how?)");
        int32_t newCapacity = currentCapacity * 2;
        if (newCapacity == 0) {
            newCapacity = 4; // reasonable default number of readers
        }

        size_t* newReaders = (size_t*)sy_aligned_malloc((size_t)(newCapacity) * sizeof(size_t), RAW_RWLOCK_ALLOC_ALIGN);
        if (newReaders == NULL) {
            return false; // out of memory
        }
        size_t* oldReaders = (size_t*)self->readers; // can be NULL it is fine
        for (int32_t i = 0; i < currentLen; i++) {
            newReaders[i] = oldReaders[i];
        }

        if (oldReaders != NULL) {
            sy_aligned_free((void*)oldReaders, currentCapacity * sizeof(size_t), RAW_RWLOCK_ALLOC_ALIGN);
        }

        self->readers = newReaders;
        self->readerCapacity = newCapacity;
    }

    self->readers[currentLen] = threadId;
    self->readerLen = currentLen + 1;
    return true;
}

static void removeThisThreadFromReadersFirstInstance(SyRawRwLock* self) {
    initializeThisThreadId();
    const size_t threadId = threadLocalThreadId;

    const int32_t currentLen = self->readerLen;
    size_t* readers = self->readers;

    int32_t found = -1;
    for (int32_t i = 0; i < currentLen; i++) {
        if (readers[i] == threadId) {
            found = i;
            break;
        }
    }

    if (found != -1) {
        for (int32_t i = found; i < (currentLen - 1); i++) {
            readers[i] = readers[i + 1]; // shift down
        }

        self->readerLen = currentLen - 1;
    }
}

static bool isThisThreadOnlyReader(const SyRawRwLock* self) {
    initializeThisThreadId();
    const size_t threadId = threadLocalThreadId;

    const int32_t currentLen = self->readerLen;
    const size_t* readers = (const size_t*)self->readers;

    // as many of this thread re-entrance
    for (int32_t i = 0; i < currentLen; i++) {
        if (readers[i] != threadId) {
            return false;
        }
    }
    return true;
}

static bool isThisThreadAReader(const SyRawRwLock* self) {
    initializeThisThreadId();
    const size_t threadId = threadLocalThreadId;

    const int32_t currentLen = self->readerLen;
    const size_t* readers = (const size_t*)self->readers;

    for (int32_t i = 0; i < currentLen; i++) {
        if (readers[i] == threadId) {
            return true;
        }
    }
    return false;
}

static void removeThisThreadFromWantToElevate(SyRawRwLock* self) {
    initializeThisThreadId();
    const size_t threadId = threadLocalThreadId;

    const int32_t currentLen = self->threadsWantElevateLen;

    for (int32_t i = 0; i < currentLen; i++) {
        if (self->threadsWantElevate[i] != threadId) {
            continue;
        }

        for (; i < (currentLen - 1); i++) {
            self->threadsWantElevate[i] = self->threadsWantElevate[i + 1];
        }
        self->threadsWantElevateLen -= 1;
        break;
    }
}

static void acquireFence(SyRawRwLock* self) {
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_pre_lock(&self->fence, 0);
#endif
    bool expected = false;
    while (!(sy_atomic_bool_compare_exchange_weak(&self->fence, &expected, true, SY_MEMORY_ORDER_SEQ_CST))) {
        expected = false;
        sy_thread_yield();
    }
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_post_lock(&self->fence, 0);
#endif
}

static void releaseFence(SyRawRwLock* self) {
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_pre_unlock(&self->fence, 0);
#endif
    sy_atomic_bool_store(&self->fence, false, SY_MEMORY_ORDER_SEQ_CST);
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_post_unlock(&self->fence, 0);
#endif
}

void sy_raw_rwlock_destroy(SyRawRwLock* self) {
    acquireFence(self);

    const size_t currentExclusiveId = sy_atomic_size_t_load(&self->exclusiveId, SY_MEMORY_ORDER_SEQ_CST);
    const int32_t currentReadersLen = self->readerLen;
    const int32_t currentWantElevateLen = self->threadsWantElevateLen;
    sy_assert_release(currentExclusiveId == 0,
                      "[sy_raw_rwlock_destroy] cannot destroy rwlock when a thread has exclusive access");
    sy_assert_release(currentReadersLen == 0,
                      "[sy_raw_rwlock_destroy] cannot destroy rwlock that was locked by another thread");
    sy_assert_release(currentWantElevateLen == 0,
                      "[sy_raw_rwlock_destroy] cannot destroy rwlock that other threads wanting to elevate");

    if (self->readers != NULL) {
        sy_aligned_free((void*)self->readers, (size_t)(self->readerCapacity) * sizeof(size_t), RAW_RWLOCK_ALLOC_ALIGN);
    }
    if (self->threadsWantElevate != NULL) {
        sy_aligned_free((void*)self->threadsWantElevate, (size_t)(self->threadsWantElevateCapacity) * sizeof(size_t),
                        RAW_RWLOCK_ALLOC_ALIGN);
    }
    releaseFence(self);
#ifdef SYNC_TSAN_ENABLED
    __tsan_mutex_destroy(&self->fence, 0);
#endif
}

SyAcquireErr sy_raw_rwlock_try_acquire_shared(SyRawRwLock* self) {
    initializeThisThreadId();
    const size_t threadId = threadLocalThreadId;

    { // Quick check. Don't wanna go through all the steps if someone has an exclusive lock.
        size_t currentExclusiveId = sy_atomic_size_t_load(&self->exclusiveId, SY_MEMORY_ORDER_SEQ_CST);
        if (currentExclusiveId != threadId) {
            if (currentExclusiveId != 0) {
                return SY_ACQUIRE_ERR_SHARED_HAS_EXCLUSIVE;
            }
        }
    }

    acquireFence(self);

    { // Fence acquired, check exclusive id again in case someone else acquired in the meantime
        size_t currentExclusiveId = sy_atomic_size_t_load(&self->exclusiveId, SY_MEMORY_ORDER_SEQ_CST);
        if (currentExclusiveId != threadId) {
            if (currentExclusiveId != 0) {
                releaseFence(self);
                return SY_ACQUIRE_ERR_SHARED_HAS_EXCLUSIVE;
            }
        }
    }

    if (addThisThreadToReaders(self) == false) {
        releaseFence(self);
        return SY_ACQUIRE_ERR_OUT_OF_MEMORY;
    }
    releaseFence(self);
    return SY_ACQUIRE_ERR_NONE;
}

SyAcquireErr sy_raw_rwlock_acquire_shared(SyRawRwLock* self) {
    while (true) {
        SyAcquireErr err = sy_raw_rwlock_try_acquire_shared(self);
        if (err == SY_ACQUIRE_ERR_NONE || err == SY_ACQUIRE_ERR_OUT_OF_MEMORY) {
            return err;
        }
        sy_thread_yield();
    }
}

void sy_raw_rwlock_release_shared(SyRawRwLock* self) {
    initializeThisThreadId();
    const size_t threadId = threadLocalThreadId;

    acquireFence(self);

    const size_t currentExclusiveId = sy_atomic_size_t_load(&self->exclusiveId, SY_MEMORY_ORDER_SEQ_CST);
    // releasing shared lock on a thread that ALSO has exclusive lock (re-entrant)
    sy_assert_release(
        currentExclusiveId == 0 || currentExclusiveId == threadId,
        "[sy_raw_rwlock_release_exclusive] cannot release shared lock when another thread has an exclusive lock");
    sy_assert_release(self->readerLen != 0,
                      "[sy_raw_rwlock_release_exclusive] cannot release shared lock if no thread has a shared lock");

    removeThisThreadFromReadersFirstInstance(self);
    releaseFence(self);
}

SyAcquireErr sy_raw_rwlock_try_acquire_exclusive(SyRawRwLock* self) {
    const size_t oldDeadlockGeneration = sy_atomic_size_t_load(&self->deadlockGeneration, SY_MEMORY_ORDER_SEQ_CST);

    initializeThisThreadId();
    const size_t threadId = threadLocalThreadId;

    { // Quick check. Don't wanna go through all the steps if someone has an exclusive lock.
        size_t currentExclusiveId = sy_atomic_size_t_load(&self->exclusiveId, SY_MEMORY_ORDER_SEQ_CST);
        if (currentExclusiveId == threadId) {
            sy_atomic_size_t_fetch_add(&self->exclusiveCount, 1, SY_MEMORY_ORDER_SEQ_CST);
            return SY_ACQUIRE_ERR_NONE;
        }
        if (currentExclusiveId != 0) {
            return SY_ACQUIRE_ERR_EXCLUSIVE_HAS_EXCLUSIVE;
        }
    }

    bool thisThreadIsReader = false;
    { // deadlock detection, update elevate wait graph if necessary
        acquireFence(self);

        if (isThisThreadAReader(self)) {
            thisThreadIsReader = true;

            const int32_t currentElevateCapacity = self->threadsWantElevateCapacity;
            const int32_t currentElevateLen = self->threadsWantElevateLen;

            if (currentElevateLen == currentElevateCapacity) {
                sy_assert_release(
                    currentElevateCapacity <= ((INT32_MAX / 2) - 1),
                    "[sy_raw_rwlock_try_acquire_exclusive] reached max value for elevate capacity (how?)");
                int32_t newCapacity = currentElevateCapacity * 2;
                if (newCapacity == 0) {
                    newCapacity = 2; // reasonable default for elevation
                }
                size_t* newThreadsWantElevate =
                    (size_t*)sy_aligned_malloc((size_t)(newCapacity) * sizeof(size_t), RAW_RWLOCK_ALLOC_ALIGN);
                if (newThreadsWantElevate == NULL) {
                    releaseFence(self);
                    return SY_ACQUIRE_ERR_OUT_OF_MEMORY;
                }

                for (int32_t i = 0; i < currentElevateLen; i++) {
                    newThreadsWantElevate[i] = self->threadsWantElevate[i];
                }

                if (self->threadsWantElevate != NULL) {
                    sy_aligned_free((void*)self->threadsWantElevate, currentElevateCapacity * sizeof(size_t),
                                    RAW_RWLOCK_ALLOC_ALIGN);
                }

                self->threadsWantElevate = newThreadsWantElevate;
                self->threadsWantElevateCapacity = newCapacity;
            }

            self->threadsWantElevate[currentElevateLen] = threadId;
            self->threadsWantElevateLen = currentElevateLen + 1;
            releaseFence(self);
            sy_thread_yield(); // let others update wait graph
        } else {
            releaseFence(self);
        }
    }

    acquireFence(self);

    const size_t newDeadlockGeneration = sy_atomic_size_t_load(&self->deadlockGeneration, SY_MEMORY_ORDER_SEQ_CST);
    if (oldDeadlockGeneration != newDeadlockGeneration) {
        // another thread detected a deadlock on this rwlock
        removeThisThreadFromWantToElevate(self);
        releaseFence(self);
        return SY_ACQUIRE_ERR_DEADLOCK;
    }

    // deadlock detection now that wait graph has been updated properly
    if (thisThreadIsReader) {
        const int32_t currentElevateLen = self->threadsWantElevateLen;
        bool foundOther = false;
        for (int32_t i = 0; i < currentElevateLen; i++) {
            if (self->threadsWantElevate[i] != threadId) {
                foundOther = true;
                break;
            }
        }
        removeThisThreadFromWantToElevate(self); // remove no matter what
        if (foundOther) {
            sy_assert_release(oldDeadlockGeneration < (SIZE_MAX - 1),
                              "[sy_raw_rwlock_try_acquire_exclusive] too many deadlocks have occurred on this rwlock");

            (void)sy_atomic_size_t_fetch_add(&self->deadlockGeneration, 1, SY_MEMORY_ORDER_SEQ_CST);
            releaseFence(self);
            return SY_ACQUIRE_ERR_DEADLOCK;
        }
    }

    { // Fence acquired, check exclusive id again in case someone else acquired in the meantime
        size_t currentExclusiveId = sy_atomic_size_t_load(&self->exclusiveId, SY_MEMORY_ORDER_SEQ_CST);
        if (currentExclusiveId != 0) {
            releaseFence(self);
            return SY_ACQUIRE_ERR_EXCLUSIVE_HAS_EXCLUSIVE;
        }
    }

    { // Check if we are the only reader, if so we can elevate this thread to exclusive
        if (!isThisThreadOnlyReader(self)) {
            releaseFence(self);
            return SY_ACQUIRE_ERR_EXCLUSIVE_HAS_OTHER_READERS;
        }

        // don't remove readers cause re-entrant functionality, just set this as exclusive owner
        sy_atomic_size_t_store(&self->exclusiveId, threadId, SY_MEMORY_ORDER_SEQ_CST);
        sy_atomic_size_t_fetch_add(&self->exclusiveCount, 1, SY_MEMORY_ORDER_SEQ_CST);
        releaseFence(self);
        return SY_ACQUIRE_ERR_NONE;
    }
}

SyAcquireErr sy_raw_rwlock_acquire_exclusive(SyRawRwLock* self) {
    while (true) {
        SyAcquireErr err = sy_raw_rwlock_try_acquire_exclusive(self);
        if (err == SY_ACQUIRE_ERR_NONE || err == SY_ACQUIRE_ERR_OUT_OF_MEMORY || err == SY_ACQUIRE_ERR_DEADLOCK) {
            return err;
        }
        sy_thread_yield();
    }
}

void sy_raw_rwlock_release_exclusive(SyRawRwLock* self) {
    initializeThisThreadId();
    const size_t threadId = threadLocalThreadId;

    acquireFence(self);

    const size_t currentExclusiveId = sy_atomic_size_t_load(&self->exclusiveId, SY_MEMORY_ORDER_SEQ_CST);
    sy_assert_release(currentExclusiveId != 0, "[sy_raw_rwlock_release_exclusive] cannot release exclusive lock when "
                                               "no thread has acquired");
    sy_assert_release(
        currentExclusiveId == threadId,
        "[sy_raw_rwlock_release_exclusive] cannot release exclusive lock that was locked by another thread");

    const size_t currentExclusiveCount = sy_atomic_size_t_fetch_sub(&self->exclusiveCount, 1, SY_MEMORY_ORDER_SEQ_CST);
    if (currentExclusiveCount == 1) {
        sy_atomic_size_t_store(&self->exclusiveId, 0, SY_MEMORY_ORDER_SEQ_CST);
    }
    releaseFence(self);
}

#ifndef SYNC_NO_FILESYSTEM

#ifndef SYNC_CUSTOM_GET_FILE_INFO
#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

bool sy_get_file_info(const char* path, size_t pathLen, size_t* outFileSize) {
#if defined(_MSC_VER)
    char nullTermPath[MAX_PATH];
#else
    char nullTermPath[PATH_MAX];
#endif

    for (size_t i = 0; i < pathLen; i++) {
        nullTermPath[i] = path[i];
    }
    nullTermPath[pathLen] = '\0';

#if defined(_MSC_VER)
    struct _stat64 s;
    if (_stat64(nullTermPath, &s) != 0) {
        return false;
    }
#else
    struct stat s;
    if (stat(nullTermPath, &s) != 0) {
        return false;
    }
#endif
    *outFileSize = (size_t)s.st_size;
    return true;
}
#endif // SYNC_CUSTOM_GET_FILE_INFO

#ifndef SYNC_CUSTOM_RELATIVE_TO_ABSOLUTE_PATH
bool sy_relative_to_absolute_path(const char* relativePath, size_t relativePathLen, char* outAbsolutePath,
                                  size_t outAbsoluteBufSize) {
#if defined(_MSC_VER)
    char relative[MAX_PATH];
    char absolute[MAX_PATH]; // TODO long paths
#else
    char relative[PATH_MAX];
    char absolute[PATH_MAX];
#endif

    for (size_t i = 0; i < relativePathLen; i++) {
        relative[i] = relativePath[i];
    }
    relative[relativePathLen] = '\0';
#if defined(_MSC_VER)
    // TODO symlink?
    if (GetFullPathNameA(relativePath, MAX_PATH, absolute, NULL) == 0) {
        return false;
    }
#else
    if (realpath(relative, absolute) == NULL) {
        return false;
    }
#endif

    size_t absoluteLen = 0;
    for (size_t i = 0; absolute[i] != '\0'; i++) {
        absoluteLen += 1;
    }

    if (outAbsoluteBufSize <= absoluteLen) {
        return false;
    }

    for (size_t i = 0; i < absoluteLen; i++) {
        outAbsolutePath[i] = absolute[i];
    }
    outAbsolutePath[absoluteLen] = '\0';

    return true;
}

#endif // SYNC_CUSTOM_RELATIVE_TO_ABSOLUTE_PATH

#endif // SYNC_NO_FILESYSTEM

#ifndef SYNC_CUSTOM_BACKTRACE

#ifndef NDEBUG
#if defined(_GAMING_XBOX) || defined(__ORBIS__) || defined(__PROSPERO__) || defined(__NX__) || defined(NN_NINTENDO_SDK)
void sy_print_callstack(void) {}
#else

#ifdef __EMSCRIPTEN__
static void print_emscripten_callstack(void) {
    syncWriteStringError("Stack trace (most recent call first):");
    char buf[4096];
    // EM_LOG_C_STACK no longer does anything?
    int bytesWritten = emscripten_get_callstack(EM_LOG_JS_STACK, buf, sizeof(buf));
    syncWriteStringError(buf);
    (void)bytesWritten;
}
#elif defined(_WIN32) // and mingw
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
// clang-format off
#include <windows.h>
#include <dbghelp.h>
#include <process.h>
#include <stdio.h>
#include <string.h>
#if __STDC_VERSION__ < 202311L
#include <stdalign.h>
#endif
// clang-format on

#if defined(_MSC_VER)
#pragma comment(lib, "Dbghelp.lib")
#endif

#define DEFAULT_BACKTRACE_DEPTH 64

void print_windows_callstack(void) {
    syncWriteStringError("Stack trace (most recent call first):");

    HANDLE process = GetCurrentProcess();
    bool loadedSymbols = true;

    // SymInitialize is intended to be called once.
    // TODO is this fine in this context?
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    if (!SymInitialize(process, NULL, TRUE)) {
        syncWriteStringError("failed to initialize symbol handler");
        loadedSymbols = false;
    }

    void* stack[DEFAULT_BACKTRACE_DEPTH];
    WORD frames = CaptureStackBackTrace(1, DEFAULT_BACKTRACE_DEPTH, stack, NULL);

    if (frames == 0) {
        syncWriteStringError("failed to capture stack frames");
        return;
    }

    alignas(alignof(SYMBOL_INFO)) char symbolBuf[sizeof(SYMBOL_INFO) + (256 * sizeof(char))];
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)symbolBuf;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = 255;

    IMAGEHLP_LINE64 line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    char moduleName[MAX_PATH] = {0};

    for (WORD i = 2; i < frames; i++) {
        DWORD64 address = (DWORD64)(uintptr_t)(stack[i]);
        char lineBuf[1024] = {0};
        const char* funcName = "???";
        const char* fileName = NULL;
        DWORD lineNumber = 0;
        const char* modName = NULL;

        DWORD64 symDisplacement = 0;
        if (SymFromAddr(process, address, &symDisplacement, symbol)) {
            if (symbol->Name[0] != '\0') {
                funcName = symbol->Name;
            }
        }

        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(process, address, &lineDisplacement, &line)) {
            fileName = line.FileName;
            lineNumber = line.LineNumber;
        }

        HMODULE hModule = NULL;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)(uintptr_t)(stack[i]), &hModule)) {
            if (GetModuleFileNameA(hModule, moduleName, MAX_PATH) != 0) {
                modName = moduleName;
            }
        }

        bool success = true;
        if (fileName != NULL && lineNumber != 0) {
            if (modName != NULL) {
                if (snprintf(lineBuf, sizeof(lineBuf), "#%-2d %s at %s:%lu (in %s)", (int)i, funcName, fileName,
                             (unsigned long)lineNumber, modName) > (int)sizeof(lineBuf)) {
                    success = false;
                }
            } else {
                if (snprintf(lineBuf, sizeof(lineBuf), "#%-2d %s at %s:%lu", (int)i, funcName, fileName,
                             (unsigned long)lineNumber) > (int)sizeof(lineBuf)) {
                    success = false;
                }
            }
        } else if (modName != NULL) {
            if (snprintf(lineBuf, sizeof(lineBuf), "#%-2d 0x%llx %s (in %s)", (int)i, (unsigned long long)address,
                         funcName, modName) > (int)sizeof(lineBuf)) {
                success = false;
            }
        } else {
            if (snprintf(lineBuf, sizeof(lineBuf), "#%-2d 0x%llx %s", (int)i, (unsigned long long)address, funcName) >
                (int)sizeof(lineBuf)) {
                success = false;
            }
        }

        if (success) {
            syncWriteStringError(lineBuf);
        } else {
            syncWriteStringError("???");
        }
    }

    // TODO need to call SymCleanup?
}

#elif defined(__MACH__) || defined(__GNUC__)
#include <dlfcn.h>
#include <execinfo.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define DEFAULT_BACKTRACE_DEPTH 64

extern char** environ; // what the heck

static void fallback_dladdr_print(const Dl_info* info, int i, void** addresses) {
    char lineBuf[512] = {0};
    const char* fname = info->dli_fname;
    const char* lastSlash = strrchr(info->dli_fname, '/');
    if (lastSlash != NULL) {
        fname = lastSlash + 1;
    }
    if (snprintf(lineBuf, sizeof(lineBuf), "#%-2d %p %s in %s", i - 1, addresses[i],
                 info->dli_sname ? info->dli_sname : "???", fname) < (int)sizeof(lineBuf)) {
        syncWriteStringError(lineBuf);
    } else {
        syncWriteStringError("???");
    }
}

static void print_posix_callstack(void) {
    syncWriteStringError("Stack trace (most recent call first):");
    void* addresses[DEFAULT_BACKTRACE_DEPTH];
    int traceSize = backtrace(addresses, DEFAULT_BACKTRACE_DEPTH);

    // don't care about this function being called
    for (int i = 2; i < traceSize; i++) {
        Dl_info info = {0};
        char lineBuf[512] = {0};

        if (dladdr(addresses[i], &info) == 0 || info.dli_fname == NULL) {
            if (snprintf(lineBuf, sizeof(lineBuf), "#%-2d %p", i - 1, addresses[i]) < (int)sizeof(lineBuf)) {
                syncWriteStringError(lineBuf);
            } else {
                syncWriteStringError("???");
            }
            continue;
        }

#if defined(__MACH__)

        char loadAddrStr[64] = {0};
        char addrStr[64] = {0};
        if (snprintf(loadAddrStr, sizeof(loadAddrStr), "0x%llx", (unsigned long long)(uintptr_t)(info.dli_fbase)) >=
            (int)sizeof(loadAddrStr)) {
            fallback_dladdr_print(&info, i, addresses);
            continue;
        }
        if (snprintf(addrStr, sizeof(addrStr), "0x%llx", (unsigned long long)(uintptr_t)(addresses[i])) >=
            (int)sizeof(addrStr)) {
            fallback_dladdr_print(&info, i, addresses);
            continue;
        }

        const char* argv[] = {"/usr/bin/atos", "-o", info.dli_fname, "-l", loadAddrStr, "-fullPath", addrStr, NULL};
        int pipefd[2] = {0};
        if (pipe(pipefd) == -1) {
            fallback_dladdr_print(&info, i, addresses);
            continue;
        }

        posix_spawn_file_actions_t actions = {0};
        if (posix_spawn_file_actions_init(&actions) != 0) {
            fallback_dladdr_print(&info, i, addresses);
            (void)close(pipefd[0]);
            (void)close(pipefd[1]);
            continue;
        }

        if (posix_spawn_file_actions_addclose(&actions, pipefd[0]) != 0 ||
            posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO) != 0 ||
            posix_spawn_file_actions_addclose(&actions, pipefd[1]) != 0) {
            fallback_dladdr_print(&info, i, addresses);
            (void)posix_spawn_file_actions_destroy(&actions);
            (void)close(pipefd[0]);
            (void)close(pipefd[1]);
            continue;
        }

        pid_t pid;
        int spawnResult = posix_spawn(&pid, "/usr/bin/atos", &actions, NULL, (char* const*)argv, environ);
        (void)posix_spawn_file_actions_destroy(&actions);
        if (spawnResult != 0) {
            fallback_dladdr_print(&info, i, addresses);
            (void)close(pipefd[0]);
            (void)close(pipefd[1]);
            continue;
        }

        (void)close(pipefd[1]); // write end

        char atosBuf[1024];
        ssize_t totalRead = 0;
        ssize_t bytesRead = 0;
        while (totalRead < (ssize_t)(sizeof(atosBuf) - 1) &&
               (bytesRead = read(pipefd[0], atosBuf + totalRead, sizeof(atosBuf) - 1 - (size_t)totalRead)) > 0) {
            totalRead += bytesRead;
        }
        atosBuf[totalRead] = '\0';
        (void)close(pipefd[0]);

        int status;
        (void)waitpid(pid, &status, 0);
        (void)status;

        if (totalRead > 0 && atosBuf[totalRead - 1] == '\n') {
            atosBuf[totalRead - 1] = '\0';
        }

        if (totalRead > 0 && atosBuf[0] != '\0') {
            (void)snprintf(lineBuf, sizeof(lineBuf), "#%-2d %s", i - 1, atosBuf);
            syncWriteStringError(lineBuf);
        } else {
            fallback_dladdr_print(&info, i, addresses);
        }

#elif defined(__GNUC__) && !defined(__APPLE__)

        uintptr_t offset = (uintptr_t)addresses[i] - (uintptr_t)info.dli_fbase;
        char offsetStr[64] = {0};
        if (snprintf(offsetStr, sizeof(offsetStr), "0x%llx", (unsigned long long)offset) >= (int)sizeof(offsetStr)) {
            fallback_dladdr_print(&info, i, addresses);
            continue;
        }

        const char* argv[] = {"/usr/bin/addr2line", "-e", info.dli_fname, "-f", "-C", "-p", offsetStr, NULL};

        int pipefd[2] = {0};
        if (pipe(pipefd) == -1) {
            fallback_dladdr_print(&info, i, addresses);
            continue;
        }

        posix_spawn_file_actions_t actions = {0};
        if (posix_spawn_file_actions_init(&actions) != 0) {
            fallback_dladdr_print(&info, i, addresses);
            (void)close(pipefd[0]);
            (void)close(pipefd[1]);
            continue;
        }

        if (posix_spawn_file_actions_addclose(&actions, pipefd[0]) != 0 ||
            posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO) != 0 ||
            posix_spawn_file_actions_addclose(&actions, pipefd[1]) != 0) {
            (void)posix_spawn_file_actions_destroy(&actions);
            fallback_dladdr_print(&info, i, addresses);
            (void)close(pipefd[0]);
            (void)close(pipefd[1]);
            continue;
        }

        pid_t pid;
        int spawnResult = posix_spawn(&pid, "/usr/bin/addr2line", &actions, NULL, (char* const*)argv, environ);
        (void)posix_spawn_file_actions_destroy(&actions);
        if (spawnResult != 0) {
            fallback_dladdr_print(&info, i, addresses);
            (void)close(pipefd[0]);
            (void)close(pipefd[1]);
            continue;
        }

        (void)close(pipefd[1]); // write end

        char addr2lineBuf[1024];
        ssize_t totalRead = 0;
        ssize_t bytesRead = 0;
        while (totalRead < (ssize_t)(sizeof(addr2lineBuf) - 1) &&
               (bytesRead = read(pipefd[0], addr2lineBuf + totalRead, sizeof(addr2lineBuf) - 1 - (size_t)totalRead)) >
                   0) {
            totalRead += bytesRead;
        }
        addr2lineBuf[totalRead] = '\0';
        (void)close(pipefd[0]);

        int status;
        (void)waitpid(pid, &status, 0);
        (void)status;

        if (totalRead > 0 && addr2lineBuf[totalRead - 1] == '\n') {
            addr2lineBuf[totalRead - 1] = '\0';
        }

        bool isUnknown =
            (totalRead == 0) || (addr2lineBuf[0] == '\0') || (addr2lineBuf[0] == '?' && addr2lineBuf[1] == '?');

        if (!isUnknown) {
            if (snprintf(lineBuf, sizeof(lineBuf), "#%-2d %s", i - 1, addr2lineBuf) < (int)sizeof(addr2lineBuf)) {
                syncWriteStringError(lineBuf);
            } else {
                fallback_dladdr_print(&info, i, addresses);
            }
        } else {
            fallback_dladdr_print(&info, i, addresses);
        }

#endif
    }
}

#endif

SyAtomicBool callstackMutex = {0};

void sy_print_callstack(void) {
    { // lock callstack mutex
#ifdef SYNC_TSAN_ENABLED
        __tsan_mutex_pre_lock(&callstackMutex, 0);
#endif
        bool expected = false;
        while (!(sy_atomic_bool_compare_exchange_weak(&callstackMutex, &expected, true, SY_MEMORY_ORDER_SEQ_CST))) {
            expected = false;
            sy_thread_yield();
        }
#ifdef SYNC_TSAN_ENABLED
        __tsan_mutex_post_lock(&callstackMutex, 0);
#endif
    }

#ifdef __EMSCRIPTEN__
    print_emscripten_callstack();
#elif defined(_WIN32) // also MinGW
    print_windows_callstack();
#elif defined(__MACH__) || defined(__GNUC__)
    print_posix_callstack();
#endif

    { // release callstack mutex
#ifdef SYNC_TSAN_ENABLED
        __tsan_mutex_pre_unlock(&callstackMutex, 0);
#endif
        sy_atomic_bool_store(&callstackMutex, false, SY_MEMORY_ORDER_SEQ_CST);
#ifdef SYNC_TSAN_ENABLED
        __tsan_mutex_post_unlock(&callstackMutex, 0);
#endif
    }
}
#endif
#endif // NDEBUG
#endif // SYNC_CUSTOM_BACKTRACE
