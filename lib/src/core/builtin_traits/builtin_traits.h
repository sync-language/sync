//! API
#pragma once
#ifndef _SY_CORE_BUILTIN_TRAITS_BUILTIN_TRAITS_H_
#define _SY_CORE_BUILTIN_TRAITS_BUILTIN_TRAITS_H_

struct SyFunction;

typedef struct SyBuiltInCoherentTraits {
    const SyFunction* clone;
    const SyFunction* equal;
    const SyFunction* hash;
    const SyFunction* compare;
    const SyFunction* elementWiseAtomicClone;
    const SyFunction* elementWiseAtomicMove;
} SyBuiltInCoherentTraits;
#endif // _SY_CORE_BUILTIN_TRAITS_BUILTIN_TRAITS_H_
