cmake_minimum_required (VERSION 3.28)

if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    #set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")

    # https://cmake.org/cmake/help/latest/prop_tgt/MSVC_DEBUG_INFORMATION_FORMAT.html#prop_tgt:MSVC_DEBUG_INFORMATION_FORMAT
    # set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("Sync")

option(SYNC_LIB_WITH_TESTS "Compile doctest tests as well" OFF)
option(SYNC_LIB_DOCTEST_ADD_CTESTS "Add all test cases to CTest individually" ON)
option(SYNC_LIB_ASAN "Use compiler specific address sanitizing" ON)
option(SYNC_LIB_BUILD_SHARED_LIB "Build Sync as a shared library" OFF)
option(SYNC_NO_PAGES "Do not support direct virtual memory function access" OFF)
option(SYNC_NO_AVX2 "Explicitly disable AVX2 usage" OFF)
option(SYNC_NO_AVX512 "Explicitly disable AVX512 usage" OFF)
option(SYNC_WASM_USE_SIMD "Use Fixed-width SIMD on WASM" OFF)

set(CMAKE_CXX_STANDARD 17)
if(EMSCRIPTEN)
    message(STATUS "Using C++23 for Emscripten")
    set(CMAKE_CXX_STANDARD 23) # __COUNTER__ for doctest
endif()
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(EMSCRIPTEN)
    message(STATUS "Building Sync for Emscripten")
    # find_path(EMSCRIPTEN_DIR NAMES "em++")
    # message(${EMSCRIPTEN_DIR})
    # if(EMSCRIPTEN_DIR)
        
    # endif()
    # message(STATUS "Emscripten Root Directory: ${EMSCRIPTEN_ROOT}")
    # message(STATUS "guh")
endif()

set(SyncLibSources 
    "lib/src/util/panic.cpp"
    "lib/src/util/os_callstack.cpp"
    "lib/src/util/simd.cpp"
    "lib/src/mem/allocator.cpp"
    "lib/src/mem/os_mem.cpp"
    "lib/src/mem/protected_allocator.cpp"
    "lib/src/threading/sync_queue.cpp"
    "lib/src/threading/sync_obj_val.cpp"
    "lib/src/types/type_info.cpp"
    "lib/src/types/function/function.cpp"
    "lib/src/types/string/string_slice.cpp"
    "lib/src/types/string/string.cpp"
    "lib/src/types/sync_obj/sync_obj.cpp"
    "lib/src/types/array/dynamic_array.cpp"
    "lib/src/types/array/slice.cpp"
    "lib/src/types/hash/groups.cpp"
    "lib/src/types/hash/map.cpp"
    "lib/src/types/option/option.cpp"
    "lib/src/types/result/result.cpp"
    "lib/src/types/box/box.cpp"
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
    "lib/src/program/program.cpp"
    "lib/src/program/program_error.cpp"
    "lib/src/testing/child_process.cpp"
    "lib/src/testing/assert_handler.cpp"

    "lib/test/test_runner.cpp"
)

if(SYNC_LIB_BUILD_SHARED_LIB)
    add_library(SyncLib SHARED ${SyncLibSources})
else()
    add_library(SyncLib STATIC ${SyncLibSources})
endif()
add_executable(SyncLibTests "lib/test/test.cpp" ${SyncLibSources})

# Doctest Shenanigans
target_compile_definitions(SyncLibTests PRIVATE SYNC_LIB_WITH_TESTS)

if(SYNC_LIB_WITH_TESTS)
    target_compile_definitions(SyncLib PRIVATE SYNC_LIB_WITH_TESTS)
endif()

if(SYNC_NO_PAGES)
    target_compile_definitions(SyncLib PRIVATE SYNC_NO_PAGES)
    target_compile_definitions(SyncLibTests PRIVATE SYNC_NO_PAGES)
endif()

# Ensure all warnings are caught
if(EMSCRIPTEN)
    if (CMAKE_GENERATOR MATCHES "Visual Studio")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++23 /W4 /WX /EHsc /Zi")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23 -Wall -Wextra -Wpedantic -Werror -Wno-c2y-extensions")
        # TODO -Wno-c2y-extensions not this. __COUNTER__ is not liked by emsdk right now.
    endif()
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /EHsc /Zi")
    target_compile_options(SyncLib PRIVATE /W4 /WX)
    target_compile_options(SyncLibTests PRIVATE /W4 /WX)
    target_link_libraries(SyncLib PRIVATE dbghelp)
    target_link_libraries(SyncLibTests PRIVATE dbghelp)
    # C++ exceptions and unwind semantics. Necessary for Rust
    target_compile_options(SyncLib PRIVATE /EHsc /Zi) 
    target_compile_options(SyncLibTests PRIVATE /EHsc /Zi)
