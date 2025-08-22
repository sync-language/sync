cmake_minimum_required (VERSION 3.19)

if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    #set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")

    # https://cmake.org/cmake/help/latest/prop_tgt/MSVC_DEBUG_INFORMATION_FORMAT.html#prop_tgt:MSVC_DEBUG_INFORMATION_FORMAT
    # set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SyncLibSources 
    "src/util/panic.cpp"
    "src/util/os_callstack.cpp"
    "src/util/simd.cpp"
    "src/mem/allocator.cpp"
    "src/mem/os_mem.cpp"
    "src/threading/sync_queue.cpp"
    "src/threading/sync_obj_val.cpp"
    "src/types/type_info.cpp"
    "src/types/function/function.cpp"
    "src/types/string/string_slice.cpp"
    "src/types/string/string.cpp"
    "src/types/sync_obj/sync_obj.cpp"
    "src/types/array/dynamic_array.cpp"
    "src/interpreter/stack/frame.cpp"
    "src/interpreter/stack/node.cpp"
    "src/interpreter/stack/stack.cpp"
    "src/interpreter/bytecode.cpp"
    "src/interpreter/interpreter.cpp"
    "src/compiler/compile_info.cpp"
    "src/compiler/tokenizer/token.cpp"
    "src/compiler/tokenizer/tokenizer.cpp"
    "src/compiler/tokenizer/file_literals.cpp"
    "src/program/program.cpp"
    "src/testing/doctest.cpp"
)

add_library(SyncLib STATIC ${SyncLibSources})

add_executable(SyncLibTests "src/test.cpp" ${SyncLibSources})

# Ensure all warnings are caught
if(MSVC)
    target_compile_options(SyncLib PRIVATE /W4 /WX)
    target_compile_options(SyncLibTests PRIVATE /W4 /WX)
    target_link_libraries(SyncLib PRIVATE dbghelp)
    target_link_libraries(SyncLibTests PRIVATE dbghelp)
    # C++ exceptions and unwind semantics. Necessary for Rust
    target_compile_options(SyncLib PRIVATE /EHsc /Zi) 
    target_compile_options(SyncLibTests PRIVATE /EHsc /Zi) 
    target_compile_options(SyncLib PRIVATE /fsanitize=address)
    target_compile_options(SyncLibTests PRIVATE /fsanitize=address)
else()
    target_compile_options(SyncLib PRIVATE -Wall -Wextra -Wpedantic -Werror -fsanitize=address)
    target_compile_options(SyncLibTests PRIVATE -Wall -Wextra -Wpedantic -Werror -fsanitize=address)
    set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(SyncLib PRIVATE SYNC_LIB_NO_TESTS)
endif()

# SIMD
include(CheckCXXCompilerFlag)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    set (X86 TRUE)
else ()
    set (X86 FALSE)
endif ()

if(X86)
    # SSE2
    cmake_host_system_information(RESULT HAS_SSE2 QUERY HAS_SSE2)
    if(HAS_SSE2)
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            message(STATUS "Sync SSE2 GNU/Clang")
            set(CMAKE_CXX_FLAGS "-msse2")
            target_compile_options(SyncLib PRIVATE -msse2)
            target_compile_options(SyncLibTests PRIVATE -msse2)
        elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
            message(STATUS "Sync SSE2 MSVC")
            set(CMAKE_CXX_FLAGS "/arch:SSE2")
            target_compile_options(SyncLib PRIVATE /arch:SSE2)
            target_compile_options(SyncLibTests PRIVATE /arch:SSE2)
            # For some reason __SSE2__ macro doesn't seem to work? Not sure why?
        endif()
    endif()

    # AVX2
    if(MSVC)
        message(STATUS "Sync AVX2 MSVC")
        set(CMAKE_CXX_FLAGS "/arch:AVX2")
        target_compile_options(SyncLib PRIVATE /arch:AVX2)
        target_compile_options(SyncLibTests PRIVATE /arch:AVX2)
    else()
        message(STATUS "Sync AVX2 GNU/Clang")
        set(CMAKE_CXX_FLAGS "-mavx2")
        target_compile_options(SyncLib PRIVATE -mavx2)
        target_compile_options(SyncLibTests PRIVATE -mavx2)
    endif()
endif()




set_target_properties(SyncLib PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

install(TARGETS SyncLib DESTINATION .)