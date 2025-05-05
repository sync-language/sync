//! API
#pragma once
#ifndef _SY_PROGRAM_PROGRAM_HPP_
#define _SY_PROGRAM_PROGRAM_HPP_

#include "../core.h"

namespace sy {
    namespace c {
        #include "program.h"
    }

    class Function;

    class CallStack {
    public:
        CallStack(const Function* const* inFunctions, size_t inLen);
        size_t len() const { return this->inner.len; }
        const Function* operator[] (size_t idx) const;
    private:
        c::SyCallStack inner;
    };

    class Program {

    private:
        //c::SyProgram program;
    };

    class ProgramRuntimeError {
    public:

        enum class Kind : int32_t {
            None = c::syProgramRuntimeErrorKindNone,
            StackOverflow = c::syProgramRuntimeErrorKindStackOverflow,
        };

        /// Initializes as an Ok, meaning has no error.
        ProgramRuntimeError();

        static ProgramRuntimeError initStackOverflow();

        Kind kind() const { return static_cast<Kind>(this->inner.kind); }

        /// Checks if this runtime error is ok, as in not an error. Specifically if `this->kind() == Kind::None`.
        bool ok() const { return this->kind() == Kind::None; }

    private:
        c::SyProgramRuntimeError inner;
    };
}

#endif // _SY_PROGRAM_PROGRAM_H_