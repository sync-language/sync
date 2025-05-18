#pragma once
#ifndef _SY_PROGRAM_PROGRAM_INTERNAL_HPP_
#define _SY_PROGRAM_PROGRAM_INTERNAL_HPP_

#include "program.hpp"

struct Bytecode;

namespace sy {

    /// Extra metadata for script functions. 
    /// Corresponds with `SyFunction::fptr` if `SyFunction::tag == SyFunctionTypeScript`.
    struct InterpreterFunctionScriptInfo {
        const Program*  program;
        /// Less than or equal to `Stack::MAX_FRAME_LEN`
        size_t          stackSpaceRequired;
        size_t          bytecodeCount;
        const Bytecode* bytecode;
        /// When this script function is getting unwinded, it will unwind this array of slots in their specific order.
        /// Valid for [0, `unwindLen`).
        const uint16_t* unwindSlots;
        /// Length of `unwindSlots`.
        size_t          unwindLen;
    };
}

#endif // _SY_PROGRAM_PROGRAM_H_