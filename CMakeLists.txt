cmake_minimum_required (VERSION 3.28)

if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    #set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")

    # https://cmake.org/cmake/help/latest/prop_tgt/MSVC_DEBUG_INFORMATION_FORMAT.html#prop_tgt:MSVC_DEBUG_INFORMATION_FORMAT
    # set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if(WIN32)
    include(cmake/find_windows_sdk.cmake)
endif()

project("Sync")

option(BUILD_SHARED_LIBS "Build Sync as a shared library" OFF)
option(SYNC_LIB_WITH_TESTS "Compile doctest tests as well" OFF)
option(SYNC_SKIP_TSAN_TESTS "Disable TSan tests even if the compiler supports it, such as in containerized environments" OFF)
option(SYNC_DISABLE_ASAN_TESTS "Disable ASan in tests even if the compiler supports it, such as in containerized environments" OFF)
option(SYNC_LIB_DOCTEST_ADD_CTESTS "Add all test cases to CTest individually" ON)
option(SYNC_LIB_CODE_COVERAGE "Enable code coverage" OFF)
option(SYNC_LIB_ASAN "Use compiler specific address sanitizing" OFF)
option(SYNC_LIB_TSAN "Use compiler specific thread sanitizing" OFF)
option(SYNC_NO_PAGES "Do not support direct virtual memory function access" OFF)
option(SYNC_NO_AVX2 "Explicitly disable AVX2 usage" ON)
option(SYNC_NO_AVX512 "Explicitly disable AVX512 usage" ON)
option(SYNC_WASM_USE_SIMD "Use Fixed-width SIMD on WASM" OFF)
option(SYNC_NO_RVV "Explicitly disable riscv64 RVV" OFF)
option(SYNC_NO_FILESYSTEM "Disable file system access" OFF)
option(SYNC_NO_SAFETY_CHECKS "Disable all safety checks" OFF)

# Overriding core functionality. See docs/core_requirements.md
option(SYNC_CUSTOM_ALIGNED_MALLOC_FREE "Provide and link your own custom implementation of basic memory allocation operations" OFF)
option(SYNC_CUSTOM_PAGE_MEMORY "Provide and link your own custom implementation of virtual page memory operations" OFF)
option(SYNC_DEFAULT_PAGE_ALIGNMENT "Provide a custom default page size / alignment" OFF)
option(SYNC_CUSTOM_DEFAULT_FATAL_ERROR_HANDLER "Provide and link your default fatal error handler for special requirements" OFF)
option(SYNC_CUSTOM_DEFAULT_WRITE_STRING_ERROR "Provide and link your default string error writer for special requirements" OFF)
option(SYNC_CUSTOM_BACKTRACE "Provide and link your own custom implementation of printing backtraces" OFF)

set(SYNC_LIB_SOURCES
    "lib/src/core/core.c"
    "lib/src/core/builtin_functions/sort.cpp"
    "lib/src/core/builtin_traits/iterator.cpp"
    "lib/src/util/simd.cpp"
    "lib/src/mem/allocator.cpp"
    "lib/src/mem/os_mem.cpp"
    "lib/src/mem/protected_allocator.cpp"
    "lib/src/threading/sync_queue.cpp"
    "lib/src/threading/sync_obj_val.cpp"
    "lib/src/threading/locks/locks_internal.cpp"
    "lib/src/threading/locks/rwlock.cpp"
    "lib/src/threading/generation/gen_pool.cpp"
    "lib/src/threading/generation/gen_pool_internal.cpp"
    "lib/src/types/type_info.cpp"
    "lib/src/types/function/function.cpp"
    "lib/src/types/string/string_internal.cpp"
    "lib/src/types/string/string_slice.cpp"
    "lib/src/types/string/string.cpp"
    "lib/src/types/sync_obj/sync_obj.cpp"
    "lib/src/types/array/dynamic_array.cpp"
    "lib/src/types/array/slice.cpp"
    "lib/src/types/hash/groups.cpp"
    "lib/src/types/hash/map.cpp"
    "lib/src/types/option/option.cpp"
    "lib/src/types/result/result.cpp"
    "lib/src/types/task/task.cpp"
    "lib/src/types/box/box.cpp"
    "lib/src/types/anyerror/anyerror.cpp"
    "lib/src/interpreter/stack/frame.cpp"
    "lib/src/interpreter/stack/node.cpp"
    "lib/src/interpreter/stack/stack.cpp"
    "lib/src/interpreter/bytecode.cpp"
    "lib/src/interpreter/interpreter.cpp"
    "lib/src/interpreter/function_builder.cpp"
    "lib/src/compiler/compiler.cpp"
    "lib/src/compiler/tokenizer/token.cpp"
    "lib/src/compiler/tokenizer/tokenizer.cpp"
    "lib/src/compiler/tokenizer/file_literals.cpp"
    "lib/src/compiler/source_tree/source_tree.cpp"
    "lib/src/compiler/parser/parser.cpp"
    "lib/src/compiler/parser/base_nodes.cpp"
    "lib/src/compiler/parser/stack_variables.cpp"
    "lib/src/compiler/parser/type_resolution.cpp"
    "lib/src/compiler/parser/expression.cpp"
    "lib/src/compiler/graph/scope.cpp"
    "lib/src/compiler/graph/module_dependency_graph.cpp"
    "lib/src/compiler/parser/ast/function_definition.cpp"
    "lib/src/compiler/parser/ast/return.cpp"
    "lib/src/compiler/parser/ast.cpp"
    "lib/src/program/program.cpp"
    "lib/src/program/program_error.cpp"
    "lib/src/testing/assert_handler.cpp"
)

