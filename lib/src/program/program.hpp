//! API
#pragma once
#ifndef _SY_PROGRAM_PROGRAM_HPP_
#define _SY_PROGRAM_PROGRAM_HPP_

#include "../core.h"

namespace sy {
    class Function;

    class CallStack {
    public:
        CallStack(const Function* const* inFunctions, size_t inLen);
        size_t len() const { return this->_len; }
        const Function* operator[] (size_t idx) const;
    private:
        const Function* const*  _functions;
        size_t                  _len;
    };

    class Program {

    private:
        void* _inner;
    };

    class ProgramRuntimeError {
    public:

        enum class Kind : int32_t {
            None = 0,
            StackOverflow = 1,
        };

        /// Initializes as an Ok, meaning has no error.
        ProgramRuntimeError();

        static ProgramRuntimeError initStackOverflow();

        Kind kind() const { return static_cast<Kind>(this->_kind); }

        /// Checks if this runtime error is ok, as in not an error. Specifically if `this->kind() == Kind::None`.
        bool ok() const { return this->kind() == Kind::None; }

    private:
        Kind    _kind;
        void*   _inner;
    };
}

#endif // _SY_PROGRAM_PROGRAM_H_