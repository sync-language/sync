#include "compiler.hpp"
#include "../core/core_internal.h"
#include "../program/program_error.hpp"
#include "../program/program_internal.hpp"
#include "../types/array/dynamic_array.hpp"
#include "../types/hash/map.hpp"
#include "../types/string/string.hpp"
#include "graph/module_dependency_graph.hpp"
#include "parser/base_nodes.hpp"
#include "parser/parser.hpp"
#include "source_tree/source_tree.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>

using namespace sy;
namespace fs = std::filesystem;

namespace sy {
extern ProgramErrorReporter defaultErrReporter;

struct CompilerImpl {
    Allocator alloc;
    MapUnmanaged<StringSlice, DynArrayUnmanaged<SemVer>> versions;
    MapUnmanaged<ModuleVersion, Module*> modules;
};

struct ModuleImpl {
    Allocator alloc;
    StringUnmanaged name{};
    SemVer version{};
    SourceTree sourceTree;
    SourceTreeNode* rootFile = nullptr;
    MapUnmanaged<StringSlice, const Module*> dependencies{};

    ModuleImpl(Allocator inAlloc) : alloc(inAlloc), sourceTree(inAlloc) {}

    Result<void, ModuleErr> setRootFileFromDisk(StringSlice path) noexcept;

    Result<void, ModuleErr> setRootFileFromString(StringSlice absolutePath, StringSlice fileContents) noexcept;

    Result<void, ModuleErr> addDependency(const Module* module) noexcept;
};
} // namespace sy

static void deleteImpl(void* impl) noexcept {
    CompilerImpl* self = reinterpret_cast<CompilerImpl*>(impl);
    Allocator alloc = self->alloc;

    { // free all module memory
        for (auto version : self->versions) {
            version.value.destroy(alloc);
        }
        self->versions.destroy(alloc);
        for (auto mod : self->modules) {
            mod.value->~Module();
            alloc.freeObject(mod.value);
        }
        self->modules.destroy(alloc);
    }

    self->~CompilerImpl();
    alloc.freeObject(self);
}