else()
    target_compile_options(SyncLib PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(SyncLibTests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(SYNC_LIB_ASAN)
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
        target_compile_options(SyncLib PRIVATE /fsanitize=address)
        target_compile_options(SyncLibTests PRIVATE /fsanitize=address)
    else()
        target_compile_options(SyncLib PRIVATE -fsanitize=address)
        target_compile_options(SyncLibTests PRIVATE -fsanitize=address)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    endif()
endif()

if(ANDROID)
    message(STATUS "Building Sync for Android")
    target_link_libraries(
        SyncLib
        android
        log)
    target_link_libraries(
        SyncLibTests
        android
        log)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(STATUS "Building Sync for iOS")
    # set_target_properties(SyncLibTests PROPERTIES
    #     MACOSX_BUNDLE_GUI_IDENTIFIER org.synclang
    #     MACOSX_BUNDLE_BUNDLE_NAME SyncLibTests
    #     MACOSX_BUNDLE_BUNDLE_VERSION 0.0.0
    # )
    
    # set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    # find_package(XCTest REQUIRED)
    # xctest_add_bundle(SyncLibTestsIOS 
    #     SyncLib "lib/test/ios/run_tests.cpp" "lib/test/ios/run_ios_tests.m" 
    # )
    # xctest_add_test(XCTest.SyncLibTestsIOS SyncLibTestsIOS)
endif()

find_package(JNI)
if(JNI_FOUND)
    message("Building SyncNative with JNI")
    
    add_library(SyncNative SHARED "lib/src/sync_native.cpp")
    target_link_libraries(SyncNative SyncLib)
    target_include_directories(SyncNative PRIVATE ${JNI_INCLUDE_DIRS})
    target_link_libraries(SyncNative ${JNI_LIBRARIES})
    if(SYNC_LIB_WITH_TESTS)
        target_compile_definitions(SyncNative PRIVATE SYNC_LIB_WITH_TESTS)
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(SyncLib PRIVATE SYNC_LIB_NO_TESTS)
endif()

# ========================
# X86_64 SIMD instructions
# ========================
include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns) # Stupid GitHub Actions (I love you github actions)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(X86_64)|(amd64)|(AMD64)")
    set (SYNC_X86_64 ON)
else()
    set (SYNC_X86_64 OFF)
endif()

if(SYNC_X86_64) # TODO evaluate using -msimd128
    if(EMSCRIPTEN AND NOT SYNC_WASM_USE_SIMD)
        set(SYNC_X86_64_SIMD OFF)
    else()
        set(SYNC_X86_64_SIMD ON)
    endif()

    if(SYNC_X86_64_SIMD)
        # SSE2
        cmake_host_system_information(RESULT HAS_SSE2 QUERY HAS_SSE2)
        if(HAS_SSE2)
            if(MSVC)
                message(STATUS "SSE2 supported by this system - enabling")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
                target_compile_options(SyncLib PRIVATE /arch:SSE2)
                target_compile_options(SyncLibTests PRIVATE /arch:SSE2)
            else()
                message(STATUS "SSE2 supported by this system - enabling")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
                target_compile_options(SyncLib PRIVATE -msse2)
                target_compile_options(SyncLibTests PRIVATE -msse2)   
            endif()
        else()
            message(STATUS "SSE2 not supported by this system")
        endif()

        # AVX2
        if(NOT ${SYNC_NO_AVX2})
            set(CMAKE_REQUIRED_FLAGS_PRE_AVX2 ${CMAKE_REQUIRED_FLAGS})
            if(MSVC)
                set(CMAKE_REQUIRED_FLAGS "/arch:AVX2")
            else()
                set(CMAKE_REQUIRED_FLAGS "-mavx2")
            endif()
            check_cxx_source_runs("
                #include <immintrin.h>
                int main() { __m256i thing = _mm256_setzero_si256(); return 0; }
            " HAS_AVX2)
            set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_PRE_AVX2})
            if(HAS_AVX2)
                if(MSVC)
                    message(STATUS "AVX2 supported on this system - enabling")
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
                    target_compile_options(SyncLib PRIVATE /arch:AVX2)
                    target_compile_options(SyncLibTests PRIVATE /arch:AVX2)
                else()
                    message(STATUS "AVX2 supported on this system - enabling")
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
                    target_compile_options(SyncLib PRIVATE -mavx2)
                    target_compile_options(SyncLibTests PRIVATE -mavx2)
                endif()
            else()
                message(STATUS "AVX2 not supported on this system")
            endif()
        endif()

        # AVX512
        if(NOT ${SYNC_NO_AVX512})
            set(CMAKE_REQUIRED_FLAGS_PRE_AVX512 ${CMAKE_REQUIRED_FLAGS})
            if(MSVC)
                set(CMAKE_REQUIRED_FLAGS "/arch:AVX512")
            else()
                set(CMAKE_REQUIRED_FLAGS "-mavx512f -mavx512bw")
            endif()
            check_cxx_source_runs("
                #include <immintrin.h>
                int main() { __m512i thing = _mm512_setzero_si512(); return 0; }
            " HAS_AVX512)
            set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_PRE_AVX512})
            if(HAS_AVX512)
                if(MSVC)
                    message(STATUS "AVX512 supported on this system - enabling")
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX512")
                    target_compile_options(SyncLib PRIVATE /arch:AVX512)
                    target_compile_options(SyncLibTests PRIVATE /arch:AVX512)
                else()
                    message(STATUS "AVX512 supported on this system - enabling")
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512f -mavx512bw")
                    target_compile_options(SyncLib PRIVATE -mavx512f -mavx512bw)
                    target_compile_options(SyncLibTests PRIVATE -mavx512f -mavx512bw)
                endif()
            else()
                message(STATUS "AVX512 not supported on this system")
            endif()
        endif()
    endif() # SYNC_X86_64_SIMD
