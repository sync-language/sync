cmake_minimum_required (VERSION 3.16)

if(WIN32)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/find_windows_sdk.cmake)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/base_compiler_flags.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/sanitizers_coverage.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/simd_target.cmake)

enable_testing()

# Configure test runner for Emscripten
if(EMSCRIPTEN)
    find_program(NODE_EXECUTABLE NAMES node nodejs REQUIRED)
    set(CMAKE_CROSSCOMPILING_EMULATOR "${NODE_EXECUTABLE};--experimental-wasm-threads")
endif()

set(SYNC_LIB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/src)

set(CORE_DIR ${SYNC_LIB_SRC_DIR}/core)
set(CORE_SOURCES ${CORE_DIR}/core.c)

include(CheckCXXCompilerFlag)

function(add_sync_test_flags TEST_NAME ENABLE_ASAN ENABLE_TSAN)
    set_target_properties(${TEST_NAME} PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
    )

    if(EMSCRIPTEN)
        set_target_properties(${TEST_NAME} PROPERTIES
            CXX_STANDARD 23
            CXX_STANDARD_REQUIRED YES
        )
    else()
        set_target_properties(${TEST_NAME} PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED YES
        )
    endif()

    set_base_compiler_flags(${TEST_NAME})
    if(EMSCRIPTEN)
        target_compile_options(${TEST_NAME} PRIVATE -Wno-c2y-extensions)
    endif()

    if(ENABLE_ASAN)
        enable_asan(${TEST_NAME})
    elseif(ENABLE_TSAN)
        enable_tsan(${TEST_NAME})
    endif()

    if(SYNC_LIB_CODE_COVERAGE)
        enable_coverage(${TEST_NAME})
        target_compile_definitions(${TEST_NAME} PRIVATE SYNC_LIB_CODE_COVERAGE=1)
    endif()

    add_simd_flags_to_target(sync SYNC_NO_AVX2 SYNC_NO_AVX512 SYNC_WASM_USE_SIMD SYNC_NO_RVV)

    if(EMSCRIPTEN)
        target_compile_options(${TEST_NAME} PRIVATE -pthread)
        target_link_options(${TEST_NAME} PRIVATE -pthread)
        target_link_options(${TEST_NAME} PRIVATE "SHELL:-s PTHREAD_POOL_SIZE=8")

        target_link_options(${TEST_NAME} PRIVATE
            "SHELL:-s ASSERTIONS=1"
            --profiling-funcs
            -g
        )

        # Give node filesystem access
        target_link_options(${TEST_NAME} PRIVATE "-sNODERAWFS=1")
    endif()

endfunction()

# function(add_core_sync_test TEST_NAME TEST_MAIN_CPP)
#     add_executable(${TEST_NAME} ${TEST_MAIN_CPP} ${CORE_SOURCES})
#     if(SYNC_DISABLE_ASAN_TESTS)
#         add_sync_test_flags(${TEST_NAME} FALSE FALSE)
#     else()
#         add_sync_test_flags(${TEST_NAME} TRUE FALSE)
#     endif()
#     target_include_directories(${TEST_NAME} PRIVATE ${CORE_DIR})
#     if(SYNC_LIB_CODE_COVERAGE)
#         if(MSVC)
#         cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH SYNC_ROOT_DIR)
#         file(TO_NATIVE_PATH "${SYNC_ROOT_DIR}" SYNC_WINDOWS_NATIVE_ROOT_DIR)
#         add_test(NAME ${TEST_NAME}
#             COMMAND OpenCppCoverage.exe
#             --sources ${SYNC_WINDOWS_NATIVE_ROOT_DIR}\\lib\\src\\core\\*
#             --export_type binary:${TEST_NAME}.cov
#             -- ${CMAKE_CURRENT_BINARY_DIR}\\Debug\\${TEST_NAME})
#         elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#             # set proper file output name for llvm-cov
#             add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
#             set_property(TEST ${TEST_NAME} PROPERTY ENVIRONMENT "LLVM_PROFILE_FILE=${TEST_NAME}.profraw")
#         elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#             add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
#         endif()
#     else()
#         add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

#         if((NOT MSVC) AND (NOT EMSCRIPTEN) AND (NOT SYNC_SKIP_TSAN_TESTS))
#             add_executable(${TEST_NAME}_ThreadSanitizer ${TEST_MAIN_CPP} ${CORE_SOURCES})
#             add_sync_test_flags(${TEST_NAME}_ThreadSanitizer FALSE TRUE)
#             target_include_directories(${TEST_NAME}_ThreadSanitizer PRIVATE ${CORE_DIR})
#             add_test(NAME ${TEST_NAME}_ThreadSanitizer COMMAND ${TEST_NAME}_ThreadSanitizer)
#         endif()
#     endif()
# endfunction()

# function(add_core_sync_death_test TEST_NAME TEST_MAIN_CPP ERR_MSG)
#     add_executable(${TEST_NAME} ${TEST_MAIN_CPP} ${CORE_SOURCES})
#     add_sync_test_flags(${TEST_NAME} FALSE FALSE)
#     target_compile_definitions(${TEST_NAME} PRIVATE SYNC_DEATH_TEST=1)
#     target_include_directories(${TEST_NAME} PRIVATE ${CORE_DIR})
#     add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
#     set_tests_properties(${TEST_NAME} PROPERTIES
#         WILL_FAIL TRUE
#         FAIL_REGULAR_EXPRESSION ${ERR_MSG}
#     )

