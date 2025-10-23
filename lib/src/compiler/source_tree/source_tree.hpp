#pragma once
#ifndef SY_COMPILER_SOURCE_TREE_SOURCE_TREE_HPP_
#define SY_COMPILER_SOURCE_TREE_SOURCE_TREE_HPP_

#include "../../core.h"
#include "../../mem/allocator.hpp"
#include "../../threading/alloc_cache_align.hpp"
#include "../../types/array/dynamic_array.hpp"
#include "../../types/hash/map.hpp"
#include "../../types/option/option.hpp"
#include "../../types/result/result.hpp"
#include "../../types/string/string.hpp"
#include "file_type.hpp"

namespace sy {
struct SourceTreeNode;

#if defined(_MSC_VER)
// Supress warning for struct padding due to alignment specifier
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4324?view=msvc-170
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
struct alignas(ALLOC_CACHE_ALIGN) SourceTreeNode {
    /// @brief Uses `SourceFileKind` for variant access
    union Element {
        MapUnmanaged<StringSlice, SourceTreeNode*> directory;
        Option<StringUnmanaged> syncSourceFile;
        uint8_t otherFile;

        Element() noexcept;
        ~Element() noexcept {}
    };

    Allocator alloc;
    Option<SourceTreeNode*> parent;
    StringUnmanaged name;
    SourceFileKind kind;
    Element elem;

    ~SourceTreeNode() noexcept;

    static Result<SourceTreeNode*, AllocErr> init(Allocator inAlloc, Option<SourceTreeNode*> inParent,
                                                  StringSlice inName, SourceFileKind inKind) noexcept;
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

struct SourceTree {
    Allocator alloc;
    SourceTreeNode* rootNode = nullptr;
    // DynArrayUnmanaged<SourceTreeNode*> allNodes{};

    SourceTree(Allocator inAlloc) : alloc(inAlloc) {}

    ~SourceTree() noexcept;

    /// @brief
    /// @param absolutePath Absolute path to file, including for "remote" directories
    /// @param kind Whether directory, sync source code file, or other
    /// @return Either the node at that path already in the tree or a new node, or an error.
    Result<SourceTreeNode*, SourceTreeErr> insert(StringSlice absolutePath, SourceFileKind kind) noexcept;
};
} // namespace sy

#endif // SY_COMPILER_SOURCE_TREE_SOURCE_TREE_HPP_