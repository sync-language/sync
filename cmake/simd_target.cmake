# Queries SIMD capabilities exposing per-feature variables.
# Tells the caller what the toolchain COULD emit so per-translation-unit files
# can opt-in.
#
# Covers:
# - SSE42 (x86_64 SSE4.2)
# - AVX2 (x86_64 AVX2)
# - AVX512 (x86_64 AVX512F and AVX512BW)
# - SVE (AArch64 Scalable Vector Extension)
# - RVV (RISC-V64 RVV 1.0)
# - WASM_SIMD (WASM 128-bit SIMD)
#
# Makes boolean (ON/OFF) features per variable being:
# - SYNC_CAN_EMIT_<X> if the toolchain is permitted to compile source code using <X>
#   instructions, gated for translation units. Probed via check_cxx_source_compiles —
#   does not require the build host CPU to actually run the instructions.
#
# - SYNC_BUILD_RUNS_<X> if the machine that is actually compiling the target can execute
#   <X> instructions. Mostly for informational purposes, or gating tests that would
#   otherwise SIGILL on the build host.
#
# - SYNC_BUILD_<X> is the one that controls whether this SIMD stuff should be in the binary.
#   Computed as: SYNC_CAN_EMIT_<X> AND NOT SYNC_NO_<X>. This is the single source of
#   truth consumed by the apply functions below and by the root CMakeLists.txt when
#   promoting to a source-side #define.
#
# The user-facing SYNC_NO_<X> options live in the root CMakeLists.txt and are not modified.
#
# Cross-compile override: pre-set SYNC_CAN_EMIT_<X>=TRUE before include()

include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)

# @param FEATURE one of `SSE42`, `AVX2`, `AVX512`, `SVE`, `RVV`, `WASM_SIMD`
# @param GCC_FLAGS flags for GCC or Clang
# @param MSVC_FLAGS flags flags for MSVC or Clang-cl
# @param ARCH_REGEX is matched against CMAKE_SYSTEM_PROCESSOR. Use empty string to skip
# the arch gate, such as WASM.
# @param TEST_SOURCE snippet of source code to try to compile/run.
function(_sync_simd_probe FEATURE GCC_FLAGS MSVC_FLAGS ARCH_REGEX TEST_SOURCE)
    set(_arch_ok TRUE)
    if(NOT ARCH_REGEX STREQUAL "")
        if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "${ARCH_REGEX}")
            set(_arch_ok FALSE)
        endif()
    endif()

    set(_can_var "SYNC_CAN_EMIT_${FEATURE}")
    set(_run_var "SYNC_BUILD_RUNS_${FEATURE}")

    if(NOT _arch_ok)
        if(NOT DEFINED ${_can_var})
            set(${_can_var} OFF CACHE INTERNAL "")
        endif()
        if(NOT DEFINED ${_run_var})
            set(${_run_var} OFF CACHE INTERNAL "")
        endif()
        return()
    endif()

    if(MSVC) # and Clang-cl
        set(_flags "${MSVC_FLAGS}")
    else()
        set(_flags "${GCC_FLAGS}")
    endif()

    set(_prev_flags "${CMAKE_REQUIRED_FLAGS}")
    set(CMAKE_REQUIRED_FLAGS "${_flags}")

    if(NOT DEFINED ${_can_var})
        check_cxx_source_compiles("${TEST_SOURCE}" ${_can_var})
    endif()
    if(${_can_var})
        message(STATUS "Sync SIMD: compiler can emit ${FEATURE}")
    endif()

    if(NOT DEFINED ${_run_var})
        if(CMAKE_CROSSCOMPILING)
            set(${_run_var} OFF CACHE INTERNAL "")
        else()
            check_cxx_source_runs("${TEST_SOURCE}" ${_run_var})
        endif()
    endif()
    if(${_run_var})
        message(STATUS "Sync SIMD: build machine runs ${FEATURE}")
    endif()

    set(CMAKE_REQUIRED_FLAGS "${_prev_flags}")
endfunction()

# ==============================
# x86_64
# ==============================

