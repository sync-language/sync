#if SYNC_LIB_WITH_TESTS

#define DOCTEST_CONFIG_NO_POSIX_SIGNALS
#define DOCTEST_CONFIG_IMPLEMENT
#include "test_runner.h"
#include "../src/doctest.h"

extern "C" {
SY_API int run_sync_tests(int argc, char** argv) { return doctest::Context(argc, argv).run(); }
}

#endif