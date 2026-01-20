#if SYNC_LIB_WITH_TESTS

// #define DOCTEST_CONFIG_NO_POSIX_SIGNALS redefine issue
#define DOCTEST_CONFIG_IMPLEMENT
#include "test_runner.h"
#include "../src/doctest.h"

#if defined(DOCTEST_CONFIG_POSIX_SIGNALS)
#error "DOCTEST_CONFIG_POSIX_SIGNALS should not be defined when DOCTEST_CONFIG_NO_POSIX_SIGNALS is set"
#endif

extern "C" {
SY_API int run_sync_tests(int argc, char** argv) { return doctest::Context(argc, argv).run(); }
}

#endif