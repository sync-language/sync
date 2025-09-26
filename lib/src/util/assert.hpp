#pragma once
#ifndef SY_UTIL_ASSERT_HPP_
#define SY_UTIL_ASSERT_HPP_

// #ifndef SY_CUSTOM_ASSERT_HANDLER

#include "debug.hpp"
#include "os_callstack.hpp"
#include <assert.h>
#include <iomanip>
#include <iostream>

#define sy_assert(expression, message)                                                                                 \
    do {                                                                                                               \
        if (!(expression)) {                                                                                           \
            Backtrace::generate().print();                                                                             \
            try {                                                                                                      \
                std::cerr << "Assertion failed: (" << #expression << ") \'" << message << "\', file " << __FILE__      \
                          << ':' << __LINE__ << std::endl;                                                             \
            } catch (...) {                                                                                            \
            }                                                                                                          \
            _ST_DEBUG_BREAK();                                                                                         \
        }                                                                                                              \
    } while (false)

// #else // SY_CUSTOM_ASSERT_HANDLER

// #endif // SY_CUSTOM_ASSERT_HANDLER

#endif // SY_UTIL_ASSERT_HPP_