# Sets SIMD flags for targets / files through checking the what SIMD instructions the build machine # supports.
#
# Defines the following variables globally:
# - _CPU_HAS_X86_64_AVX2
# - _CPU_HAS_X86_64_AVX512
# - _CPU_HAS_RISCV64_RVV_SUPPORTED
#
# If cross compiling, set any the above variables to `TRUE` to enable them.

include(CheckCXXSourceRuns) # Stupid GitHub Actions (I love you github actions)

if(CMAKE_CROSSCOMPILING)
    if(NOT DEFINED _CPU_HAS_X86_64_AVX2)
        set(_CPU_HAS_X86_64_AVX2 OFF)
    endif()
    if(NOT DEFINED _CPU_HAS_X86_64_AVX512)
        set(_CPU_HAS_X86_64_AVX512 OFF)
    endif()
    if(NOT DEFINED _CPU_HAS_RISCV64_RVV_SUPPORTED)
        set(_CPU_HAS_RISCV64_RVV_SUPPORTED OFF)
    endif()
else() # CMAKE_CROSSCOMPILING

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(X86_64)|(amd64)|(AMD64)")
        if(NOT DEFINED _CPU_HAS_X86_64_AVX2)
            set(CMAKE_REQUIRED_FLAGS_PRE_AVX2 ${CMAKE_REQUIRED_FLAGS})
            if(MSVC)
                set(CMAKE_REQUIRED_FLAGS "/arch:AVX2")
            else()
                set(CMAKE_REQUIRED_FLAGS "-mavx2")
            endif()
            check_cxx_source_runs("
                #include <immintrin.h>
                int main() { __m256i thing = _mm256_setzero_si256(); return 0; }
            " _CPU_HAS_X86_64_AVX2)
            set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_PRE_AVX2})
            
            if(_CPU_HAS_X86_64_AVX2)
                message(STATUS "x86_64 AVX2 support detected")
            endif()
        endif()

        if(NOT DEFINED _CPU_HAS_X86_64_AVX512)
            set(CMAKE_REQUIRED_FLAGS_PRE_AVX512 ${CMAKE_REQUIRED_FLAGS})
            if(MSVC)
                set(CMAKE_REQUIRED_FLAGS "/arch:AVX512")
            else()
                set(CMAKE_REQUIRED_FLAGS "-mavx512f -mavx512bw")
            endif()
            check_cxx_source_runs("
                #include <immintrin.h>
                int main() { __m512i thing = _mm512_setzero_si512(); return 0; }
            " _CPU_HAS_X86_64_AVX512)
            set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_PRE_AVX512})

            if(_CPU_HAS_X86_64_AVX512)
                message(STATUS "x86_64 AVX512 support detected")
            endif()
        endif()
        
    endif() # x86_64 host

    # For ARM64, NEON is required
    # https://chromium.googlesource.com/libyuv/libyuv/+/HEAD/docs/feature_detection.md#:~:text=Neon%20extensions.%20Neon%20is%20available%20and%20mandatory,even%20if%20later%20extensions%20like%20the%20Scalable

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(riscv64)|(RISCV64)")
        if(NOT DEFINED _CPU_HAS_RISCV64_RVV_SUPPORTED)
            # https://gcc.gnu.org/onlinedocs/gcc/RISC-V-Options.html
            set(CMAKE_REQUIRED_FLAGS_PRE_RVV ${CMAKE_REQUIRED_FLAGS})
            set(CMAKE_REQUIRED_FLAGS "-march=rv64gcv -mabi=lp64d")
            check_cxx_source_runs("
                #include <riscv_vector.h>
                int main() {
                    __attribute__((unused)) vfloat32m1_t v;
                    return 0;
                }
            " _CPU_HAS_RISCV64_RVV_SUPPORTED)
            set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_PRE_RVV})
            
            if(_CPU_HAS_RISCV64_RVV_SUPPORTED)
                message(STATUS "riscv64 RVV support detected")
            endif()
        endif()

    endif() # riscv64 host

endif() # CMAKE_CROSSCOMPILING


