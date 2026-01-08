#include "ast.hpp"
#include "../../core/core_internal.h"
#include <new>

using namespace sy;

sy::ParsedNodeChildren::~ParsedNodeChildren() noexcept {
    if (this->isDynamic == false)
        return;

    auto _ = std::move(this->storage.dynChildren);
    (void)_;
    this->isDynamic = false;
}

uint32_t sy::ParsedNodeChildren::getChild(uint32_t index) const noexcept {
    sy_assert(index < this->count, "Index out of range");

    if (this->isDynamic)
        return this->storage.dynChildren.at(index);
    return this->storage.inlineChildren.storage[index];
}

Result<void, AllocErr> sy::ParsedNodeChildren::pushChild(uint32_t childIndex, Allocator alloc) noexcept {
    sy_assert(this->count < UINT32_MAX, "Exceeding maximum children");

    if (this->isDynamic) {
        this->count += 1;
        return this->storage.dynChildren.push(childIndex);
    }

    if (this->count < MAX_INLINE_STORAGE) {
        this->storage.inlineChildren.storage[this->count] = childIndex;
        this->count += 1;
        return {};
    }

    // resize
    DynArray<uint32_t> dynStorage(alloc);
    for (decltype(this->count) i = 0; i < this->count; i++) {
        if (dynStorage.push(this->storage.inlineChildren.storage[i]).hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
    }
    if (dynStorage.push(childIndex).hasErr())
        return Error(AllocErr::OutOfMemory);

    new (&this->storage.dynChildren) DynArray<uint32_t>(std::move(dynStorage));
    this->isDynamic = true;
    this->count += 1;
    return {};
}

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"

TEST_CASE("[ParsedNodeChildren] operations") {
    {
        ParsedNodeChildren children;
        CHECK_EQ(children.len(), 0);
    }
    {
        ParsedNodeChildren children;
        CHECK(children.pushChild(5, {}));
        CHECK_EQ(children.len(), 1);
        CHECK_EQ(children.getChild(0), 5);
    }
    {
        ParsedNodeChildren children;
        CHECK(children.pushChild(5, {}));
        CHECK(children.pushChild(6, {}));
        CHECK_EQ(children.len(), 2);
        CHECK_EQ(children.getChild(0), 5);
        CHECK_EQ(children.getChild(1), 6);
    }
    {
        ParsedNodeChildren children;

        for (uint32_t i = 0; i < 9; i++) {
            CHECK(children.pushChild(i, {}));
        }
        CHECK_EQ(children.len(), 9);
        for (uint32_t i = 0; i < 9; i++) {
            CHECK_EQ(children.getChild(i), i);
        }
    }
    {
        ParsedNodeChildren children;

        for (uint32_t i = 0; i < 50; i++) {
            CHECK(children.pushChild(i, {}));
        }
        CHECK_EQ(children.len(), 50);
        for (uint32_t i = 0; i < 50; i++) {
            CHECK_EQ(children.getChild(i), i);
        }
    }
}

#endif // SYNC_LIB_WITH_TESTS