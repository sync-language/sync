#pragma once
#ifndef SY_UTIL_OS_CALLSTACK_HPP_
#define SY_UTIL_OS_CALLSTACK_HPP_

#include "../core.h"

#if SYNC_BACKTRACE_SUPPORTED

#include <string>
#include <vector>

namespace sy {
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

    std::vector<StackFrameInfo> frames{};

    static Backtrace generate() noexcept;

    /// Prints to stderr
    void print() const noexcept;
};
} // namespace sy

#endif

#endif // SY_UTIL_OS_CALLSTACK_HPP_