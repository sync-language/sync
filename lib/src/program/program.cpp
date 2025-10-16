#include "program.h"
#include "../types/function/function.hpp"
#include "../util/assert.hpp"
#include "program.hpp"

using sy::Program;
using sy::ProgramRuntimeError;

static_assert(sizeof(Program) == sizeof(SyProgram));
static_assert(sizeof(SyProgramRuntimeErrorKind) == sizeof(int));
static_assert(sizeof(sy::CallStack) == sizeof(SyCallStack));

sy::CallStack::CallStack(const Function* const* inFunctions, size_t inLen) : _functions(inFunctions), _len(inLen) {
    if (inLen != 0) {
        sy_assert(inFunctions != nullptr, "Expected non-null pointer for non-zero call stack length");
    }
}

const sy::Function* sy::CallStack::operator[](size_t idx) const {
    sy_assert(idx < this->_len, "Index out of bounds");
    return reinterpret_cast<const Function*>(this->_functions[idx]);
}
