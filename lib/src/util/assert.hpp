#pragma once
#ifndef SY_UTIL_ASSERT_HPP_
#define SY_UTIL_ASSERT_HPP_

#include <assert.h>
#define sy_assert(expression, message) assert((expression) && message)

#endif // SY_UTIL_ASSERT_HPP_