#     if((NOT MSVC) AND (NOT EMSCRIPTEN) AND (NOT SYNC_SKIP_TSAN_TESTS))
#         add_executable(${TEST_NAME}_ThreadSanitizer ${TEST_MAIN_CPP} ${CORE_SOURCES})
#         add_sync_test_flags(${TEST_NAME}_ThreadSanitizer FALSE TRUE)
#         target_compile_definitions(${TEST_NAME}_ThreadSanitizer PRIVATE SYNC_DEATH_TEST=1)
#         target_include_directories(${TEST_NAME}_ThreadSanitizer PRIVATE ${CORE_DIR})
#         add_test(NAME ${TEST_NAME}_ThreadSanitizer COMMAND ${TEST_NAME}_ThreadSanitizer)
#         set_tests_properties(${TEST_NAME}_ThreadSanitizer PROPERTIES
#             WILL_FAIL TRUE
#             FAIL_REGULAR_EXPRESSION ${ERR_MSG}
#         )
#     endif()
# endfunction()

# =================
# Core RwLock Tests
# =================
# add_core_sync_test(SyncTestCore_RwLockInitDestroy "lib_tests/core/rwlock/rwlock_init_destroy.cpp")
# add_core_sync_test(SyncTestCore_RwLockOneThreadShared "lib_tests/core/rwlock/rwlock_one_thread_shared.cpp")
# add_core_sync_test(SyncTestCore_RwLockOneThreadExclusive "lib_tests/core/rwlock/rwlock_one_thread_exclusive.cpp")
# add_core_sync_test(SyncTestCore_RwLockTwoThreadShared "lib_tests/core/rwlock/rwlock_two_thread_shared.cpp")
# add_core_sync_test(SyncTestCore_RwLockTwoThreadExclusive "lib_tests/core/rwlock/rwlock_two_thread_exclusive.cpp")
# add_core_sync_test(SyncTestCore_RwLockOneThreadReenterShared "lib_tests/core/rwlock/rwlock_one_thread_reenter_shared.cpp")
# add_core_sync_test(SyncTestCore_RwLockOneThreadReenterExclusive "lib_tests/core/rwlock/rwlock_one_thread_reenter_exclusive.cpp")
# add_core_sync_test(SyncTestCore_RwLockOneThreadElevateLock "lib_tests/core/rwlock/rwlock_one_thread_elevate_lock.cpp")
# add_core_sync_test(SyncTestCore_RwLockTwoThreadNoElevate "lib_tests/core/rwlock/rwlock_two_thread_no_elevate.cpp")
# add_core_sync_test(SyncTestCore_RwLockTwoThreadSuccessElevate "lib_tests/core/rwlock/rwlock_two_thread_success_elevate.cpp")
# add_core_sync_test(SyncTestCore_RwLockTwoThreadDeadlock "lib_tests/core/rwlock/rwlock_two_thread_deadlock.cpp")
# add_core_sync_test(SyncTestCore_RwLockThreeThreadDeadlock "lib_tests/core/rwlock/rwlock_three_thread_deadlock.cpp")
# add_core_sync_test(SyncTestCore_RwLockDeadlockLateArrival "lib_tests/core/rwlock/rwlock_deadlock_late_arrival.cpp")
# add_core_sync_test(SyncTestCore_RwLockDeadlockRapidRetry "lib_tests/core/rwlock/rwlock_deadlock_rapid_retry.cpp")
# add_core_sync_test(SyncTestCore_RwLockTryAcquireBehavior "lib_tests/core/rwlock/rwlock_try_acquire_behavior.cpp")
# add_core_sync_test(SyncTestCore_RwLockStressManyReaders "lib_tests/core/rwlock/rwlock_stress_many_readers.cpp")
# add_core_sync_test(SyncTestCore_RwLockDeepReentrance "lib_tests/core/rwlock/rwlock_deep_reentrance.cpp")
# add_core_sync_test(SyncTestCore_RwLockStressMixedWorkload "lib_tests/core/rwlock/rwlock_stress_mixed_workload.cpp")
# add_core_sync_test(SyncTestCore_RwLockArrayReallocation "lib_tests/core/rwlock/rwlock_array_reallocation.cpp")
# add_core_sync_test(SyncTestCore_RwLockExclusiveToSharedMultithread "lib_tests/core/rwlock/rwlock_exclusive_to_shared_multithread.cpp")

# add_core_sync_death_test(SyncTestCoreDeath_RwLockDestroyOnSharedLock "lib_tests/core/rwlock_death/rwlock_death_destroy_on_shared_lock.cpp" ".*cannot destroy rwlock that was locked by another thread.*")

# add_core_sync_test(SyncTestCore_FilesystemGetFileSizeSmall "lib_tests/core/filesystem/filesystem_get_file_size_this_file_small.cpp")
# add_core_sync_test(SyncTestCore_FilesystemGetFileSizeBigger "lib_tests/core/filesystem/filesystem_get_file_size_this_file_bigger.cpp")
# add_core_sync_test(SyncTestCore_FilesystemGetFileSizeNotAFile "lib_tests/core/filesystem/filesystem_get_file_size_not_a_file.cpp")
# add_core_sync_test(SyncTestCore_FilesystemRelativeToAbsoluteDir "lib_tests/core/filesystem/filesystem_relative_to_absolute_dir.cpp")

add_executable(NewTests "lib/test_main.cpp" "lib/unit_tests/rwlock.cpp" "lib/unit_tests/string.cpp" "lib/unit_tests/gen_pool.cpp")
target_link_libraries(NewTests PRIVATE sync::sync)
target_include_directories(NewTests PRIVATE "../lib/src" "lib")
add_sync_test_flags(NewTests ${SYNC_LIB_ASAN} ${SYNC_LIB_TSAN})
add_test(NAME NewTests COMMAND NewTests)
