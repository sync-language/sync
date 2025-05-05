#pragma once
#ifndef SY_INTERPRETER_INTERPRETER_HPP_
#define SY_INTERPRETER_INTERPRETER_HPP_

namespace sy {
    class ProgramRuntimeError;
    class Function;

    ProgramRuntimeError interpreterExecuteFunction(const Function* scriptFunction, void* outReturnValue);
}



#endif // SY_INTERPRETER_INTERPRETER_HPP_