#include "compiler.hpp"
#include "../types/hash/map.hpp"
#include "../types/string/string.hpp"
#include "../util/assert.hpp"
#include <cstring>

using namespace sy;

struct ModuleVersion {
    StringSlice name;
    SemVer version;

    bool operator==(const ModuleVersion& other) const {
        return this->name == other.name && this->version == other.version;
    }
};

namespace std {
template <> struct hash<ModuleVersion> {
    size_t operator()(const ModuleVersion& obj) { return obj.name.hash(); }
};
} // namespace std

struct CompilerImpl {
    Allocator alloc;
    MapUnmanaged<StringSlice, DynArrayUnmanaged<SemVer>> versions;
    MapUnmanaged<ModuleVersion, Module*> modules;
};

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

Result<Module*, Compiler::AddEmptyModuleErr> sy::Compiler::addEmptyModule(StringSlice name, SemVer version) noexcept {
    CompilerImpl* self = reinterpret_cast<CompilerImpl*>(this->inner_);

    Module* mod;
    {
        auto modAllocResult = Module::create(self->alloc, name, version);
        if (modAllocResult.hasErr()) {
            return Error(AddEmptyModuleErr::OutOfMemory);
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
                if (version == v) {
                    freeMod();
                    return Error(AddEmptyModuleErr::DuplicateModuleVersion);
                }

                if (version > v) {
                    auto insertResult = versions.insertAt(version, self->alloc, i);
                    inserted = true;
                    if (insertResult.hasErr()) {
                        freeMod();
                        return Error(AddEmptyModuleErr::OutOfMemory);
                    }
                    break;
                }
            }

            if (inserted == false) {
                auto pushResult = versions.push(version, self->alloc);
                if (pushResult.hasErr()) {
                    freeMod();
                    return Error(AddEmptyModuleErr::OutOfMemory);
                }
            }
        } else {
            DynArrayUnmanaged<SemVer> versions;
            auto pushResult = versions.push(version, self->alloc);
            if (pushResult.hasErr()) {
                freeMod();
                return Error(AddEmptyModuleErr::OutOfMemory);
            }

            auto versionsInsertResult = self->versions.insert(self->alloc, name, std::move(versions));
            if (versionsInsertResult.hasErr()) {
                freeMod();
                versions.destroy(self->alloc);
                return Error(AddEmptyModuleErr::OutOfMemory);
            }
        }
    }

    ModuleVersion modver = {name, version};

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
        return Error(AddEmptyModuleErr::OutOfMemory);
    }

    sy_assert(moduleInsertResult.value().hasValue() == false, "Module version not added to versions list correctly");
    return mod;
}
