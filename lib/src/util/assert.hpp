#pragma once
#ifndef SY_UTIL_ASSERT_HPP_
#define SY_UTIL_ASSERT_HPP_

#include "debug.hpp"
#include "os_callstack.hpp"
#include <assert.h>
#include <iomanip>
#include <iostream>

#if SYNC_BACKTRACE_SUPPORTED
#define sy_assert(expression, message)                                                                                 \
    do {                                                                                                               \
        if (!(expression)) {                                                                                           \
            sy::Backtrace::generate().print();                                                                         \
            try {                                                                                                      \
                std::cerr << "Assertion failed: (" << #expression << ") \'" << message << "\', file " << __FILE__      \
                          << ':' << __LINE__ << std::endl;                                                             \
            } catch (...) {                                                                                            \
            }                                                                                                          \
            _ST_DEBUG_BREAK();                                                                                         \
        }                                                                                                              \
    } while (false)
#else
#define sy_assert(expression, message) assert((expression) && message)
#endif

#endif // SY_UTIL_ASSERT_HPP_