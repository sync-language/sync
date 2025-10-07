//! API
#ifndef SY_TEST_RUNNER_H_
#define SY_TEST_RUNNER_H_

#if SYNC_LIB_WITH_TESTS

#if __cplusplus
extern "C" {
#endif
extern "C" int run_sync_tests(int argc, char** argv);
#if __cplusplus
}
#endif

#endif // SYNC_LIB_WITH_TESTS

#endif // SY_TEST_RUNNER_H_