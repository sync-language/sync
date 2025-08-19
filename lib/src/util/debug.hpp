#pragma once
#ifndef SY_UTIL_DEBUG_HPP_
#define SY_UTIL_DEBUG_HPP_

#ifndef NDEBUG

#if defined(_MSC_VER) || defined(_WIN32)
#define _ST_DEBUG_BREAK() __debugbreak()
#elif defined(__APPLE__) || defined(__GNUC__)
#define _ST_DEBUG_BREAK() __builtin_trap()
#else
#define _ST_DEBUG_BREAK() ((void)0)
#endif

#else // NDEBUG

#define _ST_DEBUG_BREAK() ((void)0)

#endif // NDEBUG 

#endif // SY_UTIL_DEBUG_HPP_