Result<Compiler, AllocErr> Compiler::create(Allocator alloc) noexcept {
    CompilerImpl* impl;
    {
        auto implAllocResult = alloc.allocObject<CompilerImpl>();
        if (implAllocResult.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        impl = implAllocResult.value();
        new (impl) CompilerImpl();
        impl->alloc = alloc;
    }

    Compiler self{};
    self.inner_ = reinterpret_cast<void*>(impl);
    return self;
}

Compiler::~Compiler() noexcept {
    if (this->inner_ == nullptr)
        return;
    deleteImpl(this->inner_);
    this->inner_ = nullptr;
}

sy::Compiler::Compiler(Compiler&& other) noexcept : inner_(other.inner_) { other.inner_ = nullptr; }

Compiler& sy::Compiler::operator=(Compiler&& other) noexcept {
    if (this == &other)
        return *this;

    if (this->inner_ != nullptr) {
        deleteImpl(this->inner_);
    }

    this->inner_ = other.inner_;
    other.inner_ = nullptr;
    return *this;
}

Result<Module*, AllocErr> sy::Compiler::addOrGetModule(StringSlice name, SemVer version) noexcept {
    CompilerImpl* self = reinterpret_cast<CompilerImpl*>(this->inner_);
    ModuleVersion modver = {name, version};
    {
        auto findResult = self->modules.find(modver);
        if (findResult.hasValue()) {
            return findResult.value();
        }
    }

    // doesn't exist yet

    Module* mod;
    {
        auto modAllocResult = Module::create(self->alloc, name, version);
        if (modAllocResult.hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
        mod = modAllocResult.value();
    }
    auto freeMod = [mod, self]() {
        mod->~Module();
        self->alloc.freeObject(mod);
    };

    { // add version to versions list for that module, or add new module to version list
        auto foundVersions = self->versions.find(name);
        if (foundVersions.hasValue()) {
            DynArrayUnmanaged<SemVer>& versions = foundVersions.value();
            bool inserted = false;
            for (size_t i = 0; i < versions.len(); i++) {
                SemVer v = versions[i];
                sy_assert(version != v, "Duplicate should have already been filtered");

                if (version > v) {
                    auto insertResult = versions.insertAt(version, self->alloc, i);
                    inserted = true;
                    if (insertResult.hasErr()) {
                        freeMod();
                        return Error(AllocErr::OutOfMemory);
                    }
                    break;
                }
            }

            if (inserted == false) {
                auto pushResult = versions.push(version, self->alloc);
                if (pushResult.hasErr()) {
                    freeMod();
                    return Error(AllocErr::OutOfMemory);
                }
            }
        } else {
            DynArrayUnmanaged<SemVer> versions;
            auto pushResult = versions.push(version, self->alloc);
            if (pushResult.hasErr()) {
                freeMod();
                return Error(AllocErr::OutOfMemory);
            }

            auto versionsInsertResult = self->versions.insert(self->alloc, name, std::move(versions));
            if (versionsInsertResult.hasErr()) {
                freeMod();
                versions.destroy(self->alloc);
                return Error(AllocErr::OutOfMemory);
            }
        }
    }

    auto moduleInsertResult = self->modules.insert(self->alloc, modver, mod);
    if (moduleInsertResult.hasErr()) {
        freeMod();
        DynArrayUnmanaged<SemVer>& versions = self->versions.find(name).value();
        for (size_t i = 0; i < versions.len(); i++) {
            SemVer v = versions[i];
            if (version == v) {
                versions.removeAt(i); // remove version
                break;
            }
        }
        return Error(AllocErr::OutOfMemory);
    }

    sy_assert(moduleInsertResult.value().hasValue() == false, "Module version not added to versions list correctly");
    return mod;
}

Result<DynArray<const Module*>, AllocErr> sy::Compiler::allModules() const noexcept {
    const CompilerImpl* self = reinterpret_cast<const CompilerImpl*>(this->inner_);
    DynArray<const Module*> modules(self->alloc);
    for (auto entry : self->modules) {
        if (modules.push(entry.value).hasErr()) {
            return Error(AllocErr::OutOfMemory);
        }
    }
    return modules;
}

static Result<ModuleDependencyGraph, ProgramError>
getCompileOrder(Allocator alloc, const MapUnmanaged<ModuleVersion, Module*>& modules) noexcept {
    DynArray<const Module*> allModules{alloc};
    for (const auto entry : modules) {
        if (allModules.push(entry.value).hasErr()) {
            return Error(ProgramError::OutOfMemory);
        }
    }
    auto res = ModuleDependencyGraph::init(std::move(allModules));
    if (res.hasErr()) {
        return Error(res.takeErr());
    }
    return res.takeValue();
}

static Result<ProgramModuleInternal*, ProgramError> compileModule(const ModuleImpl* mod, Allocator protAlloc,
                                                                  Allocator tempAlloc, ProgramErrorReporter errReporter,
                                                                  void* errReporterArg) {
    MapUnmanaged<const SourceTreeNode*, FileAst> asts;
    DynArray<const SourceTreeNode*> nodesToProcess(tempAlloc);
    (void)nodesToProcess;

    { // root file
        auto result = parseFile(tempAlloc, mod->rootFile, errReporter, errReporterArg);
        if (result.hasErr()) {
            return Error(result.takeErr());
        }
        FileAst ast = result.takeValue();
        sy_assert(ast.imports.len() == 0, "Imports not yet supported");
        if (asts.insert(tempAlloc, mod->rootFile, std::move(ast)).hasErr()) {
            return Error(ProgramError::OutOfMemory);
        }
        // TODO importing
    }

    // TODO other imported files

    ProgramModuleInternal* moduleInternal = nullptr;
    { // initialize module memory with protected allocator
        size_t functionCount = 0;
        size_t structCount = 0;
        for (const auto astEntry : asts) {
            functionCount += astEntry.value.nonGenericFunctions.len();
            structCount += astEntry.value.nonGenericStructs.len();
        }

        auto res =
            ProgramModuleInternal::init(protAlloc, mod->name.asSlice(), mod->version, functionCount, structCount);
        if (res.hasErr()) {
            return Error(ProgramError::OutOfMemory);
        }
        moduleInternal = res.value();
    }

    { // add entries for all functions so that recursion can work, and so that function names work
        size_t iter = 0;
        for (const auto astEntry : asts) {
            for (const IFunctionDefinition* func : astEntry.value.nonGenericFunctions) {
                auto unqualifiedRes = StringUnmanaged::copyConstructSlice(func->unqualifiedName(), protAlloc);
                if (unqualifiedRes.hasErr())
                    return Error(ProgramError::OutOfMemory);
                auto qualifiedRes = StringUnmanaged::copyConstructSlice(func->qualifiedName(), protAlloc);
                if (qualifiedRes.hasErr())
                    return Error(ProgramError::OutOfMemory);

                new (&moduleInternal->allFunctionNames[iter]) StringUnmanaged(std::move(unqualifiedRes.takeValue()));
                new (&moduleInternal->allFunctionQualifiedNames[iter])
                    StringUnmanaged(std::move(qualifiedRes.takeValue()));

                RawFunction _emptyFunc{};
                moduleInternal->allFunctions[iter] = _emptyFunc;
                moduleInternal->allFunctions[iter].name = moduleInternal->allFunctionNames[iter].asSlice();
                moduleInternal->allFunctions[iter].qualifiedName =
                    moduleInternal->allFunctionQualifiedNames[iter].asSlice();

                iter += 1;
            }
        }

        sy_assert(iter == moduleInternal->allFunctionsLen, "Iterator should match");
    }

    // TODO same for structs

    return moduleInternal;
}

static_assert(sizeof(Module) == sizeof(ModuleImpl*));
static_assert(sizeof(ProgramModule) == sizeof(ProgramModuleInternal*));

static Result<void, ProgramError> compileModules(ProgramInternal* programInternal, Allocator tempAlloc,
                                                 const MapUnmanaged<ModuleVersion, Module*>& modules,
                                                 ProgramErrorReporter errReporter, void* errReporterArg) noexcept {

    { // allocate memory for all modules
        auto modAllocRes = programInternal->protAlloc.asAllocator().allocArray<ProgramModule>(modules.len());
        if (modAllocRes.hasErr()) {
            return Error(ProgramError::OutOfMemory);
        }
        programInternal->allModules = modAllocRes.value();
        programInternal->allModulesLen = modules.len();
    }

    // MapUnmanaged<const Module*, bool> resolved{};
    auto _compileOrderRes = getCompileOrder(tempAlloc, modules);
    if (_compileOrderRes.hasErr()) {
        return Error(_compileOrderRes.takeErr());
    }
    ModuleDependencyGraph compileOrder = _compileOrderRes.takeValue();

    size_t index = 0;
    for (const Module* mod : compileOrder) {
        // compile module
        const ModuleImpl* asImpl = *reinterpret_cast<const ModuleImpl* const*>(mod);
        auto moduleCompileResult =
            compileModule(asImpl, programInternal->protAlloc.asAllocator(), tempAlloc, errReporter, errReporterArg);
        if (moduleCompileResult.hasErr()) {
            return Error(moduleCompileResult.takeErr());
        }

        ProgramModuleInternal* innerMem = moduleCompileResult.takeValue();
        ProgramModule* programModule = &programInternal->allModules[index];
        ProgramModuleInternal** programModuleInternal = reinterpret_cast<ProgramModuleInternal**>(programModule);
        *programModuleInternal = innerMem;

        index += 1;
    }

    return {};
}

Result<Program, ProgramError> sy::Compiler::compile(ProgramErrorReporter errReporter,
                                                    void* errReporterArg) const noexcept {
    if (errReporter == nullptr) {
        errReporter = defaultErrReporter;
    }

    ProgramInternal* programInternal;
    {
        ProtectedAllocator protAlloc;
        auto allocRes = protAlloc.asAllocator().allocObject<ProgramInternal>();
        if (allocRes.hasErr()) {
            // OUT OF PAGES? insane
            return Error(ProgramError::OutOfMemory);
        }
        programInternal = allocRes.value();
        new (programInternal) ProgramInternal();
        new (&programInternal->protAlloc) ProtectedAllocator(std::move(protAlloc));
        programInternal->errReporter = errReporter == nullptr ? defaultErrReporter : errReporter;
        programInternal->errReporterArg = errReporterArg;
    }

    const CompilerImpl* self = reinterpret_cast<const CompilerImpl*>(this->inner_);
    if (auto compErr = compileModules(programInternal, self->alloc, self->modules, errReporter, errReporterArg);
        compErr.hasErr()) {
        return Error(compErr.takeErr());
    }

    Program program;
    ProgramInternal** inner = reinterpret_cast<ProgramInternal**>(&program);
    *inner = programInternal;
    return Error(ProgramError::Unknown);
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

        std::string u8absolute = absolute.string();
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

    return {};
}

Result<void, ModuleErr> sy::ModuleImpl::setRootFileFromString(StringSlice absolutePath,
                                                              StringSlice fileContents) noexcept {
    sy_assert(this->rootFile == nullptr, "Root .sync file already set for this module");
    try {
        static const std::filesystem::path syncExtension = ".sync";

        const std::filesystem::path absolute(absolutePath.data(), absolutePath.data() + absolutePath.len());
        if (absolute.extension() != syncExtension) {
            return Error(ModuleErr::FileNotSyncSource);
        }
        if (absolute.has_parent_path() == false) {
            return Error(ModuleErr::SourceFileNoRootDir);
        }

        std::string u8absolute = absolute.string();
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
        sy_assert(this->rootFile->elem.syncSourceFile.hasValue() == false, "Should not have contents in root file");
        auto loadResult = StringUnmanaged::copyConstructSlice(fileContents, this->alloc);
        if (loadResult.hasErr()) {
            return Error(ModuleErr::OutOfMemory);
        }
        new (&this->rootFile->elem.syncSourceFile) Option<StringUnmanaged>(std::move(loadResult.takeValue()));
    } catch (const std::bad_alloc& e) {
        (void)e;
        return Error(ModuleErr::OutOfMemory);
    }

    return {};
}

Result<void, ModuleErr> ModuleImpl::addDependency(const Module* module) noexcept {
    auto res = this->dependencies.insert(this->alloc, module->name(), module);
    if (res.hasErr()) {
        return Error(ModuleErr::OutOfMemory);
    }
    if (res.value().hasValue()) {
        return Error(ModuleErr::DuplicateDependency);
    }
    return {};
}

static void deleteModuleImpl(void* impl) noexcept {
    ModuleImpl* self = reinterpret_cast<ModuleImpl*>(impl);
    Allocator alloc = self->alloc;
    self->name.destroy(alloc);
    self->~ModuleImpl();
    alloc.freeObject(self);
}

sy::Module::~Module() noexcept {
    if (this->inner_ == nullptr)
        return;
    deleteModuleImpl(this->inner_);
    this->inner_ = nullptr;
}

sy::Module::Module(Module&& other) noexcept : inner_(other.inner_) { other.inner_ = nullptr; }

Module& sy::Module::operator=(Module&& other) noexcept {
    if (this == &other)
        return *this;

    if (this->inner_ != nullptr) {
        deleteModuleImpl(this->inner_);
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

Result<void, ModuleErr> sy::Module::setRootFileFromString(StringSlice absolutePath, StringSlice fileContents) noexcept {
    return reinterpret_cast<ModuleImpl*>(this->inner_)->setRootFileFromString(absolutePath, fileContents);
}

Result<void, ModuleErr> sy::Module::addDependency(const Module* module) noexcept {
    return reinterpret_cast<ModuleImpl*>(this->inner_)->addDependency(module);
}

const MapUnmanaged<StringSlice, const Module*>& sy::Module::dependencies() const noexcept {
    return reinterpret_cast<const ModuleImpl*>(this->inner_)->dependencies;
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

        new (impl) ModuleImpl(alloc);
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

#if SYNC_LIB_WITH_TESTS

#include "../doctest.h"

TEST_CASE("[Compiler] empty compiler") {
    std::cerr << "[TEST DEBUG] Before Compiler::create()" << std::endl;
    Compiler c = Compiler::create().takeValue();
    std::cerr << "[TEST DEBUG] After Compiler::create(), before destructor" << std::endl;
    (void)c;
    std::cerr << "[TEST DEBUG] Test ending, destructor will run" << std::endl;
}

TEST_CASE("[Compiler] addOrGetModule empty module") {
    Compiler c = Compiler::create().takeValue();
    Module* m = c.addOrGetModule("example", SemVer()).value();
    (void)m;
}

TEST_CASE("[ModuleImpl] addOrGetModule empty module") {
    ModuleImpl impl{Allocator()};
    CHECK_EQ(impl.rootFile, nullptr);
    impl.setRootFileFromString("/example.sync", "a");
    CHECK_NE(impl.rootFile, nullptr);
    CHECK_EQ(impl.rootFile->kind, SourceFileKind::SyncSourceFile);
    CHECK_EQ(impl.rootFile->elem.syncSourceFile.value().asSlice(), "a");
}

#endif // SYNC_LIB_WITH_TESTS
