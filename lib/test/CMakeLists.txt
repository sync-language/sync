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

set_target_properties(Example PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

install(TARGETS Example DESTINATION .)

enable_testing()
add_test(NAME Example COMMAND Example)

