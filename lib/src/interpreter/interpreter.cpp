#include "interpreter.hpp"
#include "../program/program.hpp"
#include "../program/program_internal.hpp"
#include "../util/assert.hpp"
#include "stack.hpp"
#include "bytecode.hpp"
#include "../types/function/function.hpp"
#include "../types/type_info.hpp"
#include "../util/unreachable.hpp"
#include <cstring>

using sy::ProgramRuntimeError;
using sy::Program;
using sy::Type;
using sy::Function;

static ProgramRuntimeError interpreterExecuteContinuous(const Program* program);
static ProgramRuntimeError interpreterExecuteOperation(const Program* program);
static void unwindStackFrame(const uint16_t* unwindSlots, const uint16_t len);

ProgramRuntimeError sy::interpreterExecuteScriptFunction(const Function *scriptFunction, void *outReturnValue)
{
    sy_assert(scriptFunction->tag == Function::CallType::Script, 
        "Interpreter can only start executing from script functions");
    if(scriptFunction->returnType != nullptr) {
        sy_assert(outReturnValue != nullptr, "Function returns a value, which cannot be safely ignored");
    } else {
        sy_assert(outReturnValue == nullptr, 
            "Function does not return a value, so no return value address should be used");
    }

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
    // Regardless of an error or not, the frame should be unwinded.
    unwindStackFrame(scriptInfo->unwindSlots, scriptInfo->unwindLen);

    return err;
    // `FrameGuard guard` goes out of scope here, popping the frame.
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

static void unwindStackFrame(const uint16_t* unwindSlots, const uint16_t len) {
    Stack& activeStack = Stack::getActiveStack();
    for(uint16_t i = 0; i < len; i++) {
        const Type* type = activeStack.typeAt(unwindSlots[i]);
        if(type == nullptr || type->optionalDestructor == nullptr) {
            continue;
        }

        sy_assert(type->tag != Type::Tag::Reference, "Cannot destruct reference types");

        type->destroyObject(activeStack.valueMemoryAt(unwindSlots[i]));
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


static void executeReturn(const Bytecode b)
{
    // No meaningful work needs to be done. Compiler should optimize this call away.
    #if _DEBUG
    const operands::Return operands = b.toOperands<operands::Return>();
    (void)operands;
    #endif // _DEBUG

    // Frame is automatically unwinded
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

    // Frame is automatically unwinded
}

static bool pushScriptFunctionArgs(const Function* function, const uint16_t argsCount, const uint16_t* argsSrc) {
    sy_assert(function->argsLen == argsCount, "Mismatched number of arguments passed to function");
    sy_assert(function->tag == Function::CallType::Script, 
        "Cannot push script function arguments to non scirpt function");
    
    Function::CallArgs callArgs = function->startCall();
    Stack& activeStack = Stack::getActiveStack();

    for(uint16_t i = 0; i < argsCount; i++) {
        const uint16_t argSrc = argsSrc[i];
        const Type* type = activeStack.typeAt(argSrc);
        sy_assert(type != nullptr, "Cannot push null type to function");
        if(callArgs.push(activeStack.valueMemoryAt(argSrc), type) == false) {
            return false;
        }
    }
    return true;
}

static ProgramRuntimeError performCall(const Function* function, void* retDst, const uint16_t argsCount, const uint16_t* argsSrc) {
    if(function->tag == Function::CallType::Script) {
        const bool success = pushScriptFunctionArgs(function, argsCount, argsSrc);
        if(!success) {
            return ProgramRuntimeError::initStackOverflow();
        }
        return sy::interpreterExecuteScriptFunction(function, retDst);
    } else {
        sy_assert(false, "Cannot handle C function calling currently");
    }
    
    unreachable();
    return ProgramRuntimeError();
}

static ProgramRuntimeError executeCallImmediateNoReturn(ptrdiff_t& ipChange, const Bytecode* bytecodes) {
    const operands::CallImmediateNoReturn operands = bytecodes[0].toOperands<operands::CallImmediateNoReturn>();

    const Function* function = *reinterpret_cast<const Function* const*>(&bytecodes[1]);
    const uint16_t* argsSrcs = reinterpret_cast<const uint16_t*>(&bytecodes[2]);

    ipChange = static_cast<ptrdiff_t>(operands::CallImmediateNoReturn::bytecodeUsed(operands.argCount));

    return performCall(function, nullptr, operands.argCount, argsSrcs);
}