if(SYNC_LIB_WITH_TESTS)
    set(SYNC_LIB_SOURCES ${SYNC_LIB_SOURCES} "lib/test/test_runner.cpp")
endif()

if(BUILD_SHARED_LIBS)
    add_library(sync SHARED ${SYNC_LIB_SOURCES})
else()
    add_library(sync STATIC ${SYNC_LIB_SOURCES})
endif()
target_include_directories(sync INTERFACE "include")
add_library(sync::sync ALIAS sync)

set_target_properties(sync PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
)

if(EMSCRIPTEN)
    set_target_properties(sync PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED YES
    )
else()
    set_target_properties(sync PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
    )
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# See docs/core_requirements.md
if(SYNC_CUSTOM_ALIGNED_MALLOC_FREE)
    target_compile_definitions(sync PRIVATE SYNC_CUSTOM_ALIGNED_MALLOC_FREE=1)
endif()
if(SYNC_CUSTOM_PAGE_MEMORY)
    target_compile_definitions(sync PRIVATE SYNC_CUSTOM_PAGE_MEMORY=1)
endif()
if(SYNC_DEFAULT_PAGE_ALIGNMENT)
    target_compile_definitions(sync PRIVATE SYNC_DEFAULT_PAGE_ALIGNMENT=1)
endif()
if(SYNC_CUSTOM_DEFAULT_FATAL_ERROR_HANDLER)
    target_compile_definitions(sync PRIVATE SYNC_CUSTOM_DEFAULT_FATAL_ERROR_HANDLER=1)
endif()
if(SYNC_NO_FILESYSTEM)
    target_compile_definitions(sync PRIVATE SYNC_NO_FILESYSTEM=1)
endif()
if(SYNC_NO_SAFETY_CHECKS)
    target_compile_definitions(sync PRIVATE SYNC_NO_SAFETY_CHECKS=1)
endif()
if(SYNC_CUSTOM_BACKTRACE)
    target_compile_definitions(sync PRIVATE SYNC_CUSTOM_BACKTRACE=1)
endif()
if(SYNC_LIB_WITH_TESTS)
    target_compile_definitions(sync PRIVATE SYNC_LIB_WITH_TESTS)
endif()
if(SYNC_NO_PAGES)
    target_compile_definitions(sync PRIVATE SYNC_NO_PAGES)
endif()

if(MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(sync PRIVATE /MP)
endif()

# C11 Atomic Support. NOT REQUIRED, can build using the Interlocked API
if(MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(sync PRIVATE /experimental:c11atomics)
endif()

if(WIN32)
    target_link_libraries(sync PRIVATE "$<$<CONFIG:Debug>:dbghelp>")
endif()

include(cmake/base_compiler_flags.cmake)
set_base_compiler_flags(sync)
if(EMSCRIPTEN)
    target_compile_options(sync PRIVATE -Wno-c2y-extensions)
endif()

include(cmake/sanitizers_coverage.cmake)
if(SYNC_LIB_ASAN)
    enable_asan(sync)
elseif(SYNC_LIB_TSAN)
    enable_tsan(sync)
endif()
if(SYNC_LIB_CODE_COVERAGE)
    enable_coverage(sync)
    target_compile_definitions(sync PRIVATE SYNC_LIB_CODE_COVERAGE=1)
endif()

include(cmake/simd_target.cmake)
add_simd_flags_to_target(sync SYNC_NO_AVX2 SYNC_NO_AVX512 SYNC_WASM_USE_SIMD SYNC_NO_RVV)

if(EMSCRIPTEN)
    message(STATUS "Enabling pthread support for Emscripten")
    target_compile_options(sync PRIVATE -pthread)
    target_link_options(sync PRIVATE -pthread)
    # Sure
    target_link_options(sync PRIVATE "SHELL:-s PTHREAD_POOL_SIZE=8")
endif()

if(MSVC)
    target_compile_definitions(sync PRIVATE SYNC_EXPORT_DLL)
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(sync INTERFACE SYNC_IMPORT_DLL)
    endif()
endif()
set_target_properties(sync PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

# ===========
# Debug Utils
# ===========

# Conditionally enable backtrace support.
# Backtraces are not required as a core component of sync itself, so having
# them be configured as a build step, NOT as part of the C++ sources themselves
# is acceptable.
# CMake XBOX stuff publicly available. No NDA violation!!!!
# https://github.com/microsoft/Xbox-GDK-Samples/blob/73f8aa373138ecd6331a05ac908a88c287602db8/Samples/Tools/CMakeGDKExample/CMakeLists.txt#L152
if(NOT EMSCRIPTEN AND NOT (DEFINED XBOX_CONSOLE_TARGET))
    if(CMAKE_BUILD_TYPE EQUAL "Debug" OR CMAKE_BUILD_TYPE EQUAL "RelWithDebInfo")
        target_compile_definitions(sync PRIVATE SYNC_BACKTRACE_SUPPORTED)
    endif()
endif()

if(SYNC_LIB_CODE_COVERAGE)
    target_compile_definitions(sync PRIVATE SYNC_LIB_CODE_COVERAGE=1)
    message(STATUS "Code coverage enabled")
    if(MSVC)
        # OpenCppCoverage?
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # llvm-cov
        target_compile_options(sync PRIVATE -fprofile-instr-generate -fcoverage-mapping)
        target_link_options(sync PRIVATE -fprofile-instr-generate)
    else(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # gcov
        target_compile_options(sync PRIVATE --coverage)
        target_link_options(sync PRIVATE --coverage)
    endif()
endif()

install(TARGETS sync DESTINATION .)

if(SYNC_LIB_WITH_TESTS)
    target_compile_definitions(sync PRIVATE SYNC_LIB_WITH_TESTS=1)
    add_subdirectory(test)
endif()

enable_testing()