_sync_simd_probe(SSE42
    "-msse4.2"
    "/arch:SSE4.2"
    "(x86_64)|(X86_64)|(amd64)|(AMD64)|(i[3-6]86)"
    "
    #include <nmmintrin.h>
    int main() { return _mm_crc32_u32(0, 0) == 0 ? 0 : 0; }
    ")

_sync_simd_probe(AVX2
    "-mavx2"
    "/arch:AVX2"
    "(x86_64)|(X86_64)|(amd64)|(AMD64)"
    "
    #include <immintrin.h>
    int main() { __m256i v = _mm256_setzero_si256(); (void)v; return 0; }
    ")

_sync_simd_probe(AVX512
    "-mavx512f -mavx512bw"
    "/arch:AVX512"
    "(x86_64)|(X86_64)|(amd64)|(AMD64)"
    "
    #include <immintrin.h>
    int main() {
        __m512i v = _mm512_setzero_si512();
        __mmask64 m = _mm512_cmpeq_epi8_mask(v, v);
        return static_cast<int>(m & 0);
    }
    ")

# ==============================
# AArch64
# ==============================

# NEON is madatory on AArch64, but Scalable Vector Extension is not

# MSVC arm64 does not have any way to expose SVE stuff, so MSVC will just not support it here
_sync_simd_probe(SVE
    "-march=armv8-a+sve"
    ""
    "(aarch64)|(AARCH64)|(arm64)|(ARM64)"
    "
    #include <arm_sve.h>
    int main() { svbool_t p = svptrue_b8(); (void)p; return 0; }
    ")

# ==============================
# RISC-V64
# ==============================

_sync_simd_probe(RVV
    "-march=rv64gcv -mabi=lp64d"
    ""
    "(riscv64)|(RISCV64)"
    "
    #include <riscv_vector.h>
    int main() { __attribute__((unused)) vfloat32m1_t v; return 0; }
    ")

# ==============================
# WASM 32/64
# ==============================

if(EMSCRIPTEN)
    _sync_simd_probe(WASM_SIMD
        "-msimd128"
        ""
        ""
        "
        #include <wasm_simd128.h>
        int main() { v128_t v = wasm_i32x4_splat(0); (void)v; return 0; }
        ")
else()
    set(SYNC_CAN_EMIT_WASM_SIMD OFF CACHE INTERNAL "")
    set(SYNC_BUILD_RUNS_WASM_SIMD OFF CACHE INTERNAL "")
endif()

# Set the SYNC_BUILD_<X> variables, which are the actual ones that determine everything.
# SYNC_BUILD_<X> is true when the toolchain can emit this tier AND the user has
# not opted out. Cache INTERNAL so it's visible everywhere (subdirs, try_compile, etc.).
foreach(_feat SSE42 AVX2 AVX512 SVE RVV WASM_SIMD)
    if(SYNC_NO_${_feat} OR NOT SYNC_CAN_EMIT_${_feat})
        set(SYNC_BUILD_${_feat} OFF CACHE INTERNAL
            "Effective build inclusion for SIMD ${_feat}")
    else()
        set(SYNC_BUILD_${_feat} ON CACHE INTERNAL
            "Effective build inclusion for SIMD ${_feat}")
    endif()
endforeach()

# Sets SIMD flags for an entire target (executable / library). Each `ENABLE_<X>` is honored only
# if the toolchain can even emit it, AND the feature is not disabled by `SYNC_NO_<X>`.
# Use for features that should be platform-wide decisions.
#
# @param TARGET             The target to apply flags to.
# @param ENABLE_SSE42       ON to request SSE4.2 baseline on x86_64.
# @param ENABLE_AVX2        ON to request AVX2 on x86_64 (OFF usually for a whole target).
# @param ENABLE_AVX512      ON to request AVX512 F+BW on x86_64 (OFF usually for a whole target).
# @param ENABLE_SVE         ON to request SVE on ARM64. (OFF usually for a whole target).
# @param ENABLE_RVV         ON to request RVV on RISC-V.
# @param ENABLE_WASM_SIMD   ON to request msimd128 on WASM.
function(add_simd_flags_to_target TARGET
        ENABLE_SSE42 ENABLE_AVX2 ENABLE_AVX512
        ENABLE_SVE ENABLE_RVV ENABLE_WASM_SIMD)
    _sync_apply_simd_flags("${TARGET}" "target"
        "${ENABLE_SSE42}" "${ENABLE_AVX2}" "${ENABLE_AVX512}"
        "${ENABLE_SVE}" "${ENABLE_RVV}" "${ENABLE_WASM_SIMD}")
endfunction()

# Sets SIMD flags on a single source file. Use this to compile a dedicated TU
# (e.g. for AVX512 code paths) with a higher SIMD level than the rest of the target,
# so that dynamic dispatch can be used depending on the system the code is running on.
# It must be per ENTIRE source file target-wide setting can have unintended consequences
# for users whose systems do not support it, even if you don't directly call the SIMD
# functions, as the compiler may assume it's free to insert them.
#
# @param SOURCE_FILE        The source code file to apply
# @param ENABLE_SSE42       ON to request SSE4.2 on x86_64.
# @param ENABLE_AVX2        ON to request AVX2 on x86_64.
# @param ENABLE_AVX512      ON to request AVX512 F+BW on x86_64.
# @param ENABLE_SVE         ON to request SVE on ARM64.
# @param ENABLE_RVV         ON to request RVV on RISC-V.
# @param ENABLE_WASM_SIMD   ON to request msimd128 on WASM.
function(add_simd_flags_to_file SOURCE_FILE
        ENABLE_SSE42 ENABLE_AVX2 ENABLE_AVX512
        ENABLE_SVE ENABLE_RVV ENABLE_WASM_SIMD)
    _sync_apply_simd_flags("${SOURCE_FILE}" "file"
        "${ENABLE_SSE42}" "${ENABLE_AVX2}" "${ENABLE_AVX512}"
        "${ENABLE_SVE}" "${ENABLE_RVV}" "${ENABLE_WASM_SIMD}")
endfunction()

# Internal: shared body for the two public functions above.
function(_sync_apply_simd_flags WHAT KIND
        ENABLE_SSE42 ENABLE_AVX2 ENABLE_AVX512
        ENABLE_SVE ENABLE_RVV ENABLE_WASM_SIMD)

    # Resolve gcc-style and msvc-style flag lists per feature.
    set(_gcc_SSE42  "-msse4.2")
    set(_msvc_SSE42 "/arch:SSE4.2")
    set(_gcc_AVX2   "-mavx2")
    set(_msvc_AVX2  "/arch:AVX2")
    set(_gcc_AVX512 "-mavx512f;-mavx512bw")
    set(_msvc_AVX512 "/arch:AVX512")
    set(_gcc_SVE    "-march=armv8-a+sve")
    set(_msvc_SVE   "")
    set(_gcc_RVV    "-march=rv64gcv;-mabi=lp64d")
    set(_msvc_RVV   "")
    set(_gcc_WASM_SIMD   "-msimd128")
    set(_msvc_WASM_SIMD  "")

    set(_x86_arches "(x86_64)|(X86_64)|(amd64)|(AMD64)")
    set(_arm_arches "(aarch64)|(AARCH64)|(arm64)|(ARM64)")
    set(_rv_arches  "(riscv64)|(RISCV64)")

    foreach(_feat SSE42 AVX2 AVX512 SVE RVV WASM_SIMD)
        set(_enable_var "ENABLE_${_feat}")
        set(_enable "${${_enable_var}}")
        if(NOT _enable)
            continue()
        endif()
        # Single policy check: CAN_EMIT AND NOT user-opt-out.
        if(NOT SYNC_BUILD_${_feat})
            continue()
        endif()

        # Arch gate.
        set(_arch_ok TRUE)
        if(_feat STREQUAL "WASM_SIMD")
            if(NOT EMSCRIPTEN)
                set(_arch_ok FALSE)
            endif()
        elseif(_feat STREQUAL "SVE")
            if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "${_arm_arches}")
                set(_arch_ok FALSE)
            endif()
        elseif(_feat STREQUAL "RVV")
            if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "${_rv_arches}")
                set(_arch_ok FALSE)
            endif()
        else() # SSE42 / AVX2 / AVX512
            if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "${_x86_arches}")
                set(_arch_ok FALSE)
            endif()
        endif()
        if(NOT _arch_ok)
            continue()
        endif()

        if(MSVC) # and Clang-cl
            set(_flags "${_msvc_${_feat}}")
        else()
            set(_flags "${_gcc_${_feat}}")
        endif()
        if(_flags STREQUAL "")
            message(STATUS "Sync SIMD: ${_feat} requested but no flag known for this compile so skipping ${WHAT}")
            continue()
        endif()

        if(KIND STREQUAL "target")
            target_compile_options(${WHAT} PRIVATE ${_flags})
            message(STATUS "SIMD: applying ${_feat} to target ${WHAT}")
        else() # file
            get_source_file_property(_existing "${WHAT}" COMPILE_OPTIONS)
            if(_existing STREQUAL "NOTFOUND")
                set(_existing "")
            endif()
            list(APPEND _existing ${_flags})
            list(REMOVE_DUPLICATES _existing)
            set_source_files_properties("${WHAT}" PROPERTIES COMPILE_OPTIONS "${_existing}")
            message(STATUS "SIMD: applying ${_feat} to file ${WHAT}")
        endif()
    endforeach()
endfunction()
