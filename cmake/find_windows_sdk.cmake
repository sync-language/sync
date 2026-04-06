# Finds MT and RC stuff for windows compilation.

if(DEFINED ENV{VSCMD_VER})
    return()
endif()

if(CMAKE_C_COMPILER MATCHES "gcc|mingw" OR CMAKE_CXX_COMPILER MATCHES "g\\+\\+|mingw")
    return()
endif()

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

set(_VSWHERE_EXE "${_PROG_FILES_X86}/Microsoft Visual Studio/Installer/vswhere.exe")
if(NOT EXISTS ${_VSWHERE_EXE})
    message(FATAL_ERROR
        "vswhere.exe not found, please install Visual Studio build tools"
    )
endif()

# Just works on host systems right now, not cross compile. That can be done with toolchain files.
if("$ENV{PROCESSOR_ARCHITECTURE}" STREQUAL "ARM64")
    set(_HOST_VS_BIN  "HostArm64/arm64")
    set(_HOST_SDK_BIN "arm64")
else()
    set(_HOST_VS_BIN  "Hostx64/x64")
    set(_HOST_SDK_BIN "x64")
endif()

if(NOT _VS_INSTALL_PATH)
    execute_process(
        COMMAND "${_VSWHERE_EXE}" -latest -property installationPath
        OUTPUT_VARIABLE _VS_INSTALL_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

if(NOT CMAKE_MT)
    file(GLOB _MT_CANDIDATES
        "${_PROG_FILES_X86}/Windows Kits/10/bin/*/${_HOST_SDK_BIN}/mt.exe"
    )
    if(NOT _MT_CANDIDATES AND _VS_INSTALL_PATH)
        file(GLOB _MT_CANDIDATES
            "${_VS_INSTALL_PATH}/VC/Tools/MSVC/*/bin/${_HOST_VS_BIN}/mt.exe"
        )
    endif()
    if(_MT_CANDIDATES)
        list(SORT _MT_CANDIDATES)
        list(REVERSE _MT_CANDIDATES)
        list(GET _MT_CANDIDATES 0 CMAKE_MT)
        set(CMAKE_MT "${CMAKE_MT}" CACHE FILEPATH "" FORCE)
    endif()
endif()

if(NOT CMAKE_RC_COMPILER)
    file(GLOB _SDK_BIN_DIRS
        "${_PROG_FILES_X86}/Windows Kits/10/bin/*/${_HOST_SDK_BIN}"
    )
    if(_SDK_BIN_DIRS)
        list(SORT _SDK_BIN_DIRS)
        list(REVERSE _SDK_BIN_DIRS)
    endif()
    find_program(CMAKE_RC_COMPILER
        NAMES llvm-rc.exe rc.exe
        HINTS
            "${_PROG_FILES}/LLVM/bin"
            ${_SDK_BIN_DIRS}
    )
endif()
