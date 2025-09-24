#include "source_tree.hpp"
#include "../../util/unreachable.hpp"
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

using namespace sy;
namespace fs = std::filesystem;

SourceTreeNode::Element::Element() noexcept {}

SourceTreeNode::~SourceTreeNode() noexcept {
    this->name.destroy(this->alloc);
    switch (this->kind) {
    case SourceFileKind::Directory: {
        this->elem.directory.destroy(this->alloc);
    } break;
    case SourceFileKind::SyncSourceFile: {
        if (this->elem.syncSourceFile.hasValue()) {
            this->elem.syncSourceFile.value().destroy(this->alloc);
        }
    } break;
    default:
        break;
    }
}

Result<SourceTreeNode*, AllocErr> SourceTreeNode::init(Allocator inAlloc, Option<SourceTreeNode*> inParent,
                                                       StringSlice inName, sy::SourceFileKind inKind) noexcept {
    SourceTreeNode* newNode;
    {
        auto newNodeResult = inAlloc.allocObject<SourceTreeNode>();
        if (newNodeResult.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        newNode = newNodeResult.value();
        new (newNode) SourceTreeNode{inAlloc, inParent, StringUnmanaged(), inKind, {}};
        new (&newNode->elem.directory) MapUnmanaged<StringSlice, SourceTreeNode*>();
    }

    {
        auto nameRes = StringUnmanaged::copyConstructSlice(inName, inAlloc);
        if (nameRes.hasErr()) {
            newNode->~SourceTreeNode();
            inAlloc.freeObject(newNode);
            return Error(AllocErr::OutOfMemory);
        }
        new (&newNode->name) StringUnmanaged(std::move(nameRes.takeValue()));
    }

    return newNode;
}

SourceTree::~SourceTree() noexcept {
    this->rootNode->~SourceTreeNode();
    // this->allNodes.destroy(this->alloc);
}

Result<SourceTreeNode*, SourceTreeErr> SourceTree::insert(sy::StringSlice absolutePath,
                                                          sy::SourceFileKind kind) noexcept {
    try {
        const fs::path path(absolutePath.data(), absolutePath.data() + absolutePath.len());

        size_t depth = 0;

        { // make sure root is valid
            // todo improve this
            for (auto _ : path) {
                (void)_;
                depth += 1;
            }
            if (depth == 0) {
                return Error(SourceTreeErr::InvalidRoot);
            }
        }

        // make root node if doesn't exist
        if (rootNode == nullptr) {
            auto rootDir = *path.begin();
            std::string root = rootDir.u8string();
            auto rootRes = SourceTreeNode::init(this->alloc, nullptr, StringSlice(root.c_str(), root.size()),
                                                SourceFileKind::Directory);
            if (rootRes.hasErr()) {
                return Error(SourceTreeErr::OutOfMemory);
            }
            this->rootNode = rootRes.value();
        }

        auto pathIter = path.begin();
        { // check root matches this tree's root
            std::string rootStr = (*pathIter).u8string();
            sy::StringSlice rootSlice(rootStr.c_str(), rootStr.size());
            if (rootSlice != rootNode->name.asSlice()) {
                return Error(SourceTreeErr::InvalidRoot);
            }
            ++pathIter;
        }

        if (depth == 1) {
            return this->rootNode;
        }

        SourceTreeNode* current = this->rootNode;
        size_t i = 1; // already did root
        for (; i < depth; i++) {
            if (current->kind != SourceFileKind::Directory) {
                return Error(SourceTreeErr::UsingFileAsDirectory);
            }

            std::string entryStr = (*pathIter).u8string();
            sy::StringSlice entrySlice(entryStr.c_str(), entryStr.size());
            if (entrySlice.len() == 0 || entrySlice == "/") { // path ends with '/' for instance
                return current;
            }
            auto findResult = current->elem.directory.find(entrySlice);

            const bool isLastEntry = i == (depth - 1);
            if (isLastEntry) {
                if (findResult.hasValue()) {
                    SourceTreeNode* found = findResult.value();
                    if (found->kind != kind) {
                        return Error(SourceTreeErr::MismatchedType);
                    }
                    return found;
                }

                auto nodeResult = SourceTreeNode::init(this->alloc, current, entrySlice, kind);
                if (nodeResult.hasErr()) {
                    return Error(SourceTreeErr::OutOfMemory);
                }
                SourceTreeNode* newNode = nodeResult.value();
                auto insertResult = current->elem.directory.insert(current->alloc, newNode->name.asSlice(), newNode);
                return newNode;
            } else {
                if (findResult.hasValue()) {
                    current = findResult.value();
                    continue;
                }

                auto nodeResult = SourceTreeNode::init(this->alloc, current, entrySlice, SourceFileKind::Directory);
                if (nodeResult.hasErr()) {
                    return Error(SourceTreeErr::OutOfMemory);
                }
                SourceTreeNode* newNode = nodeResult.value();
                auto insertResult = current->elem.directory.insert(current->alloc, newNode->name.asSlice(), newNode);
                current = newNode;
            }

            ++pathIter;
        }
    } catch (const std::bad_alloc& e) {
        (void)e;
        return Error(SourceTreeErr::OutOfMemory);
    }

    unreachable();
}

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"

TEST_CASE("only root") {
    {
        Allocator alloc;
        SourceTree tree(alloc);
        SourceTreeNode* node = tree.insert("/", SourceFileKind::Directory).value();
        CHECK_EQ(node->name.asSlice(), "/");
        CHECK_FALSE(node->parent);
        CHECK_EQ(node, tree.rootNode);
    }
    {
        Allocator alloc;
        SourceTree tree(alloc);
        SourceTreeNode* node = tree.insert("C:", SourceFileKind::Directory).value();
        CHECK_EQ(node->name.asSlice(), "C:");
        CHECK_FALSE(node->parent);
        CHECK_EQ(node, tree.rootNode);
    }
    {
        Allocator alloc;
        SourceTree tree(alloc);
        SourceTreeNode* node = tree.insert("D:", SourceFileKind::Directory).value();
        CHECK_EQ(node->name.asSlice(), "D:");
        CHECK_FALSE(node->parent);
        CHECK_EQ(node, tree.rootNode);
    }
    {
        Allocator alloc;
        SourceTree tree(alloc);
        SourceTreeNode* node = tree.insert("C:/", SourceFileKind::Directory).value();
        CHECK_EQ(node->name.asSlice(), "C:");
        CHECK_FALSE(node->parent);
        CHECK_EQ(node, tree.rootNode);
    }
}

TEST_CASE("multiple directories without ending slash") {
    {
        Allocator alloc;
        SourceTree tree(alloc);
        SourceTreeNode* node = tree.insert("/thing/example/stuff", SourceFileKind::Directory).value();
        CHECK_EQ(node->name.asSlice(), "stuff");
        SourceTreeNode* parentExample = node->parent.value();
        CHECK_EQ(parentExample->name.asSlice(), "example");
        SourceTreeNode* parentThing = parentExample->parent.value();
        CHECK_EQ(parentThing->name.asSlice(), "thing");
        SourceTreeNode* root = parentThing->parent.value();
        CHECK_EQ(root->name.asSlice(), "/");
        CHECK_FALSE(root->parent);
        CHECK_EQ(root, tree.rootNode);
    }
#if defined(_WIN32)
    {
        Allocator alloc;
        SourceTree tree(alloc);
        SourceTreeNode* node = tree.insert("\\thing\\example\\stuff", SourceFileKind::Directory).value();
        CHECK_EQ(node->name.asSlice(), "stuff");
        SourceTreeNode* parentExample = node->parent.value();
        CHECK_EQ(parentExample->name.asSlice(), "example");
        SourceTreeNode* parentThing = parentExample->parent.value();
        CHECK_EQ(parentThing->name.asSlice(), "thing");
        SourceTreeNode* root = parentThing->parent.value();
        CHECK_EQ(root->name.asSlice(), "\\");
        CHECK_FALSE(root->parent);
        CHECK_EQ(root, tree.rootNode);
    }
#endif // _WIN32
}

TEST_CASE("multiple directories with ending slash") {
    {
        Allocator alloc;
        SourceTree tree(alloc);
        SourceTreeNode* node = tree.insert("/thing/example/stuff/", SourceFileKind::Directory).value();
        CHECK_EQ(node->name.asSlice(), "stuff");
        SourceTreeNode* parentExample = node->parent.value();
        CHECK_EQ(parentExample->name.asSlice(), "example");
        SourceTreeNode* parentThing = parentExample->parent.value();
        CHECK_EQ(parentThing->name.asSlice(), "thing");
        SourceTreeNode* root = parentThing->parent.value();
        CHECK_EQ(root->name.asSlice(), "/");
        CHECK_FALSE(root->parent);
        CHECK_EQ(root, tree.rootNode);
    }
#if defined(_WIN32)
    {
        Allocator alloc;
        SourceTree tree(alloc);
        SourceTreeNode* node = tree.insert("\\thing\\example\\stuff\\", SourceFileKind::Directory).value();
        CHECK_EQ(node->name.asSlice(), "stuff");
        SourceTreeNode* parentExample = node->parent.value();
        CHECK_EQ(parentExample->name.asSlice(), "example");
        SourceTreeNode* parentThing = parentExample->parent.value();
        CHECK_EQ(parentThing->name.asSlice(), "thing");
        SourceTreeNode* root = parentThing->parent.value();
        CHECK_EQ(root->name.asSlice(), "\\");
        CHECK_FALSE(root->parent);
        CHECK_EQ(root, tree.rootNode);
    }
#endif // _WIN32
}

TEST_CASE("source code file") {
    Allocator alloc;
    SourceTree tree(alloc);
    SourceTreeNode* node = tree.insert("example/file.sync", SourceFileKind::SyncSourceFile).value();
    CHECK_EQ(node->name.asSlice(), "file.sync");
    CHECK_EQ(node->kind, SourceFileKind::SyncSourceFile);
    SourceTreeNode* parentExample = node->parent.value();
    CHECK_EQ(parentExample->name.asSlice(), "example");
    CHECK_EQ(parentExample->kind, SourceFileKind::Directory);
    CHECK_FALSE(parentExample->parent);
    CHECK_EQ(parentExample, tree.rootNode);
}

TEST_CASE("two files same directory") {
    Allocator alloc;
    SourceTree tree(alloc);
    SourceTreeNode* node1 = tree.insert("example/file1.sync", SourceFileKind::SyncSourceFile).value();
    CHECK_EQ(node1->name.asSlice(), "file1.sync");
    CHECK_EQ(node1->kind, SourceFileKind::SyncSourceFile);
    SourceTreeNode* node2 = tree.insert("example/file2.sync", SourceFileKind::SyncSourceFile).value();
    CHECK_EQ(node2->name.asSlice(), "file2.sync");
    CHECK_EQ(node2->kind, SourceFileKind::SyncSourceFile);
    SourceTreeNode* parent1 = node1->parent.value();
    SourceTreeNode* parent2 = node2->parent.value();
    CHECK_EQ(parent1, parent2);
    CHECK_EQ(parent1->name.asSlice(), "example");
    CHECK_EQ(parent1->kind, SourceFileKind::Directory);
}

#endif // SYNC_LIB_WITH_TESTS
