#pragma once
#ifndef SY_PROGRAM_PROGRAM_INTERNAL_HPP_
#define SY_PROGRAM_PROGRAM_INTERNAL_HPP_

#include "../mem/protected_allocator.hpp"
#include "../types/array/dynamic_array.hpp"
#include "../types/array/slice.hpp"
#include "../types/function/function.hpp"
#include "../types/string/string.hpp"
#include "../types/type_info.hpp"
#include "program.hpp"

struct Bytecode;

namespace sy {

/// Extra metadata for script functions.
/// Corresponds with `SyFunction::fptr` if `SyFunction::tag == SyFunctionTypeScript`.
struct InterpreterFunctionScriptInfo {
    const Program* program;
    /// Less than or equal to `Stack::MAX_FRAME_LEN`
    uint32_t stackSpaceRequired;
    size_t bytecodeCount;
    const Bytecode* bytecode;
    /// When this script function is getting unwinded, it will unwind this array of slots in their specific order.
    /// Valid for [0, `unwindLen`).
    const uint16_t* unwindSlots;
    /// Length of `unwindSlots`.
    uint16_t unwindLen;
};

struct ProgramModuleInternal {
    StringUnmanaged name{};
    SemVer version{};
    Function* allFunctions = nullptr;
    size_t allFunctionsLen = 0;
    Type* allTypes = nullptr;
    size_t allTypesLen = 0;
};

struct ProgramInternal {
    ProtectedAllocator protAlloc{};
    ProgramModuleInternal* allModules = nullptr;
    size_t allModulesLen = 0;
};

} // namespace sy

#endif // _SY_PROGRAM_PROGRAM_H_