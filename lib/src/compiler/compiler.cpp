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
        return this->name == other.name && this->version.major == other.version.major &&
               this->version.minor == other.version.minor && this->version.patch == other.version.patch;
    }
};

namespace std {
template <> struct hash<ModuleVersion> {
    size_t operator()(const ModuleVersion& obj) { return obj.name.hash(); }
};
} // namespace std

struct CompilerImpl {
    Allocator alloc;
    MapUnmanaged<ModuleVersion, Module*> modules;
};

static void deleteImpl(void* impl) noexcept {
    CompilerImpl* self = reinterpret_cast<CompilerImpl*>(impl);
    Allocator alloc = self->alloc;
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
        memset(impl, 0, sizeof(CompilerImpl));

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

Result<Module*, AllocErr> sy::Compiler::addEmptyModule(StringSlice name, SemVer version) noexcept {
    (void)name;
    (void)version;
    return Error(AllocErr::OutOfMemory);
}
