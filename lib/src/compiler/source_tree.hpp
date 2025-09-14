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

class SY_API SourceFile final {
    friend class SourceTree;

  public:
    /// Includes file name
    StringSlice absolutePath() const { return absolutePath_.asSlice(); }

    /// Does not include file name
    StringSlice relativePath() const { return relativePath_.asSlice(); }

    /// Does not include extension
    StringSlice fileName() const { return fileName_.asSlice(); }

    /// @return Full file contents
    StringSlice contents() const { return fileContents_.asSlice(); }

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
    static Result<SourceTree, SourceTreeError> allFilesInDirectoryRecursive(Allocator alloc, StringSlice dir) noexcept;

    const DynArrayUnmanaged<SourceFile>& files() const { return files_; }

  private:
    SourceTree(Allocator alloc) : alloc_(alloc) {}

  private:
    Allocator alloc_;
    DynArrayUnmanaged<SourceFile> files_{};
};
} // namespace sy

#endif // SY_COMPILER_SOURCE_TREE_HPP_