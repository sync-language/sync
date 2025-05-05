#include "interpreter.hpp"
#include "../program/program.hpp"
#include "../program/program_internal.hpp"
#include "../util/assert.hpp"
#include "stack.hpp"
#include "bytecode.hpp"
#include "../types/function/function.hpp"

using sy::ProgramRuntimeError;
using sy::Program;

static ProgramRuntimeError interpreterExecuteContinuous(const Program* program);
static ProgramRuntimeError interpreterExecuteOperation(const Program* program);

ProgramRuntimeError sy::interpreterExecuteFunction(const Function *scriptFunction, void *outReturnValue)
{
    sy_assert(scriptFunction->tag == Function::Type::Script, "Interpreter cannot immediately execute C functions currently");

    Stack& activeStack = Stack::getActiveStack();
    FrameGuard guard = activeStack.pushFunctionFrame(scriptFunction, outReturnValue);
    if(guard.checkOverflow()) {
        return ProgramRuntimeError::initStackOverflow();
    }

    const sy::InterpreterFunctionScriptInfo* scriptInfo
        = reinterpret_cast<const sy::InterpreterFunctionScriptInfo*>(scriptFunction->fptr);

    activeStack.setInstructionPointer(scriptInfo->bytecode);

    sy_assert(scriptInfo->program != nullptr, "Script function must have a valid program");

    const ProgramRuntimeError err = interpreterExecuteContinuous(scriptInfo->program);
    if(!err.ok()) {
        // todo unwind frame
    }

    return err;
}

static ProgramRuntimeError interpreterExecuteContinuous(const Program* program) {
    while(true) {
        const Bytecode ipBytecode = *Stack::getActiveStack().getInstructionPointer();
        const OpCode opcode = ipBytecode.getOpcode();
        const bool isReturn = opcode == OpCode::Return;

        const ProgramRuntimeError err = interpreterExecuteOperation(program);
        if(err.ok() || isReturn) {
            return err;
        }
    }
}

static ProgramRuntimeError interpreterExecuteOperation(const Program* program) {
    ptrdiff_t ipChange = 1;
    return ProgramRuntimeError();
}
