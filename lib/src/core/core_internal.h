#ifndef SY_CORE_CORE_INTERNAL_H_
#define SY_CORE_CORE_INTERNAL_H_

#include "core.h"

// https://en.cppreference.com/w/c/program/unreachable

// Uses compiler specific extensions if possible.
#ifdef __GNUC__ // GCC, Clang, ICC

#define unreachable() (__builtin_unreachable())

#elif _MSC_VER // MSVC

#define unreachable() (__assume(false))

#else
// Even if no extension is used, undefined behavior is still raised by
// the empty function body and the noreturn attribute.

// The external definition of unreachable_impl must be emitted in a separated TU
// due to the rule for inline functions in C.

[[noreturn]] inline void unreachable_impl() {}
#define unreachable() (unreachable_impl())

#endif // Compiler specific

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SYNC_CACHE_LINE_SIZE
#define SYNC_CACHE_LINE_SIZE 64
#endif

extern void (*defaultFatalErrorHandlerFn)(const char* message);

/// Aligned memory allocation function required by sync. Can be overridden by defining
/// `SYNC_CUSTOM_ALIGNED_MALLOC_FREE`, in which you must supply a definition of the function at final link time.
/// @param len Amount of bytes to allocate. Must be non-zero. Must also be a multiple of `align`.
/// @param align The alignment of the returned pointer. Must be power of 2.
/// @return On success, returns the pointer to the beginning of the newly allocated memory. Must be freed with
/// `sy_aligned_free`. On failure returns a null pointer.
/// @warning If `len` is not a multiple of `align`, or `align` is not a power of `2`, the fatal error handler is
/// invoked.
extern void* sy_aligned_malloc(size_t len, size_t align);

/// Aligned memory freeing function required by sync. Can be overridden by defining
/// `SYNC_CUSTOM_ALIGNED_MALLOC_FREE`, in which you must supply a definition of the function at final link time.
/// @param len Amount of bytes to free. Must be non-zero. Must also be a multiple of `align`.
/// @param align The alignment of pointer to free. Must be power of 2.
/// @warning If `len` is not a multiple of `align`, or `align` is not a power of `2`, the fatal error handler is
/// invoked.
extern void sy_aligned_free(void* mem, size_t len, size_t align);

/// Allocates one or more pages of memory. If `SYNC_NO_PAGES` is defined, calls `sy_aligned_malloc` with an `align` of
/// `SYNC_DEFAULT_PAGE_ALIGNMENT` found in `core.c`. Alternatively, can be overridden by defining
/// `SYNC_CUSTOM_PAGE_MEMORY`
/// @param len Minimum amount of bytes to allocate pages for. Must be a multiple of `sy_page_size`
/// @return On success, returns the pointer to the beginning of the newly allocated pages / memory. Must be freed with
/// `sy_page_free`. On failure returns a null pointer.
/// @warning If `len` is not a multiple of `sy_page_size` the fatal error handler is
/// invoked.
extern void* sy_page_malloc(size_t len);

/// Frees one or more pages of memory. If `SYNC_NO_PAGES` is defined, calls `sy_aligned_free` with an `align` of
/// `SYNC_DEFAULT_PAGE_ALIGNMENT` found in `core.c`. Alternatively, can be overridden by defining
/// `SYNC_CUSTOM_PAGE_MEMORY`
/// @param pagesStart The beginning of the allocated pages / memory.
/// @param len The amount of bytes initially allocated. Must be a multiple of `sy_page_size`
/// @warning If `len` is not a multiple of `sy_page_size` the fatal error handler is
/// invoked.
extern void sy_page_free(void* pagesStart, size_t len);

/// If `SYNC_NO_PAGES` is defined, uses `SYNC_DEFAULT_PAGE_ALIGNMENT` found in `core.c`. Alternatively, can be
/// overridden by defining `SYNC_CUSTOM_PAGE_MEMORY`
/// @return If supported, the size of memory pages in bytes, or `SYNC_DEFAULT_PAGE_ALIGNMENT`
extern size_t sy_page_size(void);

/// Makes one or more virtual memory pages read only. If `SYNC_NO_PAGES` is defined, does nothing. Alternatively, can be
/// overridden by defining `SYNC_CUSTOM_PAGE_MEMORY`
/// @param pagesStart Pointer to the beginning of the pages memory.
/// @param len Amount of bytes that the pages span.
/// @warning If `len` is not a multiple of `sy_page_size` the fatal error handler is invoked.
extern void sy_make_pages_read_only(void* pagesStart, size_t len);

