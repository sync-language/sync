# Core Sync Requirements

Sync should (ideally) require very little. Sync requires access to memory allocators, but not necessarily `malloc` or `free`.

## Current Build Requirements

All build requirements are for C and C++. The Rust bindings are currently for no_std.

| C Standard Header | Why |
|-------------------|-----|
| `<stdbool.h>` | <ul><li>`bool`</li><li>`true`</li><li>`false`</li></ul> |
| `<stddef.h>` | <ul><li>`size_t`</li><li>`NULL`</li><li>`ptrdiff_t`</li></ul> |
| `<stdint.h>` | <ul><li>`int8_t`</li><li>`int16_t`</li><li>`int32_t`</li><li>`int64_t`</li><li>`uint8_t`</li><li>`uint16_t`</li><li>`uint32_t`</li><li>`uint64_t`</li><li>`uintptr_t`</li></ul> |
| `<stdlib.h>` | <ul><li>`aligned_alloc` and `free` (not MSVC) if not defined `SY_CUSTOM_ALIGNED_MALLOC_FREE` </li><li>`abort` if `SY_CUSTOM_DEFAULT_FATAL_ERROR_HANDLER` is not defined</li></ul> |
| `<stdio.h>` | <ul><li>`fprintf`, `fflush`, and `stderr` if `SYNC_CUSTOM_DEFAULT_WRITE_STRING_ERROR` is not defined</li></ul> |
| `<stdatomic.h>`| <ul><li>`memory_order_relaxed`</li><li>`memory_order_consume`</li><li>`memory_order_acquire`</li><li>`memory_order_release`</li><li>`memory_order_acq_rel`</li><li>`memory_order_seq_cst`</li><li>`atomic_load_explicit`</li><li>`atomic_store_explicit`</li><li>`atomic_fetch_add_explicit`</li><li>`atomic_fetch_sub_explicit`</li><li>`atomic_exchange_explicit`</li><li>`atomic_compare_exchange_weak_explicit`</li></ul> |
| `<unistd.h>` | <ul><li>`fsync` and `STDERR_FILENO` if `NDEBUG` is not defined</li></ul> |

| C++ Standard Header | Why |
|---------------------|-----|
| `<cstddef>` | <ul><li>`size_t`</li><li>`ptrdiff_t`</li></ul> |
| `<cstdint>` | <ul><li>`int8_t`</li><li>`int16_t`</li><li>`int32_t`</li><li>`int64_t`</li><li>`uint8_t`</li><li>`uint16_t`</li><li>`uint32_t`</li><li>`uint64_t`</li></ul> |

- C11 Compiler with standard library
- C++17 Compiler with standard library

## Header Inclusion Requirements

C headers (.h): C99

C++ headers (.hpp): C++17

## Overriding Default Behaviour

There are many macros available to disable certain default functionality depending on the constraints of your environment. You are required to link your own implementations of the disabled functions. You must provide them as definitions when building Sync from source. These macros can also be set from building as CMake, Make, Cargo, and Zig. See `lib/src/core/core.h`.

| Macro | Description |
|-------|--------------------|
| SYNC_CUSTOM_ALIGNED_MALLOC_FREE | <ul><li>`void* sy_aligned_malloc(size_t len, size_t align)`</li><li>`void sy_aligned_free(void* mem, size_t len, size_t align)`</li></ul> |
| SYNC_CUSTOM_PAGE_MEMORY | <ul><li>`void* sy_page_malloc(size_t len)`</li><li>`void sy_page_free(void* pagesStart, size_t len)`</li><li>`size_t sy_page_size()`</li><li>`void sy_make_pages_read_only(void* pagesStart, size_t len)`</li><li>`void sy_make_pages_read_write(void* pagesStart, size_t len)`</li></ul> |
| SYNC_DEFAULT_PAGE_ALIGNMENT | <ul><li>`SYNC_DEFAULT_PAGE_ALIGNMENT` default to `4096`</li></ul> |
| SYNC_CUSTOM_DEFAULT_FATAL_ERROR_HANDLER | <ul><li>`void sy_default_fatal_error_handler(const char* message)` defaults to calling the string error writer (default `sy_default_write_string_error`), then debug breaking / aborting</li></ul> |
| SYNC_CUSTOM_DEFAULT_WRITE_STRING_ERROR | <ul><li>`void sy_default_write_string_error(const char* message)` defaults printing to stderr and flushing, then debug breaking / aborting</li></ul> |
| SYNC_CUSTOM_THREAD_YIELD | <ul><li>`void sy_thread_yield(void)` yield the thread for spin locks</li></ul> |
| SYNC_CACHE_LINE_SIZE | <ul><li>`SYNC_CACHE_LINE_SIZE` default to 64</li></ul> |
| SYNC_CUSTOM_GET_FILE_INFO | <ul><li>`bool sy_get_file_info(const char* path, size_t pathLen, size_t* outFileSize)`</li></ul> |
| SYNC_CUSTOM_RELATIVE_TO_ABSOLUTE_PATH | <ul><li>`bool sy_relative_to_absolute_path(const char* relativePath, size_t relativePathLen, char* outAbsolutePath, size_t outAbsoluteBufSize)`</li></ul> |
| SYNC_DEATH_TEST | Modifies the default fatal handler to just exit with status code `1`, which can be intercepted by whatever runner you use, such as CTest |

## Future Plans

**IF** Rust becomes an official language supported by video game console SDKs (I know Rust has no_std support for nintendo switch), then a re-write is acceptable. Zig also is a good option.

Core functionality (memory allocation, atomics, error handling) will be implemented in C.
