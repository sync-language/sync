//! API
#pragma once
#ifndef SY_COMPILER_SOURCE_TREE_FILE_TYPE_HPP_
#define SY_COMPILER_SOURCE_TREE_FILE_TYPE_HPP_

namespace sy {
enum class SourceFileKind : int {
    Directory = 0,
    SyncSourceFile = 1,
    OtherFile = 2,
};
}

#endif // SY_COMPILER_SOURCE_TREE_FILE_TYPE_HPP_