/// Makes one or more virtual memory pages read / write enabled. If `SYNC_NO_PAGES` is defined, does nothing.
/// Alternatively, can be overridden by defining `SYNC_CUSTOM_PAGE_MEMORY`
/// @param pagesStart Pointer to the beginning of the pages memory.
/// @param len Amount of bytes that the pages span.
/// @warning If `len` is not a multiple of `sy_page_size` the fatal error handler is invoked.
extern void sy_make_pages_read_write(void* pagesStart, size_t len);

/// @brief Same as https://en.cppreference.com/w/cpp/atomic/memory_order.html
typedef enum SyMemoryOrder {
    SY_MEMORY_ORDER_RELAXED = 0,
    SY_MEMORY_ORDER_CONSUME = 1,
    SY_MEMORY_ORDER_ACQUIRE = 2,
    SY_MEMORY_ORDER_RELEASE = 3,
    SY_MEMORY_ORDER_ACQ_REL = 4,
    SY_MEMORY_ORDER_SEQ_CST = 5,

    // Nice Vulkan Trick
    _SY_MEMORY_ORDER_MAX = 0x7FFFFFFF
} SyMemoryOrder;

typedef struct SyAtomicSizeT {
    volatile size_t value;
} SyAtomicSizeT;

/// https://en.cppreference.com/w/cpp/atomic/atomic_load.html
size_t sy_atomic_size_t_load(const SyAtomicSizeT* self, SyMemoryOrder order);

/// https://en.cppreference.com/w/cpp/atomic/atomic_store.html
void sy_atomic_size_t_store(SyAtomicSizeT* self, size_t newValue, SyMemoryOrder order);

/// https://en.cppreference.com/w/cpp/atomic/atomic_fetch_add.html
size_t sy_atomic_size_t_fetch_add(SyAtomicSizeT* self, size_t toAdd, SyMemoryOrder order);

/// https://en.cppreference.com/w/cpp/atomic/atomic_fetch_sub.html
size_t sy_atomic_size_t_fetch_sub(SyAtomicSizeT* self, size_t toSub, SyMemoryOrder order);

/// https://en.cppreference.com/w/cpp/atomic/atomic_exchange.html
size_t sy_atomic_size_t_exchange(SyAtomicSizeT* self, size_t newValue, SyMemoryOrder order);

/// https://en.cppreference.com/w/cpp/atomic/atomic_compare_exchange.html
bool sy_atomic_size_t_compare_exchange_weak(SyAtomicSizeT* self, size_t* expected, size_t desired, SyMemoryOrder order);

typedef struct SyAtomicBool {
    volatile bool value;
} SyAtomicBool;

/// https://en.cppreference.com/w/cpp/atomic/atomic_load.html
bool sy_atomic_bool_load(const SyAtomicBool* self, SyMemoryOrder order);

/// https://en.cppreference.com/w/cpp/atomic/atomic_store.html
void sy_atomic_bool_store(SyAtomicBool* self, bool newValue, SyMemoryOrder order);

/// https://en.cppreference.com/w/cpp/atomic/atomic_exchange.html
bool sy_atomic_bool_exchange(SyAtomicBool* self, bool newValue, SyMemoryOrder order);

/// https://en.cppreference.com/w/cpp/atomic/atomic_compare_exchange.html
bool sy_atomic_bool_compare_exchange_weak(SyAtomicBool* self, bool* expected, bool desired, SyMemoryOrder order);

/// Yields execution of this thread. Can be overridden by defining `SYNC_CUSTOM_THREAD_YIELD` and then linking your own.
extern void sy_thread_yield(void);

/// Enjoy reading
/// As sync code executes, it will inevitably call into external functions (C functions) due to being embeddable.
/// As such, some static analysis between external and sync function calls for the compiler is not fully possible.
///
/// This presents a massive challenge when trying to acquire locks, as we cannot prevent, at sync compile time, that
/// acquiring an exclusive lock has not already been acquired by that same thread as a shared lock.
///
/// How we can solve this is by combining two methods. Firstly, supporting re-entrant locks. This will mean that a
/// thread trying to re-acquire a lock in the same way as it did before is not a problem. Secondly, allow to "elevate" a
/// lock, meaning turn a shared lock into an exclusive lock without giving up acquisition to another thread. If a thread
/// has a shared lock, and so does another, fail to "elevate" the lock into an exclusive lock.
typedef struct SyRawRwLock {
    SyAtomicBool fence;
    SyAtomicSizeT exclusiveId;
    SyAtomicSizeT exclusiveCount;
    size_t* readers;
    int32_t readerLen;
    int32_t readerCapacity;
    size_t* threadsWantElevate;
    int32_t threadsWantElevateLen;
    int32_t threadsWantElevateCapacity;
    SyAtomicSizeT deadlockGeneration;
} SyRawRwLock;

