#pragma once
#ifndef SY_UTIL_ASSERT_HPP_
#define SY_UTIL_ASSERT_HPP_

#include "../testing/assert_handler.hpp"
#include "debug.hpp"
#include "os_callstack.hpp"
#include <assert.h>
#include <iomanip>
#include <iostream>
#include <sstream>

// // https://stackoverflow.com/a/22759544
// template <typename S, typename T> class is_streamable {
//     template <typename SS, typename TT>
//     static auto test(int) -> decltype(std::declval<SS&>() << std::declval<TT>(), std::true_type());

//     template <typename, typename> static auto test(...) -> std::false_type;

//   public:
//     static const bool value = decltype(test<S, T>(0))::value;
// };

#if SYNC_BACKTRACE_SUPPORTED
#define sy_assert(expression, message)                                                                                 \
    do {                                                                                                               \
        if (!(expression)) {                                                                                           \
            sy::Backtrace::generate().print();                                                                         \
            std::string msg;                                                                                           \
            try {                                                                                                      \
                std::stringstream ss;                                                                                  \
                ss << "Assertion failed: (" << #expression << ") \'" << message << "\', file " << __FILE__ << ':'      \
                   << __LINE__ << std::endl;                                                                           \
                msg = ss.str();                                                                                        \
                                                                                                                       \
            } catch (...) {                                                                                            \
            }                                                                                                          \
            auto handler = sy::getAssertHandler();                                                                     \
            handler(msg.c_str());                                                                                      \
            _ST_DEBUG_BREAK();                                                                                         \
        }                                                                                                              \
    } while (false)
#else
#define sy_assert(expression, message)                                                                                 \
    do {                                                                                                               \
        if (!(expression)) {                                                                                           \
            std::string msg;                                                                                           \
            try {                                                                                                      \
                std::stringstream ss;                                                                                  \
                ss << "Assertion failed: (" << #expression << ") \'" << message << "\', file " << __FILE__ << ':'      \
                   << __LINE__ << std::endl;                                                                           \
                msg = ss.str();                                                                                        \
                                                                                                                       \
            } catch (...) {                                                                                            \
            }                                                                                                          \
            auto handler = sy::getAssertHandler();                                                                     \
            handler(msg.c_str());                                                                                      \
            _ST_DEBUG_BREAK();                                                                                         \
        }                                                                                                              \
    } while (false)
#endif

#endif // SY_UTIL_ASSERT_HPP_