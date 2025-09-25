cmake_minimum_required (VERSION 3.28)

project("CIExample")

set(CMAKE_CXX_STANDARD 17)

add_executable(Example example.cpp)

target_compile_options(Example PRIVATE -fsanitize=address)
set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")

