#include "module.hpp"
#include "../types/string/string.hpp"
#include "../util/assert.hpp"
#include "source_tree/source_tree.hpp"
#include <filesystem>
#include <fstream>
#include <new>

using namespace sy;
namespace fs = std::filesystem;

// Sync modules use semver https://semver.org/

namespace {
struct ModuleImpl {
    Allocator alloc;
    StringUnmanaged name;
    SemVer version;
    SourceTree sourceTree;
    SourceTreeNode* rootFile;

    Result<void, ModuleErr> setRootFileFromDisk(StringSlice path) noexcept;
};

Result<void, ModuleErr> ModuleImpl::setRootFileFromDisk(StringSlice path) noexcept {
    sy_assert(this->rootFile == nullptr, "Root .sync file already set for this module");
    try {
        static const std::filesystem::path syncExtension = ".sync";

        const std::filesystem::path absolute = [path]() {
            const std::filesystem::path maybeRelative(path.data(), path.data() + path.len());
            if (maybeRelative.is_relative()) {
                return fs::absolute(maybeRelative);
            }
            return maybeRelative;
        }();
        if (absolute.extension() != syncExtension) {
            return Error(ModuleErr::FileNotSyncSource);
        }

        std::string u8absolute = absolute.u8string();
        auto treeInsertResult =
            this->sourceTree.insert(StringSlice(u8absolute.data(), u8absolute.size()), SourceFileKind::SyncSourceFile);
        if (treeInsertResult.hasErr()) {
            SourceTreeErr err = treeInsertResult.err();
            switch (err) {
            case SourceTreeErr::OutOfMemory: {
                return Error(ModuleErr::OutOfMemory);
            }
            default:
                return Error(ModuleErr::Unknown);
            }
        }

        this->rootFile = treeInsertResult.value();

        auto loadResult = loadFileToString(this->alloc, absolute);
        if (loadResult.hasErr()) {
            return Error(loadResult.err());
        }

        sy_assert(this->rootFile->elem.syncSourceFile.hasValue() == false, "Should not have contents in root file");
        new (&this->rootFile->elem.syncSourceFile) Option<StringUnmanaged>(std::move(loadResult.takeValue()));

    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return Error(ModuleErr::Unknown);
    } catch (const std::bad_alloc& e) {
        (void)e;
        return Error(ModuleErr::OutOfMemory);
    }
}
} // namespace

static void deleteImpl(void* impl) noexcept {
    ModuleImpl* self = reinterpret_cast<ModuleImpl*>(impl);
    Allocator alloc = self->alloc;
    self->name.destroy(alloc);
    self->~ModuleImpl();
    alloc.freeObject(self);
}

static Result<StringUnmanaged, ModuleErr> loadFileToString(Allocator& alloc, const fs::path& path) {
    std::ifstream sourceFile(path);
    StringUnmanaged contents;
    if (!sourceFile.is_open()) {
        return Error(ModuleErr::ErrorOpeningSourceFile);
    }

    { // read file contents
      // go to end
        sourceFile.seekg(0, std::ios::end);
        size_t fileSize = sourceFile.tellg();
        auto contentResult = StringUnmanaged::fillConstruct(alloc, fileSize, '\0');
        if (contentResult.hasErr()) {
            return Error(ModuleErr::OutOfMemory);
        }

        new (&contents) StringUnmanaged(std::move(contentResult.takeValue()));
        sourceFile.seekg(0, std::ios::beg);         // back to beginning to read from
        sourceFile.read(contents.data(), fileSize); // directly into string
    }

    return contents;
}

sy::Module::~Module() noexcept {
    if (this->inner_ == nullptr)
        return;
    deleteImpl(this->inner_);
    this->inner_ = nullptr;
}

sy::Module::Module(Module&& other) noexcept : inner_(other.inner_) { other.inner_ = nullptr; }

Module& sy::Module::operator=(Module&& other) noexcept {
    if (this == &other)
        return *this;

    if (this->inner_ != nullptr) {
        deleteImpl(this->inner_);
    }

    this->inner_ = other.inner_;
    other.inner_ = nullptr;
    return *this;
}

StringSlice sy::Module::name() const { return reinterpret_cast<const ModuleImpl*>(this->inner_)->name.asSlice(); }

SemVer sy::Module::version() const { return reinterpret_cast<const ModuleImpl*>(this->inner_)->version; }

Result<void, ModuleErr> sy::Module::setRootFileFromDisk(StringSlice path) noexcept {
    return reinterpret_cast<ModuleImpl*>(this->inner_)->setRootFileFromDisk(path);
}

Result<Module*, AllocErr> Module::create(Allocator& alloc, StringSlice inName, SemVer inVersion) noexcept {
    Module* self;
    ModuleImpl* impl;
    {
        auto implAllocResult = alloc.allocObject<ModuleImpl>();
        if (implAllocResult.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        impl = implAllocResult.value();

        auto selfAllocResult = alloc.allocObject<Module>();
        if (selfAllocResult.hasErr()) {
            alloc.freeObject(impl);
            return Error(AllocErr::OutOfMemory);
        }
        self = selfAllocResult.value();

        memset(impl, 0, sizeof(ModuleImpl));
        impl->alloc = alloc;
        auto nameAllocResult = StringUnmanaged::copyConstructSlice(inName, alloc);
        if (nameAllocResult.hasErr()) {
            alloc.freeObject(impl);
            return Error(AllocErr::OutOfMemory);
        }
        auto _ = new (&impl->name) StringUnmanaged(std::move(nameAllocResult.takeValue()));
        (void)_;
        impl->version = inVersion;
        new (&impl->sourceTree) SourceTree(alloc);
        impl->rootFile = nullptr;
    }

    self->inner_ = reinterpret_cast<void*>(impl);
    return self;
}