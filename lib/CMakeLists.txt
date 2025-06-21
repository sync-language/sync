cmake_minimum_required (VERSION 3.19)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 17)

add_library(SyncLib STATIC
    "src/util/panic.cpp"
    "src/util/os_callstack.cpp"
    "src/mem/allocator.cpp"
    "src/mem/os_mem.cpp"
    "src/threading/sync_queue.cpp"
    "src/threading/sync_obj_val.cpp"
    "src/types/type_info.cpp"
    "src/types/function/function.cpp"
    "src/types/string/string_slice.cpp"
    "src/types/string/string.cpp"
    "src/types/sync_obj/sync_obj.cpp"
    "src/interpreter/stack.cpp"
    "src/interpreter/bytecode.cpp"
    "src/interpreter/interpreter.cpp"
    "src/program/program.cpp"
)

add_executable(SyncLibTests
    "src/test.cpp"
    "src/util/panic.cpp"
    "src/util/os_callstack.cpp"
    "src/mem/allocator.cpp"
    "src/mem/os_mem.cpp"
    "src/threading/sync_queue.cpp"
    "src/threading/sync_obj_val.cpp"
    "src/types/type_info.cpp"
    "src/types/function/function.cpp"
    "src/types/string/string_slice.cpp"
    "src/types/string/string.cpp"
    "src/types/sync_obj/sync_obj.cpp"
    "src/interpreter/stack.cpp"
    "src/interpreter/bytecode.cpp"
    "src/interpreter/interpreter.cpp"
    "src/program/program.cpp"
)

# Ensure all warnings are caught
if(MSVC)
  target_compile_options(SyncLib PRIVATE /W4 /WX)
  # C++ exceptions and unwind semantics. Necessary for Rust
  target_compile_options(SyncLib PRIVATE /EHsc) 
else()
  target_compile_options(SyncLib PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_compile_definitions(SyncLibTests PRIVATE SYNC_LIB_TEST)

set_target_properties(SyncLib PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

install(TARGETS SyncLib DESTINATION .)