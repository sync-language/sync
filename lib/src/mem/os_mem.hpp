#pragma once
#ifndef SY_MEM_OS_MEM_H_
#define SY_MEM_OS_MEM_H_

namespace sy {
    void* aligned_malloc(size_t len, size_t align);
    
    void aligned_free(void* buf);
}

#endif // SY_MEM_OS_MEM_H_