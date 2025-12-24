#ifndef _SY_COMPILER_PARSER_AST_HPP_
#define _SY_COMPILER_PARSER_AST_HPP_

#include "../../types/array/dynamic_array.hpp"

namespace sy {
struct ParsedNodeChildren {

    ParsedNodeChildren() noexcept = default;

    ~ParsedNodeChildren() noexcept;

    uint32_t len() const noexcept { return count; }

    uint32_t getChild(uint32_t index) const noexcept;

    Result<void, AllocErr> pushChild(uint32_t childIndex, Allocator alloc) noexcept;

  private:
    static constexpr uint32_t MAX_INLINE_STORAGE = (sizeof(DynArray<uint32_t>) / sizeof(uint32_t));

    struct InlineStorage {
        uint32_t storage[MAX_INLINE_STORAGE]{};
    };

    union Storage {
        InlineStorage inlineChildren{};
        DynArray<uint32_t> dynChildren;

        Storage() noexcept : inlineChildren(InlineStorage()) {}

        ~Storage() noexcept {}
    };

    bool isDynamic = false;
    uint32_t count = 0; // due to padding, this will fit good
    Storage storage{};
};

} // namespace sy

#endif // _SY_COMPILER_PARSER_AST_HPP_