# Sets the simd flags for a specific target (executable or library)
#
# @param TARGET The executable or library to set the simd flags for
# @param _FORCE_DISABLE_AVX2 Disable x86_64 avx2 even if the cpu target supports it
# @param _FORCE_DISABLE_AVX512 Disable x86_64 aavx512 even if the cpu target supports it
# @param _FORCE_DISABLE_WASM_128 Disable wasm simd even if the cpu target supports it
# @param _FORCE_DISABLE_RVV Disable riscv64 rvv even if the cpu target supports it
function(add_simd_flags_to_target TARGET _FORCE_DISABLE_AVX2 _FORCE_DISABLE_AVX512 _FORCE_DISABLE_WASM_128 _FORCE_DISABLE_RVV)
    if(EMSCRIPTEN)
        if(NOT ${_FORCE_DISABLE_WASM_128})
            target_compile_options(${TARGET} PRIVATE -msimd128)
        endif()
        return()
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(X86_64)|(amd64)|(AMD64)")
        if(NOT ${_FORCE_DISABLE_AVX2})
            if(_CPU_HAS_X86_64_AVX2)
                if(MSVC)
                    target_compile_options(${TARGET} PRIVATE /arch:AVX2)
                else()
                    target_compile_options(${TARGET} PRIVATE -mavx2)
                endif()
                message(STATUS "AVX2 enabling for target ${TARGET}")
            else()
                message(STATUS "AVX2 not supported by this system for target ${TARGET}")
            endif()
        endif()

        if(NOT ${_FORCE_DISABLE_AVX512})
            if(_CPU_HAS_X86_64_AVX512)
                if(MSVC)
                    target_compile_options(${TARGET} PRIVATE /arch:AVX512)
                else()
                    target_compile_options(${TARGET} PRIVATE -mavx512f -mavx512bw)
                endif()
                message(STATUS "AVX512 enabling for target ${TARGET}")
            else()
                message(STATUS "AVX512 not supported by this system for target ${TARGET}")
            endif()
        endif()
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(riscv64)|(RISCV64)")
        if(NOT ${_FORCE_DISABLE_RVV})
            if(_CPU_HAS_RISCV64_RVV_SUPPORTED)
                target_compile_options(${TARGET} PRIVATE -march=rv64gcv -mabi=lp64d)
                message(STATUS "RVV enabling for target ${TARGET}")
            else()
                message(STATUS "RVV not supported by this system for target ${TARGET}")
            endif()
        endif()
    endif()
endfunction()

# Sets the simd flags for a specific source code file
#
# @param SOURCE_FILE The source file to set the simd flags for
# @param _FORCE_DISABLE_AVX2 Disable x86_64 avx2 even if the cpu target supports it
# @param _FORCE_DISABLE_AVX512 Disable x86_64 aavx512 even if the cpu target supports it
# @param _FORCE_DISABLE_WASM_128 Disable wasm simd even if the cpu target supports it
# @param _FORCE_DISABLE_RVV Disable riscv64 rvv even if the cpu target supports it
function(add_simd_flags_to_file SOURCE_FILE _FORCE_DISABLE_AVX2 _FORCE_DISABLE_AVX512 _FORCE_DISABLE_WASM_128 _FORCE_DISABLE_RVV)
    get_source_file_property(_existing_flags ${SOURCE_FILE} COMPILE_FLAGS)

    if(EMSCRIPTEN)
        if(NOT ${_FORCE_DISABLE_WASM_128})
            set_source_files_properties(${SOURCE_FILE} PROPERTIES COMPILE_FLAGS "${_existing_flags} -msimd128")
        endif()
        return()
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(X86_64)|(amd64)|(AMD64)")
        if(NOT ${_FORCE_DISABLE_AVX2})
            if(_CPU_HAS_X86_64_AVX2)
                if(MSVC)
                    set_source_files_properties(${SOURCE_FILE} PROPERTIES COMPILE_FLAGS "${_existing_flags} /arch:AVX2")
                else()
                    set_source_files_properties(${SOURCE_FILE} PROPERTIES COMPILE_FLAGS "${_existing_flags} -mavx2")
                endif()
                message(STATUS "AVX2 enabling for file ${SOURCE_FILE}")
            else()
                message(STATUS "AVX2 not supported by this system for file ${SOURCE_FILE}")
            endif()
        endif()

        if(NOT ${_FORCE_DISABLE_AVX512})
            if(_CPU_HAS_X86_64_AVX512)
                if(MSVC)
                    set_source_files_properties(${SOURCE_FILE} PROPERTIES COMPILE_FLAGS "${_existing_flags} /arch:AVX512")
                else()
                    set_source_files_properties(${SOURCE_FILE} PROPERTIES COMPILE_FLAGS "${_existing_flags} -mavx512f -mavx512bw")
                endif()
                message(STATUS "AVX512 enabling for file ${SOURCE_FILE}")
            else()
                message(STATUS "AVX512 not supported by this system for file ${SOURCE_FILE}")
            endif()
        endif()
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(riscv64)|(RISCV64)")
        if(NOT ${_FORCE_DISABLE_RVV})
            if(_CPU_HAS_RISCV64_RVV_SUPPORTED)
                set_source_files_properties(${SOURCE_FILE} PROPERTIES COMPILE_FLAGS "${_existing_flags} -march=rv64gcv -mabi=lp64d")
                message(STATUS "RVV enabling for file ${SOURCE_FILE}")
            else()
                message(STATUS "RVV not supported by this system for file ${SOURCE_FILE}")
            endif()
        endif()
    endif()
endfunction()