typedef enum SyAcquireErr {
    SY_ACQUIRE_ERR_NONE = 0,
    SY_ACQUIRE_ERR_OUT_OF_MEMORY = 1,
    SY_ACQUIRE_ERR_SHARED_HAS_EXCLUSIVE = 2,
    SY_ACQUIRE_ERR_EXCLUSIVE_HAS_OTHER_READERS = 3,
    SY_ACQUIRE_ERR_EXCLUSIVE_HAS_EXCLUSIVE = 4,
    SY_ACQUIRE_ERR_DEADLOCK = 5,

    _SY_ACQUIRE_ERR_MAX = 0x7FFFFFFF
} SyAcquireErr;

/// Frees any memory associated with the RwLock.
/// @param self Non-null pointer to lock object
/// @warning It is a fatal error if any thread has either a shared or exclusive lock on `self`.
void sy_raw_rwlock_destroy(SyRawRwLock* self);

/// Attempts to acquire a shared lock to `self`. If the calling thread already has an exclusive or shared lock, the lock
/// re-enters and succeeds. Release the lock with `sy_raw_rwlock_release_shared`.
/// @param self Non-null pointer to lock object
/// @return If the acquire was a success, `SY_ACQUIRE_ERR_NONE`. If another thread has an exclusive lock,
/// `SY_ACQUIRE_ERR_SHARED_HAS_EXCLUSIVE`. If memory failed to allocate, `SY_ACQUIRE_ERR_OUT_OF_MEMORY`.
/// @warning `sy_raw_rwlock_release_shared` must be called to release the lock. Failure to do so is undefined behavior.
SyAcquireErr sy_raw_rwlock_try_acquire_shared(SyRawRwLock* self);

/// Acquires a shared lock to `self`. If the calling thread already has an exclusive lock, the lock re-enters
/// and succeeds. Release the lock with `sy_raw_rwlock_release_shared`.
/// @param self Non-null pointer to lock object
/// @return `SY_ACQUIRE_ERR_NONE` on success, otherwise
/// `SY_ACQUIRE_ERR_OUT_OF_MEMORY` due to allocation failure.
/// @warning `sy_raw_rwlock_release_shared` must be called to release the lock. Failure to do so is undefined behavior.
SyAcquireErr sy_raw_rwlock_acquire_shared(SyRawRwLock* self);

/// Releases a shared lock to `self` from this thread. If there are multiple shared acquisitions on this thread,
/// `sy_raw_rwlock_release_shared` must be called the same amount of times.
/// @param self Non-null pointer to lock object
/// @warning It is a fatal error if a different thread has an exclusive lock on `self`, or if no thread has a shared
/// lock.
void sy_raw_rwlock_release_shared(SyRawRwLock* self);

/// Attempts to acquire an exclusive lock to `self`. If the calling thread already has an exclusive lock, the
/// lock re-enters and succeeds. If no other thread has a shared lock, and this thread does, also the lock re-enters
/// with the lock state elevated to exclusive for this thread and succeeds. Release the lock with
/// `sy_raw_rwlock_release_exclusive`.
/// @param self Non-null pointer to lock object
/// @return If the acquire was a success, `SY_ACQUIRE_ERR_NONE`. If another thread has an exclusive lock,
/// `SY_ACQUIRE_ERR_EXCLUSIVE_HAS_EXCLUSIVE`. If another thread has a shared lock,
/// `SY_ACQUIRE_ERR_EXCLUSIVE_HAS_OTHER_READERS`. If two or more threads are trying to elevate from shared to exclusive,
/// `SY_ACQUIRE_ERR_DEADLOCK`. If memory allocation fails, `SY_ACQUIRE_ERR_OUT_OF_MEMORY`.
/// @warning `sy_raw_rwlock_release_exclusive` must be called to release the lock. Failure to do so is undefined
/// behavior. It is best to not re-try the lock if `SY_ACQUIRE_ERR_EXCLUSIVE_HAS_OTHER_READERS` is returned,
/// as this can easily lead to deadlocks. If `SY_ACQUIRE_ERR_DEADLOCK` is returned, two threads are competing to elevate
/// their locks to exclusive. Analyze carefully where this is happening at the call-site.
SyAcquireErr sy_raw_rwlock_try_acquire_exclusive(SyRawRwLock* self);

