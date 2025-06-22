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
        void* address;

        #if defined __APPLE || defined __GNUC__
        static StackFrameInfo parse(const char* const buffer);
        #endif
    };

    std::vector<StackFrameInfo> frames;

    static Backtrace getBacktrace();
};

#endif // SY_UTIL_OS_CALLSTACK_HPP_