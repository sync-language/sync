#include <doctest.h>
#include <threading/generation/gen_pool.hpp>
#include <threading/generation/gen_pool_internal.hpp>
#include <types/string/string.hpp>

namespace sy {
namespace internal {
struct Test_GenPool {
    static internal::GenPoolImpl* impl(GenPool& self) { return self.impl_; }
};

struct Test_GenOwner {
    template <typename T> static uint64_t gen(const GenOwner<T>& g) { return g.gen_; }
    template <typename T> static GenTypedPool::Chunk* chunk(const GenOwner<T>& g) {
        return reinterpret_cast<GenTypedPool::Chunk*>(g.chunk_);
    }
    template <typename T> static uint64_t index(const GenRef<T>& g) { return g.objectIndex_; }
};

struct Test_GenRef {
    template <typename T> static uint64_t gen(const GenRef<T>& g) { return g.gen_; }
    template <typename T> static GenTypedPool::Chunk* chunk(const GenOwner<T>& g) {
        return reinterpret_cast<GenTypedPool::Chunk*>(g.chunk_);
    }
    template <typename T> static uint64_t index(const GenRef<T>& g) { return g.objectIndex_; }
};
} // namespace internal
} // namespace sy

using namespace sy;
using sy::internal::GenPoolImpl;
using sy::internal::GenTypedPool;
using Chunk = sy::internal::GenTypedPool::Chunk;