/// Attempts to acquire an exclusive lock to `self`. If the calling thread already has an exclusive lock, the
/// lock re-enters and succeeds. If no other thread has a shared lock, and this thread does, also the lock re-enters
/// with the lock state elevated to exclusive for this thread and succeeds. Release the lock with
/// `sy_raw_rwlock_release_exclusive`.
/// @param self Non-null pointer to lock object
/// @return If the acquire was a success, `SY_ACQUIRE_ERR_NONE`. If another thread is also trying to elevate their lock
/// from shared to exclusive, `SY_ACQUIRE_ERR_DEADLOCK`. If memory allocation fails, `SY_ACQUIRE_ERR_OUT_OF_MEMORY`.
/// @warning `sy_raw_rwlock_release_exclusive` must be called to release the lock. Failure to do so is undefined
/// behavior. This function also does not re-try the lock if `SY_ACQUIRE_ERR_EXCLUSIVE_HAS_OTHER_READERS` is returned,
/// as this can easily lead to deadlocks. For instance, if thread A and B both have shared locks, and then both want to
/// elevate them to exclusive, this would be a deadlock. If `SY_ACQUIRE_ERR_DEADLOCK` is returned, two threads are
/// competing to elevate their locks to exclusive. Analyze carefully where this is happening at the call-site.
SyAcquireErr sy_raw_rwlock_acquire_exclusive(SyRawRwLock* self);

/// Releases a shared lock to `self` from this thread. If there are multiple shared acquisitions on this thread,
/// `sy_raw_rwlock_release_shared` must be called the same amount of times.
/// @param self Non-null pointer to lock object
/// @warning It is a fatal error if a different thread has an exclusive lock on `self`, or if no thread has an exclusive
/// lock.
void sy_raw_rwlock_release_exclusive(SyRawRwLock* self);

#ifndef SYNC_NO_FILESYSTEM

/// Fetches relevant file info from a file on disk. This functionality is not strictly required for Sync to work, but is
/// simply useful. The compiler can function without it if you do not call any file system specific compiler
/// functionality. Can be overridden by defining `SYNC_CUSTOM_GET_FILE_INFO`, in which you must supply a definition of
/// the function at final link time. Since it's not required the compiler, you can link a function that just returns
/// false, and not use any file system logic for the compiler.
/// @param path Non-null pointer to a path string. Does not need to be null terminated.
/// @param pathLen Length, in bytes, of the string pointed to by `path`.
/// @param outFileSize Non-null pointer where the size of the file at `path` will be written to.
/// @return `true` if succeeds to get the info, otherwise `false`. This could mean not a valid file.
extern bool sy_get_file_info(const char* path, size_t pathLen, size_t* outFileSize);

/// Converts a relative path to an absolute path. This functionality is not strictly required for Sync to work, but it
/// is simply useful. The compiler can function without it if you do not call any file system specific compiler
/// functionality. Can be overridden by defining
/// `SYNC_CUSTOM_RELATIVE_TO_ABSOLUTE_PATH`, in which you must supply a definition of the function at final link time.
/// Since it's not required the compiler, you can link a function that just returns false, and not use any file system
/// logic for the compiler.c
/// @param relativePath Non-null pointer to the relative path string. Does not need to be null terminated.
/// @param relativePathLen Length, in bytes, of the string pointed to by `relativePath`.
/// @param outAbsolutePath Non-null pointer where the absolute path will be written to. Will have a null terminator
/// written to the end.
/// @param outAbsoluteBufSize Size of the buffer pointed to by `outAbsolutePath`.
/// @return `true` if succeeds to convert to absolute, otherwise `false`.
extern bool sy_relative_to_absolute_path(const char* relativePath, size_t relativePathLen, char* outAbsolutePath,
                                         size_t outAbsoluteBufSize);

#endif // SYNC_NO_FILESYSTEM

#ifdef __cplusplus
}
#endif

#endif
