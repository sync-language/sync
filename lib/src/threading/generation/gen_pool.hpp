//! API
#pragma once
#ifndef SY_THREADING_GENERATION_GEN_POOL_HPP_
#define SY_THREADING_GENERATION_GEN_POOL_HPP_

#include "../../core/core.h"
#include "../../mem/allocator.hpp"
#include "../../types/result/result.hpp"

namespace sy {
namespace internal {
struct GenTypedPool;
}

class Type;
class RawGenRef;

/// Generational reference pool, supporting atomic data access.
/// Supports two access modes, the first being clone / swap, the second being readonly/readwrite
/// locked iteration.
class GenPool {
  public:
    GenPool() = default;

    GenPool(GenPool&& other) noexcept;

    GenPool& operator=(GenPool&& other) noexcept;

    ~GenPool() noexcept;

    GenPool(const GenPool& other) = delete;

    GenPool& operator=(const GenPool& other) = delete;

    static Result<GenPool, AllocErr> init(Allocator allocator = Allocator()) noexcept;

    Result<RawGenRef, AllocErr> rawAdd(void* obj, const Type* objType) noexcept;

  private:
    struct GenPoolImpl;
    friend struct internal::GenTypedPool;

    GenPoolImpl* impl_ = nullptr;
};

class RawGenRef {
  public:
  private:
    friend struct internal::GenTypedPool;

    void* chunk_;
    uint32_t objectIndex_;
};

} // namespace sy

#endif // SY_THREADING_GENERATION_GEN_POOL_HPP_
