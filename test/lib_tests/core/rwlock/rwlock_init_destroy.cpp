#include <core_internal.h>

int main() {
    SyRawRwLock lock{};
    sy_raw_rwlock_destroy(&lock);
    return 0;
}