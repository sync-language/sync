#ifndef SY_MEM_PROTECTED_ALLOCATOR_HPP_
#define SY_MEM_PROTECTED_ALLOCATOR_HPP_

#include "allocator.hpp"
#include <mutex>

namespace sy {
class ProtectedAllocator : public IAllocator {
  protected:
    virtual void* alloc(size_t len, size_t align) noexcept;

    virtual void free(void* buf, size_t len, size_t align) noexcept;

  private:
    std::mutex mutex_{};
};
} // namespace sy

#endif // SY_MEM_PROTECTED_ALLOCATOR_HPP_