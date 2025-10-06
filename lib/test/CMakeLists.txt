cmake_minimum_required (VERSION 3.28)

project("RiscvTest")

option(SYNC_LIB_WITH_TESTS "Compile doctest tests as well" OFF)
option(SYNC_LIB_DOCTEST_ADD_CTESTS "Add all test cases to CTest individually" ON)
option(SYNC_LIB_ASAN "Use compiler specific address sanitizing" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(RiscvTest "main.cpp")