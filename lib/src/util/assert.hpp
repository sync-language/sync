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
    Backtrace::generate().print();\
}\
assert((expression) && message); \
} while(false)

#else // SY_CUSTOM_ASSERT_HANDLER

#endif // SY_CUSTOM_ASSERT_HANDLER

#endif // SY_UTIL_ASSERT_HPP_