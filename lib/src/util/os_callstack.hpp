#pragma once
#ifndef SY_UTIL_OS_CALLSTACK_HPP_
#define SY_UTIL_OS_CALLSTACK_HPP_

#include "../core.h"
#include <string>
#include <vector>

class Backtrace {
public:

    class StackFrameInfo {
    public:
        std::string obj{};
        std::string functionName{};
        std::string fullFilePath{};
        int lineNumber{};
        void* address{};
    };

    std::vector<StackFrameInfo> frames;

    static Backtrace generate();

    /// Prints to stderr
    void print() const;
};

extern "C" {
    SY_API void test_backtrace_stuff();
}

#endif // SY_UTIL_OS_CALLSTACK_HPP_