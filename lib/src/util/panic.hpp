#pragma once
#ifndef SY_UTIL_PANIC_HPP_
#define SY_UTIL_PANIC_HPP_

extern "C" [[noreturn]] void sy_panic_handler(int line, const char* filename, const char* message);

#define sy_panic(message) sy_panic_handler(__LINE__, __FILE__, message)

#endif // SY_UTIL_PANIC_HPP_