#if SYNC_LIB_WITH_TESTS

// Make sure doctest symbols stay private to SyncLib as a static library
#ifdef _WIN32
#define DOCTEST_INTERFACE
#else
#define DOCTEST_INTERFACE __attribute__((visibility("hidden")))
#endif

#define DOCTEST_CONFIG_IMPLEMENT
#include "test_runner.h"
#include "../src/doctest.h"

extern "C" {
SY_API int run_sync_tests(int argc, char** argv) { return doctest::Context(argc, argv).run(); }
}

#endif