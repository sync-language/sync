#include "protected_allocator.hpp"
#include "os_mem.hpp"

using namespace sy;

void* ProtectedAllocator::alloc(size_t len, size_t align) noexcept {
    std::scoped_lock _lock(mutex_);
    (void)len;
    (void)align;
    return nullptr;
}

void ProtectedAllocator::free(void* buf, size_t len, size_t align) noexcept {
    std::scoped_lock _lock(mutex_);
    (void)buf;
    (void)len;
    (void)align;
}
