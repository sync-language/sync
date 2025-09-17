#include "source_tree.hpp"
#include "../types/hash/map.hpp"
#include "../util/assert.hpp"
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <string>

using namespace sy;
using std::filesystem::path;

/*
Every source file needs
- The actual file contents
- It's path (absolute is not actually necessary, but useful for debugging)
- The name of the file
*/

static Result<StringUnmanaged, AllocErr> pathToString(Allocator& alloc, const path& path) {
    try {
        std::string u8str = path.u8string();
        return StringUnmanaged::copyConstructSlice(StringSlice(u8str.c_str(), u8str.size()), alloc);
    } catch (const std::bad_alloc& e) {
        (void)e;
        return Error(AllocErr::OutOfMemory);
    }
}

namespace {
struct Tree {
    struct Node;

    struct Node {
        /// @brief Uses `sy::SourceEntry::Kind` for variant access
        union Element {
            MapUnmanaged<StringSlice, Node*> directory_;
            StringUnmanaged syncSourceFile_;
            uint8_t otherFile_;

            Element() { memset(this, 0, sizeof(Element)); }
            ~Element() noexcept {}
        };

        static Result<Node*, AllocErr> init(Allocator alloc, Node* parent, const path& fullPath, const path& name);

        ~Node();

        Allocator alloc_;
        Node* parent_;
        StringUnmanaged name_;
        StringUnmanaged path_;
        SourceEntry::Kind kind_;
        Element elem_;
    };

    Allocator alloc_;
    Node* topNode_;
    DynArrayUnmanaged<Node*> allNodes_;

    ~Tree();

    static Result<Tree*, AllocErr> initDir(Allocator alloc, const path& root) noexcept;

    Result<Node*, AllocErr> findDir(const path& path) noexcept;

    Result<void, AllocErr> addDir(const path& fullPath, const path& parentPath, const path& name) noexcept;

    Result<void, SourceTreeError> addSyncSourceFile(const path& fullPath, const path& parentPath,
                                                    const path& name) noexcept;

    Result<void, AllocErr> addOtherFile(const path& fullPath, const path& parentPath, const path& name);

