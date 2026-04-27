//! API
#pragma once
#ifndef SY_THREADING_GENERATION_GEN_POOL_H_
#define SY_THREADING_GENERATION_GEN_POOL_H_

#include "../../core/core.h"
#include "../../mem/allocator.h"
#include "../../program/program_error.h"

struct SyType;

typedef struct SyGenOwner {
    /// PRIVATE: Internal only, not ABI stable.
    uint64_t gen_;
    /// PRIVATE: Internal only, not ABI stable.
    void* chunk_;
    /// PRIVATE: Internal only, not ABI stable.
    uint32_t objectIndex_;
} SyGenOwner;

typedef struct SyGenRef {
    /// PRIVATE: Internal only, not ABI stable.
    uint64_t gen_;
    /// PRIVATE: Internal only, not ABI stable.
    const void* chunk_;
    /// PRIVATE: Internal only, not ABI stable.
    uint32_t objectIndex_;
} SyGenRef;

typedef struct SyGenPool {
    /// PRIVATE: Internal only, not ABI stable.
    void* impl_;
} SyGenPool;

#ifdef __cplusplus
extern "C" {
#endif

SY_API SyAllocErr sy_gen_pool_init(SyAllocator allocator, SyGenPool* outGenPool);

SY_API SyAllocErr sy_gen_pool_add(SyGenPool* self, void* obj, const SyType* objType,
                                  SyGenOwner* outGenOwner);

SY_API SyProgramError sy_gen_owner_destroy(SyGenOwner* self);

SY_API SyGenRef sy_gen_owner_ref(const SyGenOwner* self);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SY_THREADING_GENERATION_GEN_POOL_H_