endif()

if(EMSCRIPTEN AND SYNC_WASM_USE_SIMD)
    # https://webassembly.org/features/
    # Fixed-width SIMD supported by everything
    message(STATUS "Enabling Fixed-width SIMD for WASM")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msimd128")
    target_compile_options(SyncLib PRIVATE -msimd128)
    target_compile_options(SyncLibTests PRIVATE -msimd128)
endif()

if(MSVC)
    target_compile_definitions(SyncLib PRIVATE SYNC_EXPORT_DLL)
    target_compile_definitions(SyncLib INTERFACE SYNC_IMPORT_DLL)
endif()
set_target_properties(SyncLib PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

install(TARGETS SyncLib DESTINATION .)

enable_testing()
# if(SYNC_LIB_DOCTEST_ADD_CTESTS)
#     include(scripts/vendor/doctest/doctest.cmake)
#     doctest_discover_tests(SyncLibTests)
# else()
#     add_test(NAME SyncLibTests COMMAND SyncLibTests)
# endif()

# if(X86)
#     # SSE2
#     cmake_host_system_information(RESULT HAS_SSE2 QUERY HAS_SSE2)
#     if(HAS_SSE2)
#         if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
#             message(STATUS "Sync SSE2 GNU/Clang")
#             set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
#             target_compile_options(SyncLib PRIVATE -msse2)
#             target_compile_options(SyncLibTests PRIVATE -msse2)
#         elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
#             message(STATUS "Sync SSE2 MSVC")
#             set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
#             target_compile_options(SyncLib PRIVATE /arch:SSE2)
#             target_compile_options(SyncLibTests PRIVATE /arch:SSE2)
#             # For some reason __SSE2__ macro doesn't seem to work? Not sure why?
#         endif()
#     endif()

#     # AVX2
#     if(MSVC)
#         message(STATUS "Sync AVX2 MSVC")
#         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
#         target_compile_options(SyncLib PRIVATE /arch:AVX2)
#         target_compile_options(SyncLibTests PRIVATE /arch:AVX2)
#     else()
#         message(STATUS "Sync AVX2 GNU/Clang")
#         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
#         target_compile_options(SyncLib PRIVATE -mavx2)
#         target_compile_options(SyncLibTests PRIVATE -mavx2)
#     endif()

#     # AVX512
#     if(MSVC)
#         message(STATUS "Sync AVX512 MSVC")
#         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX512")
#         target_compile_options(SyncLib PRIVATE /arch:AVX512)
#         target_compile_options(SyncLibTests PRIVATE /arch:AVX512)
#     else()
#         message(STATUS "Sync AVX512 GNU/Clang")
#         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512f")
#         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512bw")
#         target_compile_options(SyncLib PRIVATE -mavx512f)
#         target_compile_options(SyncLib PRIVATE -mavx512bw)
#         target_compile_options(SyncLibTests PRIVATE -mavx512f)
#         target_compile_options(SyncLibTests PRIVATE -mavx512bw)
#     endif()
# endif()
