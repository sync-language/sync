#include "interpreter.hpp"
#include "../program/program.hpp"
#include "../program/program_internal.hpp"
#include "../util/assert.hpp"
#include "stack.hpp"
#include "bytecode.hpp"
#include "../types/function/function.hpp"
#include "../types/type_info.hpp"
#include <cstring>

using sy::ProgramRuntimeError;
using sy::Program;
using sy::Type;
using sy::Function;

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

static void executeReturn(const Bytecode b);
static void executeReturnValue(const Bytecode b);
static ProgramRuntimeError executeCallImmediateNoReturn(ptrdiff_t& ipChange, const Bytecode* bytecodes);

static ProgramRuntimeError interpreterExecuteOperation(const Program* program) {
    (void)program;

    ptrdiff_t ipChange = 1;
    const Bytecode* instructionPointer = Stack::getActiveStack().getInstructionPointer();
    const OpCode opcode = instructionPointer->getOpcode();

    ProgramRuntimeError potentialErr{}; // Initialize as ok
    switch(opcode) {
        case OpCode::Nop: break;
        case OpCode::Return: {
            executeReturn(*instructionPointer);
        } break;
        case OpCode::ReturnValue: {
            executeReturnValue(*instructionPointer);
        } break;
        case OpCode::CallImmediateNoReturn: {
            executeCallImmediateNoReturn(ipChange, instructionPointer);
        } break;

        default: {
            sy_assert(static_cast<uint8_t>(opcode) && false, "Unimplemented opcode");
        }
    }
    return potentialErr;
}

void executeReturn(const Bytecode b)
{
    const operands::Return operands = b.toOperands<operands::Return>();
    (void)operands;

    // todo unwind frame
}

static void executeReturnValue(const Bytecode b)
{
    const operands::ReturnValue operands = b.toOperands<operands::ReturnValue>();

    Stack& activeStack = Stack::getActiveStack();

    void* retDst = activeStack.returnDst();
    sy_assert(retDst != nullptr, "Cannot assign return value to null memory");

    const Type* retValType = activeStack.typeAt(operands.src);
    sy_assert(retValType != nullptr, "Cannot return null type");

    std::memcpy(retDst, activeStack.valueMemoryAt(operands.src), retValType->sizeType);

    // todo unwind frame
}

static ProgramRuntimeError performCall(const Function* function, void* retDst, const uint16_t argsCount, const uint16_t* argsSrc) {
    // todo
    sy_assert(false, "todo");

    (void)function;
    (void)retDst;
    (void)argsCount;
    (void)argsSrc;
    return ProgramRuntimeError();
}

static ProgramRuntimeError executeCallImmediateNoReturn(ptrdiff_t& ipChange, const Bytecode* bytecodes) {
    const operands::CallImmediateNoReturn operands = bytecodes[0].toOperands<operands::CallImmediateNoReturn>();

    const Function* function = *reinterpret_cast<const Function* const*>(&bytecodes[1]);
    const uint16_t* argsSrcs = reinterpret_cast<const uint16_t*>(&bytecodes[2]);

    ipChange = static_cast<ptrdiff_t>(operands::CallImmediateNoReturn::bytecodeUsed(operands.argCount));

    return performCall(function, nullptr, operands.argCount, argsSrcs);
}
