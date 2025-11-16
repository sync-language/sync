#include "program.h"
#include "../types/function/function.hpp"
#include "../util/assert.hpp"
#include "program.hpp"
#include "program_internal.hpp"

using namespace sy;

static_assert(sizeof(Program) == sizeof(SyProgram));
static_assert(sizeof(SyProgramRuntimeErrorKind) == sizeof(int));
static_assert(sizeof(CallStack) == sizeof(SyCallStack));

sy::CallStack::CallStack(const Function* const* inFunctions, size_t inLen) : _functions(inFunctions), _len(inLen) {
    if (inLen != 0) {
        sy_assert(inFunctions != nullptr, "Expected non-null pointer for non-zero call stack length");
    }
}

const sy::Function* sy::CallStack::operator[](size_t idx) const {
    sy_assert(idx < this->_len, "Index out of bounds");
    return reinterpret_cast<const Function*>(this->_functions[idx]);
}

ModuleVersion sy::ProgramModule::moduleInfo() const noexcept {
    const ProgramModuleInternal* self = reinterpret_cast<const ProgramModuleInternal*>(this->inner_);
    return ModuleVersion{self->name.asSlice(), self->version};
}

Option<const Function*> sy::ProgramModule::getFunctionByQualifiedName(StringSlice qualifiedName) const noexcept {
    const ProgramModuleInternal* self = reinterpret_cast<const ProgramModuleInternal*>(this->inner_);
    return self->getFunctionByQualifiedName(qualifiedName);
}

Option<const ProgramModule&> sy::Program::getModule(StringSlice name, Option<SemVer> version) const noexcept {
    const ProgramInternal* self = reinterpret_cast<const ProgramInternal*>(this->inner_);
    auto findVersions = self->moduleVersions.find(name);
    if (findVersions.hasValue() == false) {
        return std::nullopt;
    }

    sy_assert(findVersions.value().len() > 0, "Zero versions??");

    if (version.hasValue() == false) {
        return Option<const ProgramModule&>(*findVersions.value()[0]);
    } else {
        SemVer desired = version.value();
        for (const ProgramModule* versionedModule : findVersions.value()) {
            if (versionedModule->moduleInfo().version == desired) {
                return Option<const ProgramModule&>(*versionedModule);
            }
        }
        return std::nullopt;
    }
}

Result<ProgramModuleInternal*, AllocErr> sy::ProgramModuleInternal::init(Allocator protAlloc, StringSlice name,
                                                                         SemVer version, size_t functionCount,
                                                                         size_t structCount) {
    //! If memory allocation fails, it is ok as the `protAlloc` will free the memory itself

    ProgramModuleInternal* self = nullptr;

    auto res = protAlloc.allocObject<ProgramModuleInternal>();
    if (res.hasErr())
        return Error(AllocErr::OutOfMemory);

    self = res.value();
    new (self) ProgramModuleInternal();

    auto nameCopyRes = StringUnmanaged::copyConstructSlice(name, protAlloc);
    if (nameCopyRes.hasErr())
        return Error(AllocErr::OutOfMemory);

    new (&self->name) StringUnmanaged(std::move(nameCopyRes.takeValue()));
    self->version = version;

    if (functionCount > 0) {
        self->allFunctionsLen = functionCount;
        auto funcMemRes = protAlloc.allocArray<Function>(functionCount);
        if (funcMemRes.hasErr())
            return Error(AllocErr::OutOfMemory);
        auto funcNameRes = protAlloc.allocArray<StringUnmanaged>(functionCount);
        if (funcNameRes.hasErr())
            return Error(AllocErr::OutOfMemory);
        auto funcQualifiedNameRes = protAlloc.allocArray<StringUnmanaged>(functionCount);
        if (funcQualifiedNameRes.hasErr())
            return Error(AllocErr::OutOfMemory);

        self->allFunctions = funcMemRes.value();
        self->allFunctionNames = funcNameRes.value();
        self->allFunctionQualifiedNames = funcQualifiedNameRes.value();
    }

    if (structCount > 0) {
        self->allTypesLen = structCount;
        auto typeMemRes = protAlloc.allocArray<Type>(structCount);
        if (typeMemRes.hasErr())
            return Error(AllocErr::OutOfMemory);
        auto typeNameRes = protAlloc.allocArray<StringUnmanaged>(structCount);
        if (typeNameRes.hasErr())
            return Error(AllocErr::OutOfMemory);
        auto typeQualifiedNameRes = protAlloc.allocArray<StringUnmanaged>(structCount);
        if (typeQualifiedNameRes.hasErr())
            return Error(AllocErr::OutOfMemory);

        self->allTypes = typeMemRes.value();
        self->allTypeNames = typeNameRes.value();
        self->allTypeQualifiedNames = typeQualifiedNameRes.value();
    }

    return self;
}

Option<const Function*>
sy::ProgramModuleInternal::getFunctionByQualifiedName(StringSlice qualifiedName) const noexcept {
    // TODO optimize this to use a map or something
    for (size_t i = 0; i < this->allFunctionsLen; i++) {
        if (this->allFunctionQualifiedNames[i].asSlice() == qualifiedName) {
            return Option<const Function*>(&this->allFunctions[i]);
        }
    }
    return {};
}
