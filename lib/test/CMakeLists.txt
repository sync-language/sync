cmake_minimum_required (VERSION 3.28)

project("CIExample")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(Example example.cpp)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

target_compile_options(Example PRIVATE -Wall -Wextra -Wpedantic -Werror -fsanitize=address)
set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")

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
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
            target_compile_options(Example PRIVATE -msse2)
        elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
            message(STATUS "Sync SSE2 MSVC")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
            target_compile_options(Example PRIVATE /arch:SSE2)
            # For some reason __SSE2__ macro doesn't seem to work? Not sure why?
        endif()
    endif()

    # AVX2
    if(MSVC)
        message(STATUS "Sync AVX2 MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
        target_compile_options(Example PRIVATE /arch:AVX2)
    else()
        message(STATUS "Sync AVX2 GNU/Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
        target_compile_options(Example PRIVATE -mavx2)
    endif()

    # AVX512
    if(MSVC)
        message(STATUS "Sync AVX512 MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX512")
        target_compile_options(Example PRIVATE /arch:AVX512)
    else()
        message(STATUS "Sync AVX512 GNU/Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512f")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512bw")
        target_compile_options(Example PRIVATE -mavx512f)
        target_compile_options(Example PRIVATE -mavx512f)
    endif()
endif()

set_target_properties(Example PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

install(TARGETS Example DESTINATION .)

enable_testing()
add_test(NAME Example COMMAND Example)

