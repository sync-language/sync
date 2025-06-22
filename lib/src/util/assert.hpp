#pragma once
#ifndef SY_UTIL_ASSERT_HPP_
#define SY_UTIL_ASSERT_HPP_

#ifndef SY_CUSTOM_ASSERT_HANDLER

#include "os_callstack.hpp"
#include <assert.h>
#include <iostream>
#include <iomanip>

#define sy_assert(expression, message)\
do {\
if(!(expression)) { \
    auto trace = Backtrace::generate();\
    if(trace.frames.size() > 0) {\
        std::cerr << "Stack trace (most recent call first):\n";\
        size_t i = 0;\
        for(const auto frame : trace.frames) {\
            std::cerr << '#';\
            std::cerr.width((trace.frames.size() / 10) + 2);\
            std::cerr.setf(std::ios_base::left);\
            std::cerr << i;\
            std::cerr.unsetf(std::ios_base::left);\
            std::cerr.width(-1);\
            std::cerr << ' ' << frame.address << " in " << frame.functionName;\
            std::cerr << " at " << frame.fullFilePath << ':' << frame.lineNumber << std::endl;\
            i += 1;\
        }\
    }\
}\
assert((expression) && message); \
} while(false)

#else // SY_CUSTOM_ASSERT_HANDLER

#endif // SY_CUSTOM_ASSERT_HANDLER

#endif // SY_UTIL_ASSERT_HPP_