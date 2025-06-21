#pragma once
#ifndef SY_UTIL_OS_CALLSTACK_HPP_
#define SY_UTIL_OS_CALLSTACK_HPP_

#include <string>
#include <vector>

class Backtrace {
public:

    class StackFrameInfo {
    public:
        std::string obj;
        std::string functionName;
        std::string fullFilePath;
        int lineNumber;

        static StackFrameInfo parse(const char* const buffer);
    };

    std::vector<StackFrameInfo> frames;

    static Backtrace getBacktrace();
};

#endif // SY_UTIL_OS_CALLSTACK_HPP_