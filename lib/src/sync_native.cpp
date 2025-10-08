#include "../test/test_runner.h"
#include <jni.h>

#if SYNC_LIB_WITH_TESTS
extern "C" JNIEXPORT void JNICALL Java_com_lib_sync_Sync_run_sync_tests(JNIEnv* env, jobject) {
    (void)env;
    run_sync_tests(0, nullptr);
}
#endif // SYNC_LIB_WITH_TESTS