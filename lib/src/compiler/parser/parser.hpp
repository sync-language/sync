#ifndef SY_COMPILER_PARSER_PARSER_HPP_
#define SY_COMPILER_PARSER_PARSER_HPP_

#include "../../mem/allocator.hpp"
#include "../../types/sync_obj/sync_obj.hpp"
#include "../source_tree/source_tree.hpp"
#include "../tokenizer/tokenizer.hpp"

namespace sy {
struct ParseInfo {
    TokenIter tokenIter;
    Allocator alloc;
    Tokenizer tokenizer;
    /// Will always be of type `SourceFileKind::SyncSourceFile`
    const volatile SourceTreeNode* fileSource;
};
} // namespace sy

#endif // SY_COMPILER_PARSER_PARSER_HPP_