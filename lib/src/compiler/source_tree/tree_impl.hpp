#pragma once
#ifndef SY_COMPILER_SOURCE_TREE_TREE_IMPL_HPP_
#define SY_COMPILER_SOURCE_TREE_TREE_IMPL_HPP_

#include "../../core.h"
#include "../../mem/allocator.hpp"
#include "../../threading/alloc_cache_align.hpp"
#include "../../types/array/dynamic_array.hpp"
#include "../../types/hash/map.hpp"
#include "../../types/option/option.hpp"
#include "../../types/result/result.hpp"
#include "../../types/string/string.hpp"
#include "file_type.hpp"
#include <cstring>

struct SourceTreeNode;

struct alignas(ALLOC_CACHE_ALIGN) SourceTreeNode {
    /// @brief Uses `sy::SourceFileKind` for variant access
    union Element {
        sy::MapUnmanaged<sy::StringSlice, SourceTreeNode*> directory;
        sy::Option<sy::StringUnmanaged> syncSourceFile;
        uint8_t otherFile;

        Element() { memset(this, 0, sizeof(Element)); }
        ~Element() noexcept {}
    };

    sy::Allocator alloc;
    sy::Option<SourceTreeNode*> parent;
    sy::StringUnmanaged name;
    sy::SourceFileKind kind;
    Element elem;

    ~SourceTreeNode() noexcept;

    static sy::Result<SourceTreeNode*, sy::AllocErr> init(sy::Allocator inAlloc, sy::Option<SourceTreeNode*> inParent,
                                                          sy::StringSlice inName, sy::SourceFileKind inKind);
};

struct TreeImpl {
    sy::Allocator alloc;
    SourceTreeNode* rootNode = nullptr;
    // sy::DynArrayUnmanaged<SourceTreeNode*> allNodes{};

    TreeImpl(sy::Allocator inAlloc) : alloc(inAlloc) {}

    ~TreeImpl() noexcept;

    sy::Result<SourceTreeNode*, sy::SourceTreeErr> insert(sy::StringSlice absolutePath, sy::SourceFileKind kind);
};

#endif // SY_COMPILER_SOURCE_TREE_TREE_IMPL_HPP_