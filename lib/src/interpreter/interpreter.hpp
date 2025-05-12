#pragma once
#ifndef SY_INTERPRETER_INTERPRETER_HPP_
#define SY_INTERPRETER_INTERPRETER_HPP_

namespace sy {
    class ProgramRuntimeError;
    class Function;

    /// Begins execution of a function. Does not handle pushing the arguments of the function.
    /// The return of the call will be stored in `outReturnValue`, provided the function does return a value.
    ProgramRuntimeError interpreterExecuteFunction(const Function* scriptFunction, void* outReturnValue);
}



#endif // SY_INTERPRETER_INTERPRETER_HPP_