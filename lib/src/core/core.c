#include "core.h"
#include "core_internal.h"
#include <stdlib.h>

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
    if ((len % pageSize) != 0) {
        defaultFatalErrorHandlerFn("[sy_page_malloc] len must be multiple of sy_page_size");
    }
#if defined(SYNC_NO_PAGES)
    sy_aligned_free(pagesStart, len, pageSize);
#elif defined(_WIN32)
    bool result = VirtualFree(pagesStart, 0, MEM_RELEASE);
    if (result == false) {
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
    const bool success = VirtualProtect(pagesStart, len, newProtect, &oldProtect);
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
    const bool success = VirtualProtect(pagesStart, len, newProtect, &oldProtect);
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
#elif defined(_POSIX_VERSION)
#if __has_include(<sched.h>)
    (void)sched_yield();
#endif
#elif defined(__x86_64__)
    _mm_pause();
#elif defined(__aarch64__)
    __yield();
#elif defined(__riscv)
    __builtin_riscv_pause();
#else
// idk man
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
    if (fetched >= (SIZE_MAX - 1)) {
        defaultFatalErrorHandlerFn("[initializeThisThreadId] reached max value for thread id generator (how?)");
    }

    threadLocalThreadId = fetched + 1; // don't start at 0
}

/// @return `false` if out of memory, otherwise `true`
static bool addThisThreadToReaders(SyRawRwLock* self) {
    initializeThisThreadId();
    const size_t threadId = threadLocalThreadId;

    const int32_t currentLen = self->readerLen;
    const int32_t currentCapacity = self->readerCapacity;

    if (currentLen == currentCapacity) { // reallocate if necessary
        if (currentCapacity > ((INT32_MAX / 2) - 1)) {
            defaultFatalErrorHandlerFn("[addThisThreadToReaders] reached max value for reader capacity (how?)");
        }
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
    if (currentExclusiveId != 0) {
        defaultFatalErrorHandlerFn("[sy_raw_rwlock_destroy] cannot destroy rwlock when a thread has exclusive access");
    }
    if (currentReadersLen > 0) {
        defaultFatalErrorHandlerFn("[sy_raw_rwlock_destroy] cannot destroy rwlock that was locked by another thread");
    }
    if (currentWantElevateLen > 0) {
        defaultFatalErrorHandlerFn(
            "[sy_raw_rwlock_destroy] cannot destroy rwlock that other threads wanting to elevate");
    }

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
    if (currentExclusiveId != 0 && currentExclusiveId != threadId) {
        defaultFatalErrorHandlerFn(
            "[sy_raw_rwlock_release_exclusive] cannot release shared lock when another thread has an exclusive lock");
    }
    if (self->readerLen == 0) {
        defaultFatalErrorHandlerFn(
            "[sy_raw_rwlock_release_exclusive] cannot release shared lock if no thread has a shared lock");
    }

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
                if (currentElevateCapacity > ((INT32_MAX / 2) - 1)) {
                    defaultFatalErrorHandlerFn(
                        "[sy_raw_rwlock_try_acquire_exclusive] reached max value for elevate capacity (how?)");
                }
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
            if (oldDeadlockGeneration >= (SIZE_MAX - 1)) {
                defaultFatalErrorHandlerFn(
                    "[sy_raw_rwlock_try_acquire_exclusive] too many deadlocks have occurred on this rwlock");
            }

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
    if (currentExclusiveId == 0) {
        defaultFatalErrorHandlerFn(
            "[sy_raw_rwlock_release_exclusive] cannot release exclusive lock when no thread has acquired");
    }
    if (currentExclusiveId != threadId) {
        defaultFatalErrorHandlerFn(
            "[sy_raw_rwlock_release_exclusive] cannot release exclusive lock that was locked by another thread");
    }

    const size_t currentExclusiveCount = sy_atomic_size_t_fetch_sub(&self->exclusiveCount, 1, SY_MEMORY_ORDER_SEQ_CST);
    if (currentExclusiveCount == 1) {
        sy_atomic_size_t_store(&self->exclusiveId, 0, SY_MEMORY_ORDER_SEQ_CST);
    }
    releaseFence(self);
}
