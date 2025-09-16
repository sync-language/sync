#include "tree_impl.hpp"

using namespace sy;

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

sy::Result<SourceTreeNode*, sy::AllocErr>
SourceTreeNode::initDir(sy::Allocator inAlloc, sy::Option<SourceTreeNode*> inParent, sy::StringSlice inName) {
    auto res = SourceTreeNode::init(inAlloc, inParent, inName);
    if (res.hasValue()) {
        res.value()->kind = sy::SourceFileKind::Directory;
    }
    return res;
}

sy::Result<SourceTreeNode*, sy::AllocErr> SourceTreeNode::initSyncSourceFile(sy::Allocator inAlloc,
                                                                             sy::Option<SourceTreeNode*> inParent,
                                                                             sy::StringSlice inName) {
    auto res = SourceTreeNode::init(inAlloc, inParent, inName);
    if (res.hasValue()) {
        res.value()->kind = sy::SourceFileKind::SyncSourceFile;
    }
    return res;
}

sy::Result<SourceTreeNode*, sy::AllocErr>
SourceTreeNode::initOtherFile(sy::Allocator inAlloc, sy::Option<SourceTreeNode*> inParent, sy::StringSlice inName) {
    auto res = SourceTreeNode::init(inAlloc, inParent, inName);
    if (res.hasValue()) {
        res.value()->kind = sy::SourceFileKind::OtherFile;
    }
    return res;
}

Result<SourceTreeNode*, AllocErr> SourceTreeNode::init(Allocator inAlloc, Option<SourceTreeNode*> inParent,
                                                       StringSlice inName) {
    SourceTreeNode* newNode;
    {
        auto newNodeResult = inAlloc.allocObject<SourceTreeNode>();
        if (newNodeResult.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        newNode = newNodeResult.value();
        memset(newNode, 0, sizeof(SourceTreeNode));
    }

    newNode->alloc = inAlloc;
    newNode->parent = inParent;
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

TreeImpl::~TreeImpl() noexcept {
    this->rootNode->~SourceTreeNode();
    this->allNodes.destroy(this->alloc);
}
