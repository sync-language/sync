# Enables ASan + UBSan, TSan, and code coverage instrumentation.
# Finds the clang ASan libraries for windows if using clang-cl.

if(NOT _PROG_FILES_X86)
    execute_process(
        COMMAND cmd /c "echo %ProgramFiles(x86)%"
        OUTPUT_VARIABLE _PROG_FILES_X86
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()
if(NOT _PROG_FILES)
    execute_process(
        COMMAND cmd /c "echo %ProgramFiles%"
        OUTPUT_VARIABLE _PROG_FILES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(X86_64)|(amd64)|(AMD64)")
        set(_CLANG_ASAN_DYNAMIC_LIB_NAME "clang_rt.asan_dynamic-x86_64.lib")
        set(_CLANG_ASAN_DYNAMIC_THUNK_LIB_NAME "clang_rt.asan_dynamic_runtime_thunk-x86_64.lib")
        set(_CLANG_UBSAN_LIB_NAME "clang_rt.ubsan_standalone-x86_64.lib")
        set(_CLANG_ASAN_DYNAMIC_DLL_NAME "clang_rt.asan_dynamic-x86_64.dll")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(_CLANG_ASAN_DYNAMIC_LIB_NAME "clang_rt.asan_dynamic-aarch64.lib")
        set(_CLANG_ASAN_DYNAMIC_THUNK_LIB_NAME "clang_rt.asan_dynamic_runtime_thunk-aarch64.lib")
        set(_CLANG_UBSAN_LIB_NAME "clang_rt.ubsan_standalone-aarch64.lib")
        set(_CLANG_ASAN_DYNAMIC_DLL_NAME "clang_rt.asan_dynamic-aarch64.dll")
    endif()

    if(NOT LLVM_ASAN_LIB)
        file(GLOB _LLVM_ASAN_LIBS
            "${_PROG_FILES}/LLVM/lib/clang/*/lib/windows/${_CLANG_ASAN_DYNAMIC_LIB_NAME}"
            )
        if(_LLVM_ASAN_LIBS)
            list(SORT _LLVM_ASAN_LIBS)
            list(REVERSE _LLVM_ASAN_LIBS)
            list(GET _LLVM_ASAN_LIBS 0 LLVM_ASAN_LIB)
        endif()
    endif()

    if(NOT LLVM_ASAN_THUNK_LIB)
        file(GLOB _LLVM_ASAN_THUNK_LIBS
            "${_PROG_FILES}/LLVM/lib/clang/*/lib/windows/${_CLANG_ASAN_DYNAMIC_THUNK_LIB_NAME}"
            )
        if(_LLVM_ASAN_THUNK_LIBS)
            list(SORT _LLVM_ASAN_THUNK_LIBS)
            list(REVERSE _LLVM_ASAN_THUNK_LIBS)
            list(GET _LLVM_ASAN_THUNK_LIBS 0 LLVM_ASAN_THUNK_LIB)
        endif()
    endif()

    if(NOT LLVM_UBSAN_LIB)
        file(GLOB _LLVM_UBSAN_LIBS
            "${_PROG_FILES}/LLVM/lib/clang/*/lib/windows/${_CLANG_UBSAN_LIB_NAME}"
            )
        if(_LLVM_UBSAN_LIBS)
            list(SORT _LLVM_UBSAN_LIBS)
            list(REVERSE _LLVM_UBSAN_LIBS)
            list(GET _LLVM_UBSAN_LIBS 0 LLVM_UBSAN_LIB)
        endif()
    endif()

    if(NOT CLANG_ASAN_DLL)
        file(GLOB _ASAN_DLLS
            "${_PROG_FILES}/LLVM/lib/clang/*/lib/windows/${_CLANG_ASAN_DYNAMIC_DLL_NAME}"
            )
        if(_ASAN_DLLS)
            list(SORT _ASAN_DLLS)
            list(REVERSE _ASAN_DLLS)  # Newest first
            list(GET _ASAN_DLLS 0 CLANG_ASAN_DLL)
        endif()
    endif()
elseif(MSVC)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(X86_64)|(amd64)|(AMD64)")
        set(_CLANG_ASAN_DYNAMIC_DLL_NAME "clang_rt.asan_dynamic-x86_64.dll")
        set(_MSVC_ASAN_HOST_DIR "Hostx64/x64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(_CLANG_ASAN_DYNAMIC_DLL_NAME "clang_rt.asan_dynamic-arm64.dll")
        set(_MSVC_ASAN_HOST_DIR "Hostarm64/arm64")
    endif()

    if(NOT CLANG_ASAN_DLL AND _CLANG_ASAN_DYNAMIC_DLL_NAME)
        file(GLOB _ASAN_DLLS
            "${_PROG_FILES}/Microsoft Visual Studio/*/*/VC/Tools/MSVC/*/bin/${_MSVC_ASAN_HOST_DIR}/${_CLANG_ASAN_DYNAMIC_DLL_NAME}"
            "${_PROG_FILES_X86}/Microsoft Visual Studio/*/*/VC/Tools/MSVC/*/bin/${_MSVC_ASAN_HOST_DIR}/${_CLANG_ASAN_DYNAMIC_DLL_NAME}"
        )
        if(_ASAN_DLLS)
            list(SORT _ASAN_DLLS)
            list(REVERSE _ASAN_DLLS)  # Newest first
            list(GET _ASAN_DLLS 0 CLANG_ASAN_DLL)
        endif()
    endif()
endif()

# Enables UBSan as well for targets that support it
function(enable_asan TARGET)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(${TARGET} PRIVATE -fsanitize=address -fsanitize=undefined)
        target_link_libraries(${TARGET} PRIVATE
            "${LLVM_ASAN_LIB}"
            "${LLVM_ASAN_THUNK_LIB}"
            "${LLVM_UBSAN_LIB}"
        )

        # Parallel build race condition when many targets in the same directory all
        # try to copy the .dll simultaneously. Hash for distinct "fake target" names
        # so that the test/ and root stuff all get the .dll properly.
        string(MD5 _dir_hash "${CMAKE_CURRENT_BINARY_DIR}")
        if(NOT TARGET copy_asan_dll_${_dir_hash})
            add_custom_target(copy_asan_dll_${_dir_hash}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CLANG_ASAN_DLL}"
                    "${CMAKE_CURRENT_BINARY_DIR}/${_CLANG_ASAN_DYNAMIC_DLL_NAME}"
            )
        endif()
        add_dependencies(${TARGET} copy_asan_dll_${_dir_hash})
    elseif(MSVC)
        if(CLANG_ASAN_DLL)
            target_compile_options(${TARGET} PRIVATE /fsanitize=address)
            add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CLANG_ASAN_DLL}"
                    "$<TARGET_FILE_DIR:${TARGET}>/${_CLANG_ASAN_DYNAMIC_DLL_NAME}"
            )
        endif()
    else()
        target_compile_options(${TARGET} PRIVATE -fsanitize=address -fsanitize=undefined)
        target_link_options(${TARGET} PRIVATE -fsanitize=address -fsanitize=undefined)
    endif()
endfunction()

function(enable_tsan TARGET)
    if(WIN32)
        message(FATAL_ERROR "Thread Sanitizer not available on windows, even with clang-cl")
    else()
        target_compile_options(${TARGET} PRIVATE -fsanitize=thread -fno-PIE)
        target_link_options(${TARGET} PRIVATE -fsanitize=thread -no-pie)
    endif()
endfunction()

function(enable_coverage TARGET)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${TARGET} PRIVATE -fprofile-instr-generate -fcoverage-mapping -fcoverage-compilation-dir="${CMAKE_SOURCE_DIR}")
        target_link_options(${TARGET} PRIVATE -fprofile-instr-generate)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(${TARGET} PRIVATE --coverage -fprofile-abs-path)
        target_link_options(${TARGET} PRIVATE --coverage)
    else()
        if(MSVC)
            message(FATAL_ERROR "Use clang-cl on windows to get llvm-cov support, or mingw64 g++ to get gcov support")
        else()
            message(FATAL_ERROR "Coverage instrumentation only available for clang and g++")
        endif()
    endif()
endfunction()
