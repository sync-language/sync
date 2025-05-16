#include "program.h"
#include "program.hpp"
#include "../types/function/function.hpp"
#include "../util/assert.hpp"

sy::ProgramRuntimeError::ProgramRuntimeError()
{
    inner.kind = static_cast<SyProgramRuntimeErrorKind>(Kind::None);
}

sy::ProgramRuntimeError sy::ProgramRuntimeError::initStackOverflow()
{
    ProgramRuntimeError self;
    self.inner.kind = static_cast<SyProgramRuntimeErrorKind>(Kind::StackOverflow);
    return self;
}

sy::CallStack::CallStack(const Function *const *inFunctions, size_t inLen)
{
    if(inLen != 0) {
        sy_assert(inFunctions != nullptr, "Expected non-null pointer for non-zero call stack length");
    }
    this->inner.functions = reinterpret_cast<const SyFunction* const*>(inFunctions);
    this->inner.len = inLen;
}

const sy::Function *sy::CallStack::operator[](size_t idx) const
{
    sy_assert(idx < this->inner.len, "Index out of bounds");
    return reinterpret_cast<const Function*>(this->inner.functions[idx]);
}
