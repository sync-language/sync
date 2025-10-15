#include "protected_allocator.hpp"
#include "../types/result/result.hpp"
#include "../util/assert.hpp"
#include "os_mem.hpp"

using namespace sy;

// https://developer.apple.com/documentation/BundleResources/Entitlements/com.apple.security.cs.allow-jit

#if defined(__EMSCRIPTEN__)

// No-op
static void makeMemoryReadOnly(void* baseAddress, size_t size) {
    (void)baseAddress;
    (void)size;
}

#elif defined(_MSC_VER) || defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
// clang-format off
#include <windows.h>
// clang-format on

static void makeMemoryReadOnly(void* baseAddress, size_t size) {
    const DWORD newProtect = PAGE_READONLY;
    DWORD oldProtect;
    const bool success = VirtualProtect(baseAddress, size, newProtect, &oldProtect);
    sy_assert(success, "Failed to make memory read-only");
}

#elif defined(__APPLE__) || defined(__GNUC__)
#include <sys/mman.h>

static void makeMemoryReadOnly(void* baseAddress, size_t size) {
    const int success = mprotect(baseAddress, size, PROT_READ);
    sy_assert(success == 0, "Failed to make memory read-only");
}

#endif

namespace sy {
struct MemoryProtectedNode {
    void* baseMem = nullptr;
    size_t size = 0;
    // Where memory will try to be allocated from
    size_t offset = 0;
    MemoryProtectedNode* prev = nullptr;

    static Result<MemoryProtectedNode, AllocErr> init(size_t minSize) noexcept {
        size_t pageSize = page_size();
        size_t allocSize = 0;
        if (minSize <= pageSize) {
            allocSize = pageSize;
        } else {
            const size_t remainder = minSize % pageSize;
            if (remainder == 0) {
                allocSize = minSize;
            } else {
                allocSize = minSize + (minSize - remainder);
            }
        }

        void* newMem = page_malloc(allocSize);
        if (newMem == nullptr) {
            return Error(AllocErr::OutOfMemory);
        }

        MemoryProtectedNode self{};
        self.baseMem = newMem;
        self.size = allocSize;
        return self;
    }

    Result<void*, AllocErr> tryAlloc(size_t len, size_t align) noexcept {
        const size_t remainder = offset % align;
        size_t actualOffset = offset;
        if (remainder != 0) {
            actualOffset += align - remainder;
        }

        if ((size - offset) < len) {
            return Error(AllocErr::OutOfMemory);
        }

        uint8_t* allocMem = reinterpret_cast<uint8_t*>(baseMem) + actualOffset;
        this->offset = actualOffset + len;
        return reinterpret_cast<void*>(allocMem);
    }
};
} // namespace sy

void sy::ProtectedAllocator::makeReadOnly() noexcept {
    MemoryProtectedNode* current = reinterpret_cast<MemoryProtectedNode*>(this->tail_);
    while (current != nullptr) {
        makeMemoryReadOnly(current->baseMem, current->size);
        current = current->prev;
    }
}

void* ProtectedAllocator::alloc(size_t len, size_t align) noexcept {
    std::scoped_lock _lock(mutex_);

    if (this->tail_ == nullptr) {
        auto res = MemoryProtectedNode::init(len + sizeof(MemoryProtectedNode));
        if (res.hasErr()) {
            return nullptr;
        }

        MemoryProtectedNode node = res.value();
        MemoryProtectedNode* baseMem = reinterpret_cast<MemoryProtectedNode*>(node.baseMem);
        *baseMem = node;
        this->tail_ = reinterpret_cast<void*>(baseMem);
    }

    {
        MemoryProtectedNode* tail = reinterpret_cast<MemoryProtectedNode*>(this->tail_);
        if (auto tryAllocRes = tail->tryAlloc(len, align); tryAllocRes.hasValue()) {
            return tryAllocRes.value();
        }

        const size_t minSize = (tail->size * 2) < (len + sizeof(MemoryProtectedNode))
                                   ? (len + sizeof(MemoryProtectedNode))
                                   : (tail->size * 2);
        auto newNodeRes = MemoryProtectedNode::init(minSize);
        if (newNodeRes.hasErr()) {
            return nullptr;
        }

        MemoryProtectedNode newNode = newNodeRes.value();
        newNode.prev = tail;
        MemoryProtectedNode* baseMem = reinterpret_cast<MemoryProtectedNode*>(newNode.baseMem);
        *baseMem = newNode;
        auto newAllocRes = baseMem->tryAlloc(len, align);
        sy_assert(newAllocRes.hasValue(), "This should not have failed");
        return newAllocRes.value();
    }
}

void ProtectedAllocator::free(void* buf, size_t len, size_t align) noexcept {
    (void)buf;
    (void)len;
    (void)align;
    sy_assert(false, "Memory should not be freed through the protected allocator. Memory is freed by the destructor");
}
