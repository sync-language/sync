cmake_minimum_required (VERSION 3.12)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("Sync")

set(CMAKE_CXX_STANDARD 17)

add_library(SyncLib STATIC
    "lib/src/test.cpp"
    "lib/src/mem/allocator.cpp"
    "lib/src/mem/os_mem.cpp"
)

# Ensure all warnings are caught
if(MSVC)
  target_compile_options(SyncLib PRIVATE /W4 /WX)
else()
  target_compile_options(SyncLib PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

set_target_properties(SyncLib PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

install(TARGETS SyncLib DESTINATION .)