  private:
    Tree(Allocator alloc) : alloc_(alloc) {}
};

Tree::~Tree() {
    this->topNode_->~Node();
    this->allNodes_.destroy(this->alloc_);
}

Result<Tree*, AllocErr> Tree::initDir(Allocator alloc, const path& root) noexcept {
    Tree* self;
    {
        auto res = alloc.allocObject<Tree>();
        if (res.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        self = res.value();
    }
    new (self) Tree(alloc);

    {
        auto res = alloc.allocObject<Node>();
        if (res.hasErr()) {
            alloc.freeObject(self);
            return Error(AllocErr::OutOfMemory);
        }
        self->topNode_ = res.value();
        memset(self->topNode_, 0, sizeof(Node));
    }

    self->topNode_->alloc_ = alloc;
    {
        auto res = pathToString(alloc, root);
        if (res.hasErr()) {
            alloc.freeObject(self->topNode_);
            alloc.freeObject(self);
        }
        new (&self->topNode_->path_) StringUnmanaged(std::move(res.takeValue()));
    }
    self->topNode_->kind_ = SourceEntry::Kind::Directory;
    // elem map already zero initialized

    return self;
}

Result<Tree::Node*, AllocErr> Tree::findDir(const path& path) noexcept {
    { // is root
        auto rootResult = pathToString(this->alloc_, path);
        if (rootResult.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }

        StringUnmanaged root = rootResult.takeValue();
        if (root.asSlice() == this->topNode_->path_.asSlice()) {
            root.destroy(this->alloc_);
            return this->topNode_;
        }
        root.destroy(this->alloc_);
    }

    size_t last = 0;
    for (const auto& _ : path) {
        (void)_;
        last++;
    }

    Node* current = this->topNode_;
    size_t i = 0;
    for (const auto& elem : path) {
        sy_assert(current->kind_ == SourceEntry::Kind::Directory, "Must be directory");
        auto pathStrRes = pathToString(this->alloc_, elem);
        if (pathStrRes.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        StringUnmanaged elemStr = pathStrRes.takeValue();

        for (auto dir : current->elem_.directory_) {
            if (dir.key == elemStr.asSlice()) {
                current = dir.value;
                elemStr.destroy(this->alloc_);
                if (i == (last - 1)) {
                    return current;
                }
                continue;
            }
        }

        elemStr.destroy(this->alloc_);
        i += 1;
    }
    return nullptr;
}

Result<void, AllocErr> Tree::addDir(const path& fullPath, const path& parentPath, const path& name) noexcept {
    auto findResult = this->findDir(parentPath);
    if (findResult.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }
    Node* parentNode = findResult.value();
    sy_assert(parentNode != nullptr, "Must find");

    Node* newNode;
    {
        auto newNodeResult = Node::init(this->alloc_, parentNode, fullPath, name);
        if (newNodeResult.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        newNode = newNodeResult.value();
    }

    newNode->kind_ = SourceEntry::Kind::Directory;

    if (!this->allNodes_.push(newNode, this->alloc_)) {
        newNode->~Node();
        this->alloc_.freeObject(newNode);
        return Error(AllocErr::OutOfMemory);
    }

    auto insertResult = parentNode->elem_.directory_.insert(this->alloc_, newNode->name_.asSlice(), newNode);
    if (insertResult.hasErr()) {
        this->allNodes_.removeAt(this->allNodes_.len() - 1);
        newNode->~Node();
        this->alloc_.freeObject(newNode);
        return Error(AllocErr::OutOfMemory);
    }
    sy_assert(insertResult.value().hasValue() == false, "what");
    return {};
}

Result<void, SourceTreeError> Tree::addSyncSourceFile(const path& fullPath, const path& parentPath,
                                                      const path& name) noexcept {
    auto findResult = this->findDir(parentPath);
    if (findResult.hasErr()) {
        return Error(SourceTreeError::OutOfMemory);
    }
    Node* parentNode = findResult.value();
    sy_assert(parentNode != nullptr, "Must find");

    Node* newNode;
    {
        auto newNodeResult = Node::init(this->alloc_, parentNode, fullPath, name);
        if (newNodeResult.hasErr()) {
            return Error(SourceTreeError::OutOfMemory);
        }
        newNode = newNodeResult.value();
    }

    newNode->kind_ = SourceEntry::Kind::SyncSourceFile;
    std::ifstream sourceFile(fullPath);
    if (!sourceFile.is_open()) {
        return Error(SourceTreeError::ErrorOpeningSourceFile);
    }

    { // read file contents
      // go to end
        sourceFile.seekg(0, std::ios::end);
        size_t fileSize = sourceFile.tellg();
        auto contentResult = StringUnmanaged::fillConstruct(this->alloc_, fileSize, '\0');
        if (contentResult.hasErr()) {
            newNode->~Node();
            this->alloc_.freeObject(newNode);
            return Error(SourceTreeError::OutOfMemory);
        }

        new (&newNode->elem_.syncSourceFile_) StringUnmanaged(std::move(contentResult.takeValue()));
        sourceFile.seekg(0, std::ios::beg);                               // back to beginning to read from
        sourceFile.read(newNode->elem_.syncSourceFile_.data(), fileSize); // directly into string
    }

    if (!this->allNodes_.push(newNode, this->alloc_)) {
        newNode->~Node();
        this->alloc_.freeObject(newNode);
        return Error(SourceTreeError::OutOfMemory);
    }

    auto insertResult = parentNode->elem_.directory_.insert(this->alloc_, newNode->name_.asSlice(), newNode);
    if (insertResult.hasErr()) {
        this->allNodes_.removeAt(this->allNodes_.len() - 1);
        newNode->~Node();
        this->alloc_.freeObject(newNode);
        return Error(SourceTreeError::OutOfMemory);
    }
    sy_assert(insertResult.value().hasValue() == false, "what");

    return {};
}

Result<void, AllocErr> Tree::addOtherFile(const path& fullPath, const path& parentPath, const path& name) {
    auto findResult = this->findDir(parentPath);
    if (findResult.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }
    Node* parentNode = findResult.value();
    sy_assert(parentNode != nullptr, "Must find");

    Node* newNode;
    {
        auto newNodeResult = Node::init(this->alloc_, parentNode, fullPath, name);
        if (newNodeResult.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        newNode = newNodeResult.value();
    }

    newNode->kind_ = SourceEntry::Kind::OtherFile;

    if (!this->allNodes_.push(newNode, this->alloc_)) {
        newNode->~Node();
        this->alloc_.freeObject(newNode);
        return Error(AllocErr::OutOfMemory);
    }

    auto insertResult = parentNode->elem_.directory_.insert(this->alloc_, newNode->name_.asSlice(), newNode);
    if (insertResult.hasErr()) {
        this->allNodes_.removeAt(this->allNodes_.len() - 1);
        newNode->~Node();
        this->alloc_.freeObject(newNode);
        return Error(AllocErr::OutOfMemory);
    }
    sy_assert(insertResult.value().hasValue() == false, "what");

    return {};
}

Result<Tree::Node*, AllocErr> Tree::Node::init(Allocator alloc, Node* parent, const path& fullPath, const path& name) {
    Node* newNode;
    {
        auto newNodeResult = alloc.allocObject<Node>();
        if (newNodeResult.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        newNode = newNodeResult.value();
        memset(newNode, 0, sizeof(Node));
    }

    newNode->alloc_ = alloc;
    newNode->parent_ = parent;
    {
        auto nameRes = pathToString(alloc, name);
        if (nameRes.hasErr()) {
            newNode->~Node();
            alloc.freeObject(newNode);
            return Error(AllocErr::OutOfMemory);
        }
        new (&newNode->name_) StringUnmanaged(std::move(nameRes.takeValue()));
    }
    {
        auto fullPathRes = pathToString(alloc, fullPath);
        if (fullPathRes.hasErr()) {
            newNode->~Node();
            alloc.freeObject(newNode);
            return Error(AllocErr::OutOfMemory);
        }
        new (&newNode->path_) StringUnmanaged(std::move(fullPathRes.takeValue()));
    }

    return newNode;
}

Tree::Node::~Node() {
    this->name_.destroy(this->alloc_);
    this->path_.destroy(this->alloc_);
    switch (this->kind_) {
    case SourceEntry::Kind::Directory: {
        this->elem_.directory_.destroy(this->alloc_);
    } break;
    case SourceEntry::Kind::SyncSourceFile: {
        this->elem_.syncSourceFile_.destroy(this->alloc_);
    } break;
    default:
        break;
    }
}

} // namespace

SourceEntry::Kind SourceEntry::kind() const noexcept {
    const Tree::Node* node = reinterpret_cast<const Tree::Node*>(this->node_);
    return node->kind_;
}

StringSlice SourceEntry::absolutePath() const noexcept {
    const Tree::Node* node = reinterpret_cast<const Tree::Node*>(this->node_);
    return node->path_.asSlice();
}

// SourceTree::~SourceTree() noexcept {
//     if (this->tree_ == nullptr)
//         return;

//     Tree* tree = reinterpret_cast<Tree*>(this->tree_);
//     Allocator alloc = tree->alloc_;
//     tree->~Tree();
//     alloc.freeObject(tree);
//     this->tree_ = nullptr;
// }

// SourceTree::SourceTree(SourceTree&& other) noexcept : tree_(other.tree_) { other.tree_ = nullptr; }

// Result<SourceTree, SourceTreeError> sy::SourceTree::allFilesInAbsoluteDirectoryRecursive(Allocator alloc,
//                                                                                          StringSlice dir) noexcept {
//     SourceTree self;

//     try {
//         const std::filesystem::path syncExtension = ".sync";
//         const std::filesystem::path path(dir.data(), dir.data() + dir.len());
//         sy_assert(path.is_absolute(), "Expected absolute path");

//         if (!std::filesystem::exists(path)) {
//             return Error(SourceTreeError::DirectoryNotExist);
//         }
//         if (!std::filesystem::is_directory(path)) {
//             return Error(SourceTreeError::NotDirectory);
//         }

//         Tree* tree;
//         { // init tree
//             auto res = Tree::initDir(alloc, path);
//             if (res.hasErr()) {
//                 return Error(SourceTreeError::OutOfMemory);
//             }
//             tree = res.value();
//         }

//         self.tree_ = reinterpret_cast<void*>(tree);
//         // if exception occurs, SourceTree destructor will ensure no leaks

//         for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
//             const std::filesystem::path& entryPath = entry.path();
//             const std::filesystem::path parentPath = entryPath.parent_path();
//             const std::filesystem::path name = entryPath.filename();

//             if (entry.is_directory()) {
//                 if (!tree->addDir(entryPath, parentPath, name)) {
//                     return Error(SourceTreeError::OutOfMemory);
//                 }
//             } else if (entry.is_regular_file()) {
//                 if (name.extension() == syncExtension) {
//                     if (auto res = tree->addSyncSourceFile(entryPath, parentPath, name); res.hasErr()) {
//                         return Error(res.err());
//                     }
//                 } else {
//                     if (auto res = tree->addOtherFile(entryPath, parentPath, name); res.hasErr()) {
//                         return Error(SourceTreeError::OutOfMemory);
//                     }
//                 }
//             }
//         }

//     } catch (const std::filesystem::filesystem_error& e) {
//         std::cerr << "Filesystem error: " << e.what() << std::endl;
//         return Error(SourceTreeError::UnknownError);
//     } catch (const std::bad_alloc& e) {
//         (void)e;
//         return Error(SourceTreeError::OutOfMemory);
//     }

//     return self;
// }

// bool SourceTree::Iterator::operator!=(const Iterator& other) const { return this->node_ != other.node_; }

// SourceEntry SourceTree::Iterator::operator*() const { return makeEntry(*this->node_); }

// SourceTree::Iterator& SourceTree::Iterator::operator++() {
//     const Tree* tree = reinterpret_cast<const Tree*>(this->tree_);
//     const Tree::Node* const* node = reinterpret_cast<const Tree::Node* const*>(this->node_);
//     const Tree::Node* const* maxNode = tree->allNodes_.data() + tree->allNodes_.len();
//     node++;
//     if (node == maxNode) {
//         this->node_ = nullptr;
//     } else {
//         this->node_ = reinterpret_cast<const void* const*>(node);
//     }
//     return *this;
// }

// SourceTree::Iterator SourceTree::begin() const {
//     const Tree* tree = reinterpret_cast<const Tree*>(this->tree_);
//     const Tree::Node* const* allNodes = tree->allNodes_.data();
//     return Iterator(this->tree_, reinterpret_cast<const void* const*>(allNodes));
// }

// SourceTree::Iterator SourceTree::end() const { return Iterator(this->tree_, nullptr); }

// SourceEntry sy::SourceTree::makeEntry(const void* node) {
//     SourceEntry entry;
//     entry.node_ = node;
//     return entry;
// }

#if SYNC_LIB_WITH_TESTS

#include "../doctest.h"

// TEST_CASE("some stuff") {
//     auto res = SourceTree::allFilesInAbsoluteDirectoryRecursive(
//         Allocator(), "C:\\Users\\Admin\\Documents\\sync\\lib\\test\\source_tree_stuff");
//     CHECK(res.hasValue());

//     SourceTree tree = res.takeValue();
//     for (auto entry : tree) {
//         std::cout << entry.absolutePath().data() << std::endl;
//     }
// }

#endif // SYNC_LIB_WITH_TESTS