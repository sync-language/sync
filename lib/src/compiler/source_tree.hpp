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
};

class SourceTree;

class SourceFile {
    friend class SourceTree;

  private:
    void destroy(Allocator& alloc) noexcept;

  private:
    StringUnmanaged absolutePath_;
    StringUnmanaged relativePath_;
    StringUnmanaged fileName_;
    StringUnmanaged fileContents_;
};

class SourceTree {
  public:
    ~SourceTree() noexcept;

    SourceTree(SourceTree&&) = default;

    /// @brief
    /// @param dir Directory to read from.
    /// @return
    static Result<SourceTree, SourceTreeError> fromDirectory(Allocator alloc, StringSlice dir) noexcept;

  private:
    SourceTree(Allocator alloc) : alloc_(alloc) {}

  private:
    Allocator alloc_;
    DynArrayUnmanaged<SourceFile> files_{};
};
} // namespace sy

#endif // SY_COMPILER_SOURCE_TREE_HPP_