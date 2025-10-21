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
