# PS4 and PS5 OS are based on FreeBSD, and use x64, so this is a good approximation of that.

set(CMAKE_SYSTEM_NAME FreeBSD)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Zig is used in Sync, so might as well use it for cross compiling
find_program(ZIG_EXECUTABLE zig REQUIRED)
set(CMAKE_C_COMPILER "${ZIG_EXECUTABLE}")
set(CMAKE_CXX_COMPILER "${ZIG_EXECUTABLE}")
set(CMAKE_C_COMPILER_ARG1 "cc")
set(CMAKE_CXX_COMPILER_ARG1 "c++")

# Silly windows
set(CMAKE_AR "${CMAKE_CURRENT_LIST_DIR}/zig-ar.bat" CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB "${CMAKE_CURRENT_LIST_DIR}/zig-ranlib.bat" CACHE FILEPATH "Ranlib")

set(CMAKE_C_FLAGS_INIT "-target x86_64-freebsd")
set(CMAKE_CXX_FLAGS_INIT "-target x86_64-freebsd")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# some FreeBSD-specific flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
