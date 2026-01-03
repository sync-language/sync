#pragma once
#ifndef SY_MEM_OS_MEM_H_
#define SY_MEM_OS_MEM_H_

#include "../core/core.h"

extern "C" {
extern void* aligned_malloc(size_t len, size_t align);

extern void aligned_free(void* buf);

extern void* page_malloc(size_t len);

extern void page_free(void* pagesStart, size_t len);
}

size_t page_size();

#endif // SY_MEM_OS_MEM_H_