#include <assert.h>
#include <core_internal.h>

int main() {
    SyRawRwLock lock{};
    const bool _r1 = (sy_raw_rwlock_acquire_shared(&lock) == SY_ACQUIRE_ERR_NONE);
    assert(_r1);
    (void)_r1;
    sy_raw_rwlock_destroy(&lock);
}