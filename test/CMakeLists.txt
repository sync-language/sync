cmake_minimum_required (VERSION 3.16)

enable_testing()

# Configure test runner for Emscripten
if(EMSCRIPTEN)
    find_program(NODE_EXECUTABLE NAMES node nodejs REQUIRED)
    set(CMAKE_CROSSCOMPILING_EMULATOR "${NODE_EXECUTABLE};--experimental-wasm-threads")
endif()

if(APPLE AND SYNC_SANDBOX_TESTS)
    set(CMAKE_CROSSCOMPILING_EMULATOR "sandbox-exec" "-f" "${CMAKE_CURRENT_SOURCE_DIR}/sandbox.sb")
endif()

# idk this should probably work
if(LINUX AND SYNC_SANDBOX_TESTS)
    find_program(BWRAP_EXECUTABLE bwrap)
    if(BWRAP_EXECUTABLE)
        set(CMAKE_CROSSCOMPILING_EMULATOR
            "${BWRAP_EXECUTABLE}"
            "--unshare-net"           # No network
            "--ro-bind" "/" "/"       # Read-only root
            "--bind" "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}"  # Writable build dir
            "--bind" "/tmp" "/tmp"
            "--dev" "/dev"
            "--proc" "/proc"
        )
    endif()
endif()

set(SYNC_LIB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/src)

set(CORE_DIR ${SYNC_LIB_SRC_DIR}/core)
set(CORE_SOURCES ${CORE_DIR}/core.c)

if(SYNC_LIB_CODE_COVERAGE)
    message(STATUS "Enabling coverage for tests, and disabiling any sanitizers for test executables")
endif()

include(CheckCXXCompilerFlag)

