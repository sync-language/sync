#include <assert.h>
#include <core_internal.h>
#include <iostream>
#include <signal.h>

void fatalHandler(const char* msg) {
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}

int main() {
    sy_set_fatal_error_handler(fatalHandler);
    SyRawRwLock lock{};
    assert(sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
    sy_raw_rwlock_destroy(&lock);
    return 0;
}