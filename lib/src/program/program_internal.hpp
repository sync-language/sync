#pragma once
#ifndef _SY_PROGRAM_PROGRAM_INTERNAL_HPP_
#define _SY_PROGRAM_PROGRAM_INTERNAL_HPP_

#include "program.hpp"

namespace sy {
    struct Bytecode;

    /// Extra metadata for script functions. 
    /// Corresponds with `SyFunction::fptr` if `SyFunction::tag == SyFunctionTypeScript`.
    struct InterpreterFunctionScriptInfo {
        const Program*  program;
        uint16_t        stackSpaceRequired;
        size_t          bytecodeCount;
        const Bytecode* bytecode;
    };
}

#endif // _SY_PROGRAM_PROGRAM_H_