function(add_sync_test_flags TEST_NAME ENABLE_ASAN ENABLE_UBSAN ENABLE_TSAN)
    if(EMSCRIPTEN)
        set_target_properties(${TEST_NAME} PROPERTIES
            C_STANDARD 11
            C_STANDARD_REQUIRED YES
            CXX_STANDARD 23  # __COUNTER__ for doctest
            CXX_STANDARD_REQUIRED YES
        )
    else()
        set_target_properties(${TEST_NAME} PROPERTIES
            C_STANDARD 11
            C_STANDARD_REQUIRED YES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED YES
        )
    endif()

    if(MSVC)
        target_compile_options(${TEST_NAME} PRIVATE /experimental:c11atomics)
        target_compile_options(${TEST_NAME} PRIVATE /W4 /WX /EHsc /Zi)
        target_compile_options(${TEST_NAME} PRIVATE /MP)
        if(ENABLE_ASAN)
            if(NOT SYNC_LIB_CODE_COVERAGE)
                target_compile_options(${TEST_NAME} PRIVATE /fsanitize=address)
            endif()
        endif()
    else()
        target_compile_options(${TEST_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
        if(NOT EMSCRIPTEN)
            if(ENABLE_ASAN)
                target_compile_options(${TEST_NAME} PRIVATE -fsanitize=address)
                target_link_options(${TEST_NAME} PRIVATE -fsanitize=address)
            endif()
            if(ENABLE_UBSAN)
                target_compile_options(${TEST_NAME} PRIVATE -fsanitize=undefined)
                target_link_options(${TEST_NAME} PRIVATE -fsanitize=undefined)
            endif()
            if(ENABLE_TSAN)
                target_compile_options(${TEST_NAME} PRIVATE -fsanitize=thread -fno-PIE)
                target_link_options(${TEST_NAME} PRIVATE -fsanitize=thread -no-pie)
            endif()
        
        else()
            target_compile_options(${TEST_NAME} PRIVATE "-Wno-c2y-extensions")
        endif()
    endif()

    if(SYNC_LIB_CODE_COVERAGE)
        target_compile_definitions(${TEST_NAME} PRIVATE SYNC_LIB_CODE_COVERAGE=1)
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

    if(HAS_SSE2)
        if(MSVC)
            target_compile_options(${TEST_NAME} PRIVATE /arch:SSE2)
        else()
            target_compile_options(${TEST_NAME} PRIVATE -msse2)
        endif()
    endif()

    if(NOT ${SYNC_NO_AVX2})
        if(HAS_AVX2)
            if(MSVC)
                target_compile_options(${TEST_NAME} PRIVATE /arch:AVX2)
            else()
                target_compile_options(${TEST_NAME} PRIVATE -mavx2)
            endif()
        endif()
    endif()

    if(NOT ${SYNC_NO_AVX512})
        if(HAS_AVX2)
            if(MSVC)
                target_compile_options(${TEST_NAME} PRIVATE /arch:AVX512)
            else()
                target_compile_options(${TEST_NAME} PRIVATE -mavx512f -mavx512bw)
            endif()
        endif()
    endif()


    if(EMSCRIPTEN AND SYNC_WASM_USE_SIMD)
        # https://webassembly.org/features/
        # Fixed-width SIMD supported by everything
        target_compile_options(${TEST_NAME} PRIVATE -msimd128)
    endif()

    if(EMSCRIPTEN)
        target_compile_options(${TEST_NAME} PRIVATE -pthread)
        target_link_options(${TEST_NAME} PRIVATE -pthread)
        target_link_options(${TEST_NAME} PRIVATE "SHELL:-s PTHREAD_POOL_SIZE=8")

        target_link_options(${TEST_NAME} PRIVATE
            "-s ASSERTIONS=1 -s DEMANGLE_SUPPORT=1 --profiling-funcs -g"
        )

        # Give node filesystem access
        target_link_options(${TEST_NAME} PRIVATE "-sNODERAWFS=1")
    endif()

endfunction()

function(add_core_sync_test TEST_NAME TEST_MAIN_CPP)
    add_executable(${TEST_NAME} ${TEST_MAIN_CPP} ${CORE_SOURCES})
    add_sync_test_flags(${TEST_NAME} ${SYNC_LIB_ASAN} TRUE FALSE)
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

        if((NOT MSVC) AND (NOT EMSCRIPTEN) AND (NOT SYNC_SKIP_TSAN_TESTS))
            add_executable(${TEST_NAME}_ThreadSanitizer ${TEST_MAIN_CPP} ${CORE_SOURCES})
            add_sync_test_flags(${TEST_NAME}_ThreadSanitizer FALSE TRUE TRUE)
            target_include_directories(${TEST_NAME}_ThreadSanitizer PRIVATE ${CORE_DIR})
            add_test(NAME ${TEST_NAME}_ThreadSanitizer COMMAND ${TEST_NAME}_ThreadSanitizer)
        endif()
    endif()
endfunction()

function(add_core_sync_death_test TEST_NAME TEST_MAIN_CPP ERR_MSG)
    add_executable(${TEST_NAME} ${TEST_MAIN_CPP} ${CORE_SOURCES})
    add_sync_test_flags(${TEST_NAME} ${SYNC_LIB_ASAN} TRUE FALSE)
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
    
    if((NOT MSVC) AND (NOT EMSCRIPTEN) AND (NOT SYNC_SKIP_TSAN_TESTS))
        add_executable(${TEST_NAME}_ThreadSanitizer ${TEST_MAIN_CPP} ${CORE_SOURCES})
        add_sync_test_flags(${TEST_NAME}_ThreadSanitizer FALSE TRUE TRUE)
        target_include_directories(${TEST_NAME}_ThreadSanitizer PRIVATE ${CORE_DIR})
        add_test(NAME ${TEST_NAME}_ThreadSanitizer COMMAND ${TEST_NAME}_ThreadSanitizer)
    endif()
endfunction()

function(add_core_sync_death_test TEST_NAME TEST_MAIN_CPP ERR_MSG)
    add_executable(${TEST_NAME} ${TEST_MAIN_CPP} ${CORE_SOURCES})
    add_sync_test_flags(${TEST_NAME} ${SYNC_LIB_ASAN} TRUE FALSE)
    target_include_directories(${TEST_NAME} PRIVATE ${CORE_DIR})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES
        PASS_REGULAR_EXPRESSION ${ERR_MSG}
    )
    if((NOT MSVC) AND (NOT EMSCRIPTEN) AND (NOT SYNC_SKIP_TSAN_TESTS))
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

add_core_sync_test(SyncTestCore_FilesystemGetFileSizeSmall "core/filesystem/filesystem_get_file_size_this_file_small.cpp")
add_core_sync_test(SyncTestCore_FilesystemGetFileSizeBigger "core/filesystem/filesystem_get_file_size_this_file_bigger.cpp")
add_core_sync_test(SyncTestCore_FilesystemGetFileSizeNotAFile "core/filesystem/filesystem_get_file_size_not_a_file.cpp")
add_core_sync_test(SyncTestCore_FilesystemRelativeToAbsoluteDir "core/filesystem/filesystem_relative_to_absolute_dir.cpp")

# ==================
# Main Library Tests
# ==================
set(SYNC_LIB_TESTS_SOURCES
    "../lib/src/core/core.c"
    "../lib/src/core/builtin_functions/sort.cpp"
    "../lib/src/core/builtin_traits/iterator.cpp"
    "../lib/src/util/os_callstack.cpp"
    "../lib/src/util/simd.cpp"
    "../lib/src/mem/allocator.cpp"
    "../lib/src/mem/os_mem.cpp"
    "../lib/src/mem/protected_allocator.cpp"
    "../lib/src/threading/sync_queue.cpp"
    "../lib/src/threading/sync_obj_val.cpp"
    "../lib/src/types/type_info.cpp"
    "../lib/src/types/function/function.cpp"
    "../lib/src/types/string/string_slice.cpp"
    "../lib/src/types/string/string.cpp"
    "../lib/src/types/sync_obj/sync_obj.cpp"
    "../lib/src/types/array/dynamic_array.cpp"
    "../lib/src/types/array/slice.cpp"
    "../lib/src/types/hash/groups.cpp"
    "../lib/src/types/hash/map.cpp"
    "../lib/src/types/option/option.cpp"
    "../lib/src/types/result/result.cpp"
    "../lib/src/types/task/task.cpp"
    "../lib/src/types/box/box.cpp"
    "../lib/src/interpreter/stack/frame.cpp"
    "../lib/src/interpreter/stack/node.cpp"
    "../lib/src/interpreter/stack/stack.cpp"
    "../lib/src/interpreter/bytecode.cpp"
    "../lib/src/interpreter/interpreter.cpp"
    "../lib/src/interpreter/function_builder.cpp"
    "../lib/src/compiler/compiler.cpp"
    "../lib/src/compiler/tokenizer/token.cpp"
    "../lib/src/compiler/tokenizer/tokenizer.cpp"
    "../lib/src/compiler/tokenizer/file_literals.cpp"
    "../lib/src/compiler/source_tree/source_tree.cpp"
    "../lib/src/compiler/parser/parser.cpp"
    "../lib/src/compiler/parser/base_nodes.cpp"
    "../lib/src/compiler/parser/stack_variables.cpp"
    "../lib/src/compiler/parser/type_resolution.cpp"
    "../lib/src/compiler/parser/expression.cpp"
    "../lib/src/compiler/graph/scope.cpp"
    "../lib/src/compiler/graph/module_dependency_graph.cpp"
    "../lib/src/compiler/parser/ast/function_definition.cpp"
    "../lib/src/compiler/parser/ast/return.cpp"
    "../lib/src/compiler/parser/ast.cpp"
    "../lib/src/program/program.cpp"
    "../lib/src/program/program_error.cpp"
    "../lib/src/testing/child_process.cpp"
    "../lib/src/testing/assert_handler.cpp"

    "../lib/test/test_runner.cpp"
    "../lib/test/test.cpp"
)

#set(SYNC_LIB_TESTS_SOURCES ${SYNC_LIB_SOURCES} "../lib/test/test.cpp")

add_executable(SyncLibTests ${SYNC_LIB_TESTS_SOURCES})
target_compile_definitions(SyncLibTests PRIVATE SYNC_LIB_WITH_TESTS=1)
add_sync_test_flags(SyncLibTests ${SYNC_LIB_ASAN} TRUE FALSE)
if((NOT MSVC) AND (NOT EMSCRIPTEN) AND (NOT SYNC_SKIP_TSAN_TESTS))
    add_executable(SyncLibTests_ThreadSanitizer ${SYNC_LIB_TESTS_SOURCES})
    target_compile_definitions(SyncLibTests_ThreadSanitizer PRIVATE SYNC_LIB_WITH_TESTS=1)
    add_sync_test_flags(SyncLibTests_ThreadSanitizer TRUE TRUE FALSE)
endif()

if(SYNC_LIB_CODE_COVERAGE AND MSVC)
    cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH SYNC_ROOT_DIR)
    file(TO_NATIVE_PATH "${SYNC_ROOT_DIR}" SYNC_WINDOWS_NATIVE_ROOT_DIR)
    add_test(NAME SyncLibTests 
        COMMAND OpenCppCoverage.exe 
        --sources ${SYNC_WINDOWS_NATIVE_ROOT_DIR}\\lib\\src\\*
        --export_type binary:SyncLibTests.cov
        -- ${CMAKE_CURRENT_BINARY_DIR}\\Debug\\SyncLibTests)
else()
    add_test(NAME SyncLibTests COMMAND SyncLibTests)
    if((NOT MSVC) AND (NOT EMSCRIPTEN) AND (NOT SYNC_SKIP_TSAN_TESTS))
        add_test(NAME SyncLibTests_ThreadSanitizer COMMAND SyncLibTests_ThreadSanitizer)
    endif()
endif()
