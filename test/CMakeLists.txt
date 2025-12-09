cmake_minimum_required (VERSION 3.16)

enable_testing()

set(SYNC_LIB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/src)

set(CORE_DIR ${SYNC_LIB_SRC_DIR}/core)
set(CORE_SOURCES ${CORE_DIR}/core.c)

if(SYNC_LIB_CODE_COVERAGE)
    message(STATUS "Enabling coverage for tests, and disabiling any sanitizers for test executables")
endif()

include(CheckCXXCompilerFlag)

function(add_sync_test_flags TEST_NAME ENABLE_ASAN ENABLE_UBSAN ENABLE_TSAN)
    set_target_properties(${TEST_NAME} PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
    )
    if(MSVC)
        target_compile_options(${TEST_NAME} PRIVATE /experimental:c11atomics)
        target_compile_options(${TEST_NAME} PRIVATE /W4 /WX /EHsc /Zi)
        if(ENABLE_ASAN)
            if(NOT SYNC_LIB_CODE_COVERAGE)
                target_compile_options(${TEST_NAME} PRIVATE /fsanitize=address)
            endif()
        endif()
    else()
        target_compile_options(${TEST_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
        if(ENABLE_ASAN)
            target_compile_options(${TEST_NAME} PRIVATE -fsanitize=address)
            target_link_options(${TEST_NAME} PRIVATE -fsanitize=address)
        endif()
        if(ENABLE_UBSAN)
            target_compile_options(${TEST_NAME} PRIVATE -fsanitize=undefined)
            target_link_options(${TEST_NAME} PRIVATE -fsanitize=undefined)
        endif()
        if(ENABLE_TSAN)
            target_compile_options(${TEST_NAME} PRIVATE -fsanitize=thread)
            target_link_options(${TEST_NAME} PRIVATE -fsanitize=thread)
        endif()
    endif()

    if(SYNC_LIB_CODE_COVERAGE)
        if(MSVC)
            # OpenCppCoverage?
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # llvm-cov
            target_compile_options(${TEST_NAME} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
            target_link_options(${TEST_NAME} PRIVATE -fprofile-instr-generate)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            # gcov
            target_compile_options(${TEST_NAME} PRIVATE --coverage)
            target_link_options(${TEST_NAME} PRIVATE --coverage)
        endif()
    endif()
endfunction()

function(add_core_sync_test TEST_NAME TEST_MAIN_CPP)
    add_executable(${TEST_NAME} ${TEST_MAIN_CPP} ${CORE_SOURCES})
    add_sync_test_flags(${TEST_NAME} TRUE TRUE FALSE)
    target_include_directories(${TEST_NAME} PRIVATE ${CORE_DIR})
    if(SYNC_LIB_CODE_COVERAGE)
        if(MSVC)
        cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH SYNC_ROOT_DIR)
        file(TO_NATIVE_PATH "${SYNC_ROOT_DIR}" SYNC_WINDOWS_NATIVE_ROOT_DIR)
        add_test(NAME ${TEST_NAME} 
            COMMAND OpenCppCoverage.exe 
            --sources ${SYNC_WINDOWS_NATIVE_ROOT_DIR}\\lib\\src\\core\\*
            --export_type binary:${TEST_NAME}.cov
            -- ${CMAKE_CURRENT_BINARY_DIR}\\Debug\\${TEST_NAME})
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # set proper file output name for llvm-cov
            add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
            set_property(TEST ${TEST_NAME} PROPERTY ENVIRONMENT "LLVM_PROFILE_FILE=${TEST_NAME}.profraw")
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
        endif()
    else()
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

        if(NOT MSVC)
            add_executable(${TEST_NAME}_ThreadSanitizer ${TEST_MAIN_CPP} ${CORE_SOURCES})
            add_sync_test_flags(${TEST_NAME}_ThreadSanitizer FALSE TRUE TRUE)
            target_include_directories(${TEST_NAME}_ThreadSanitizer PRIVATE ${CORE_DIR})
            add_test(NAME ${TEST_NAME}_ThreadSanitizer COMMAND ${TEST_NAME}_ThreadSanitizer)
        endif()
    endif()
endfunction()

function(add_core_sync_death_test TEST_NAME TEST_MAIN_CPP ERR_MSG)
    add_executable(${TEST_NAME} ${TEST_MAIN_CPP} ${CORE_SOURCES})
    add_sync_test_flags(${TEST_NAME} TRUE TRUE FALSE)
    target_include_directories(${TEST_NAME} PRIVATE ${CORE_DIR})   
    if(SYNC_LIB_CODE_COVERAGE AND MSVC)
        cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH SYNC_ROOT_DIR)
        file(TO_NATIVE_PATH "${SYNC_ROOT_DIR}" SYNC_WINDOWS_NATIVE_ROOT_DIR)
        add_test(NAME ${TEST_NAME} 
            COMMAND OpenCppCoverage.exe 
            --sources ${SYNC_WINDOWS_NATIVE_ROOT_DIR}\\lib\\src\\core\\*
            --export_type binary:${TEST_NAME}.cov
            -- ${CMAKE_CURRENT_BINARY_DIR}\\Debug\\${TEST_NAME})
    else()
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endif()
    set_tests_properties(${TEST_NAME} PROPERTIES
        PASS_REGULAR_EXPRESSION ${ERR_MSG}
    )

    if(NOT MSVC)
        add_executable(${TEST_NAME}_ThreadSanitizer ${TEST_MAIN_CPP} ${CORE_SOURCES})
        add_sync_test_flags(${TEST_NAME}_ThreadSanitizer FALSE TRUE TRUE)
        target_include_directories(${TEST_NAME}_ThreadSanitizer PRIVATE ${CORE_DIR})
        add_test(NAME ${TEST_NAME}_ThreadSanitizer COMMAND ${TEST_NAME}_ThreadSanitizer)
        set_tests_properties(${TEST_NAME}_ThreadSanitizer PROPERTIES
            PASS_REGULAR_EXPRESSION ${ERR_MSG}
        )
    endif()
endfunction()

# =================
# Core RwLock Tests
# =================
add_core_sync_test(SyncTestCore_RwLockInitDestroy "core/rwlock/rwlock_init_destroy.cpp")
add_core_sync_test(SyncTestCore_RwLockOneThreadShared "core/rwlock/rwlock_one_thread_shared.cpp")
add_core_sync_test(SyncTestCore_RwLockOneThreadExclusive "core/rwlock/rwlock_one_thread_exclusive.cpp")
add_core_sync_test(SyncTestCore_RwLockTwoThreadShared "core/rwlock/rwlock_two_thread_shared.cpp")
add_core_sync_test(SyncTestCore_RwLockTwoThreadExclusive "core/rwlock/rwlock_two_thread_exclusive.cpp")
add_core_sync_test(SyncTestCore_RwLockOneThreadReenterShared "core/rwlock/rwlock_one_thread_reenter_shared.cpp")
add_core_sync_test(SyncTestCore_RwLockOneThreadReenterExclusive "core/rwlock/rwlock_one_thread_reenter_exclusive.cpp")
add_core_sync_test(SyncTestCore_RwLockOneThreadElevateLock "core/rwlock/rwlock_one_thread_elevate_lock.cpp")
add_core_sync_test(SyncTestCore_RwLockTwoThreadNoElevate "core/rwlock/rwlock_two_thread_no_elevate.cpp")
add_core_sync_test(SyncTestCore_RwLockTwoThreadSuccessElevate "core/rwlock/rwlock_two_thread_success_elevate.cpp")
add_core_sync_test(SyncTestCore_RwLockTwoThreadDeadlock "core/rwlock/rwlock_two_thread_deadlock.cpp")
add_core_sync_test(SyncTestCore_RwLockThreeThreadDeadlock "core/rwlock/rwlock_three_thread_deadlock.cpp")
add_core_sync_test(SyncTestCore_RwLockDeadlockLateArrival "core/rwlock/rwlock_deadlock_late_arrival.cpp")
add_core_sync_test(SyncTestCore_RwLockDeadlockRapidRetry "core/rwlock/rwlock_deadlock_rapid_retry.cpp")
add_core_sync_test(SyncTestCore_RwLockTryAcquireBehavior "core/rwlock/rwlock_try_acquire_behavior.cpp")
add_core_sync_test(SyncTestCore_RwLockStressManyReaders "core/rwlock/rwlock_stress_many_readers.cpp")
add_core_sync_test(SyncTestCore_RwLockDeepReentrance "core/rwlock/rwlock_deep_reentrance.cpp")
add_core_sync_test(SyncTestCore_RwLockStressMixedWorkload "core/rwlock/rwlock_stress_mixed_workload.cpp")
add_core_sync_test(SyncTestCore_RwLockArrayReallocation "core/rwlock/rwlock_array_reallocation.cpp")
add_core_sync_test(SyncTestCore_RwLockExclusiveToSharedMultithread "core/rwlock/rwlock_exclusive_to_shared_multithread.cpp")

add_core_sync_death_test(SyncTestCoreDeath_RwLockDestroyOnSharedLock "core/rwlock_death/rwlock_death_destroy_on_shared_lock.cpp" ".*cannot destroy rwlock that was locked by another thread.*")
