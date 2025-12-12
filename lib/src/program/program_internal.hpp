#pragma once
#ifndef SY_PROGRAM_PROGRAM_INTERNAL_HPP_
#define SY_PROGRAM_PROGRAM_INTERNAL_HPP_

#include "../mem/protected_allocator.hpp"
#include "../types/array/dynamic_array.hpp"
#include "../types/array/slice.hpp"
#include "../types/function/function.hpp"
#include "../types/hash/map.hpp"
#include "../types/string/string.hpp"
#include "../types/type_info.hpp"
#include "program.hpp"

namespace sy {
struct Bytecode;

/// Extra metadata for script functions.
/// Corresponds with `SyFunction::fptr` if `SyFunction::tag == SyFunctionTypeScript`.
struct InterpreterFunctionScriptInfo {
    const Program* program;
    /// Less than or equal to `Stack::MAX_FRAME_LEN`
    uint16_t stackSpaceRequired;
    size_t bytecodeCount;
    const Bytecode* bytecode;
    /// When this script function is getting unwinded, it will unwind this array of slots in their specific order.
    /// Valid for [0, `unwindLen`).
    const int16_t* unwindSlots;
    /// Length of `unwindSlots`.
    uint16_t unwindLen;
};

struct ProgramModuleInternal {
    StringUnmanaged name{};
    SemVer version{};
    Function* allFunctions = nullptr;
    StringUnmanaged* allFunctionNames = nullptr;
    StringUnmanaged* allFunctionQualifiedNames = nullptr;
    InterpreterFunctionScriptInfo* allFunctionScriptInfo = nullptr;
    size_t allFunctionsLen = 0;
    Type* allTypes = nullptr;
    StringUnmanaged* allTypeNames = nullptr;
    StringUnmanaged* allTypeQualifiedNames = nullptr;
    size_t allTypesLen = 0;

    /// Allocates all required memory for future operations, but the allocated
    /// arrays are left in an uninitialized state.
    static Result<ProgramModuleInternal*, AllocErr> init(Allocator protAlloc, StringSlice name, SemVer version,
                                                         size_t functionCount, size_t structCount);

    Option<const Function*> getFunctionByQualifiedName(StringSlice qualifiedName) const noexcept;
};

struct ProgramInternal {
    ProtectedAllocator protAlloc{};
    ProgramModule* allModules = nullptr;
    size_t allModulesLen = 0;
    MapUnmanaged<StringSlice, DynArrayUnmanaged<ProgramModule*>> moduleVersions;
    ProgramErrorReporter errReporter = nullptr;
    void* errReporterArg = nullptr;
};

} // namespace sy

#endif // _SY_PROGRAM_PROGRAM_H_