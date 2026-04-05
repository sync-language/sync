# Sets warnings as errors, more warnings, debug stuff, etc

function(set_base_compiler_flags TARGET)
    if(EMSCRIPTEN)
        target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    elseif(MSVC)
        target_compile_options(${TARGET} PRIVATE /W4 /WX /EHsc /Zi)
    else()
        target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    if(CMAKE_BUILD_TYPE EQUAL "Debug" OR CMAKE_BUILD_TYPE EQUAL "RelWithDebInfo")
        if(WIN32)
            target_link_libraries(${TARGET} PRIVATE "dbghelp")
        endif()
    endif()
endfunction()
