#include "module.hpp"
#include "../types/string/string.hpp"
#include <new>

using namespace sy;

// Sync modules use semver https://semver.org/

struct ModuleImpl {
    Allocator alloc;
    StringUnmanaged name;
    SemVer version;
};

static void deleteImpl(void* impl) noexcept {
    ModuleImpl* self = reinterpret_cast<ModuleImpl*>(impl);
    Allocator alloc = self->alloc;
    self->name.destroy(alloc);
    self->~ModuleImpl();
    alloc.freeObject(self);
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
    }

    self->inner_ = reinterpret_cast<void*>(impl);
    return self;
}