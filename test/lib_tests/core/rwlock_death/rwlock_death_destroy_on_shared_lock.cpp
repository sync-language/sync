#include <assert.h>
#include <core_internal.h>

int main() {
    SyRawRwLock lock{};
    assert(sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
    sy_raw_rwlock_destroy(&lock);
}