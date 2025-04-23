#pragma once
#ifndef SY_MEM_OS_MEM_H_
#define SY_MEM_OS_MEM_H_

#include "../core.h"

extern "C" {
    extern void* aligned_malloc(size_t len, size_t align);
    
    extern void aligned_free(void* buf);
}

#endif // SY_MEM_OS_MEM_H_