//! API
#pragma once
#ifndef SY_COMPILER_SOURCE_TREE_HPP_
#define SY_COMPILER_SOURCE_TREE_HPP_

#include "../core.h"
#include "../mem/allocator.hpp"
#include "../types/array/dynamic_array.hpp"
#include "../types/result/result.hpp"
#include "../types/string/string.hpp"
#include "../types/string/string_slice.hpp"

namespace sy {

enum class SourceTreeError : int {
    OutOfMemory,
    DirectoryNotExist,
    NotDirectory,
    NoFiles,
    ErrorOpeningSourceFile,
    UnknownError,
};

class SourceTree;

/// Is either a file or directory
class SourceEntry {
  public:
    enum class Kind {
        Directory,
        SyncSourceFile,
        OtherFile,
    };

    Kind kind() const noexcept;

    StringSlice absolutePath() const noexcept;

  private:
    friend class SourceTree;
    const void* node_;
};

class SourceTree {
  public:
    ~SourceTree() noexcept;

    SourceTree(SourceTree&& other) noexcept;

    /// @brief
    /// @param dir Directory to read from.
    /// @return
    static Result<SourceTree, SourceTreeError> allFilesInDirectoryRecursive(Allocator alloc, StringSlice dir) noexcept;

    class SY_API Iterator final {
        friend class SourceTree;
        Iterator(const void* tree, const void* const* node) : tree_(tree), node_(node) {}
        const void* tree_;
        const void* const* node_;

      public:
        bool operator!=(const Iterator& other) const;
        SourceEntry operator*() const;
        Iterator& operator++();
        Iterator& operator++(int) {
            ++(*this);
            return *this;
        };
    };

    friend class Iterator;

    Iterator begin() const;

    Iterator end() const;

  private:
    SourceTree() = default;

    static SourceEntry makeEntry(const void* node);

    void* tree_ = nullptr;
};

} // namespace sy

#endif // SY_COMPILER_SOURCE_TREE_HPP_