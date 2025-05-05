#include "interpreter.hpp"
#include "../program/program.hpp"
#include "../program/program_internal.hpp"
#include "../util/assert.hpp"
#include "stack.hpp"
#include "bytecode.hpp"
#include "../types/function/function.hpp"

using sy::ProgramRuntimeError;

ProgramRuntimeError sy::interpreterExecuteFunction(const Function *scriptFunction, void *outReturnValue)
{
    sy_assert(scriptFunction->tag == Function::Type::Script, "Interpreter cannot immediately execute C functions currently");

    Stack& activeStack = Stack::getActiveStack();
    FrameGuard guard = activeStack.pushFunctionFrame(scriptFunction, outReturnValue);
    if(guard.checkOverflow()) {
        return ProgramRuntimeError::initStackOverflow();
    }

    return ProgramRuntimeError();
}