#include "panic.hpp"
#include <iostream>

#ifdef _MSC_VER
#define DEBUG_BREAK __debugbreak()
#elif __GNUC__
#define DEBUG_BREAK __builtin_trap()
#endif

extern "C" [[noreturn]] void sy_panic_handler(int line, const char* filename, const char* message) {
    std::cerr << "File " << filename << ", Line " << line << "\n";
    std::cerr << message << std::endl;
    DEBUG_BREAK;
}