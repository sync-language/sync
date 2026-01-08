#include "interpreter.hpp"
#include "../core/core_internal.h"
#include "../program/program.hpp"
#include "../program/program_internal.hpp"
#include "../types/function/function.hpp"
#include "../types/type_info.hpp"
#include "bytecode.hpp"
#include "stack/stack.hpp"
#include <cstring>

using namespace sy;

enum class OkExecStatus { Continue, FunctionCall, Return };

static Result<OkExecStatus, ProgramError> interpreterExecuteContinuous(const Program* program);
static Result<OkExecStatus, ProgramError> interpreterExecuteOperation(const Program* program);
static void unwindStackFrame(const int16_t* unwindSlots, const uint16_t len);

static void setupFunctionStackFrame(const RawFunction* scriptFunction, void* outReturnValue) {
    sy_assert(scriptFunction->tag == FunctionType::Script,
              "Interpreter can only start executing from script functions");
    if (scriptFunction->returnType != nullptr) {
        sy_assert(outReturnValue != nullptr, "Function returns a value, which cannot be safely ignored");
    } else {
        sy_assert(outReturnValue == nullptr,
                  "Function does not return a value, so no return value address should be used");
    }

    Stack& activeStack = Stack::getActiveStack();
    activeStack.pushFunctionFrame(scriptFunction, outReturnValue);

    const sy::InterpreterFunctionScriptInfo* scriptInfo =
        reinterpret_cast<const sy::InterpreterFunctionScriptInfo*>(scriptFunction->fptr);
    activeStack.setInstructionPointer(scriptInfo->bytecode);
}

Result<void, ProgramError> sy::interpreterExecuteScriptFunction(const RawFunction* scriptFunction,
                                                                void* outReturnValue) {
    // Just setup the initial function call stack
    setupFunctionStackFrame(scriptFunction, outReturnValue);

    size_t depth = 1;
    while (depth > 0) {
        Stack& activeStack = Stack::getActiveStack();
        const sy::RawFunction* currentFunction = activeStack.getCurrentFunction().value();
        const sy::InterpreterFunctionScriptInfo* currentScriptInfo =
            reinterpret_cast<const sy::InterpreterFunctionScriptInfo*>(currentFunction->fptr);

        auto res = interpreterExecuteContinuous(currentScriptInfo->program);
        if (res.hasErr()) {
            while (depth > 0) {
                currentFunction = activeStack.getCurrentFunction().value();
                currentScriptInfo = reinterpret_cast<const sy::InterpreterFunctionScriptInfo*>(currentFunction->fptr);
                unwindStackFrame(currentScriptInfo->unwindSlots, currentScriptInfo->unwindLen);
                activeStack.popFrame();
                depth -= 1;
            }
            return Error(res.takeErr());
        }

        sy_assert(res.value() != OkExecStatus::Continue, "Continue should not have escaped to here");

        if (res.value() == OkExecStatus::Return) {
            unwindStackFrame(currentScriptInfo->unwindSlots, currentScriptInfo->unwindLen);
            activeStack.popFrame();
            depth -= 1;
        }

        if (res.value() == OkExecStatus::FunctionCall) {
            depth += 1;
        }
    }

    return {};
}

static Result<OkExecStatus, ProgramError> interpreterExecuteContinuous(const Program* program) {
    while (true) {
        auto execResult = interpreterExecuteOperation(program);
        if (execResult.hasValue() && execResult.value() == OkExecStatus::Continue) {
            // just keep executing bytecode
        } else {
            return execResult;
        }
    }
}

static void unwindStackFrame(const int16_t* unwindSlots, const uint16_t len) {
    Stack& activeStack = Stack::getActiveStack();
    for (uint16_t i = 0; i < len; i++) {
        const Type* type = activeStack.typeAt(unwindSlots[i]);
        if (type == nullptr || !type->destructor.hasValue()) {
            continue;
        }

        sy_assert(type->tag != Type::Tag::Reference, "Cannot destruct reference types");

        type->destroyObject(activeStack.frameValueAt<void>(unwindSlots[i]));
    }
}

static void executeReturn(const Bytecode b);
static void executeReturnValue(const Bytecode b);
static Result<void, ProgramError> executeCallImmediateNoReturn(ptrdiff_t& ipChange, const Bytecode* bytecodes);
static Result<void, ProgramError> executeCallSrcNoReturn(ptrdiff_t& ipChange, const Bytecode* bytecodes);
static Result<void, ProgramError> executeCallImmediateWithReturn(ptrdiff_t& ipChange, const Bytecode* bytecodes);
static Result<void, ProgramError> executeCallSrcWithReturn(ptrdiff_t& ipChange, const Bytecode* bytecodes);
static void executeLoadDefault(ptrdiff_t& ipChange, const Bytecode* bytecodes);
static void executeLoadImmediateScalar(ptrdiff_t& ipChange, const Bytecode* bytecodes);
static void executeMemsetUninitialized(const Bytecode bytecode);
static void executeSetType(ptrdiff_t& ipChange, const Bytecode* bytecodes);
static void executeSetNullType(const Bytecode bytecode);
static void executeJump(ptrdiff_t& ipChange, const Bytecode bytecode);
static void executeJumpIfFalse(ptrdiff_t& ipChange, const Bytecode bytecode);
static void executeDestruct(const Bytecode bytecode);

static Result<OkExecStatus, ProgramError> interpreterExecuteOperation(const Program* program) {
    (void)program;

    ptrdiff_t ipChange = 1;
    const Bytecode* instructionPointer = Stack::getActiveStack().getInstructionPointer();
    const OpCode opcode = instructionPointer->getOpcode();

    auto result = [opcode, instructionPointer, &ipChange]() -> Result<OkExecStatus, ProgramError> {
        switch (opcode) {
        case OpCode::Noop:
            break;
        case OpCode::Return: {
            executeReturn(*instructionPointer);
            return OkExecStatus::Return;
        } break;
        case OpCode::ReturnValue: {
            executeReturnValue(*instructionPointer);
            return OkExecStatus::Return;
        } break;
        case OpCode::CallImmediateNoReturn: {
            if (auto callRes = executeCallImmediateNoReturn(ipChange, instructionPointer); callRes.hasErr()) {
                return Error(callRes.takeErr());
            }
            return OkExecStatus::FunctionCall;
        } break;
        case OpCode::CallSrcNoReturn: {
            if (auto callRes = executeCallSrcNoReturn(ipChange, instructionPointer); callRes.hasErr()) {
                return Error(callRes.takeErr());
            }
            return OkExecStatus::FunctionCall;
        } break;
        case OpCode::CallImmediateWithReturn: {
            if (auto callRes = executeCallImmediateWithReturn(ipChange, instructionPointer); callRes.hasErr()) {
                return Error(callRes.takeErr());
            }
            return OkExecStatus::FunctionCall;
        } break;
        case OpCode::CallSrcWithReturn: {
            if (auto callRes = executeCallSrcWithReturn(ipChange, instructionPointer); callRes.hasErr()) {
                return Error(callRes.takeErr());
            }
            return OkExecStatus::FunctionCall;
        } break;
        case OpCode::LoadDefault: {
            executeLoadDefault(ipChange, instructionPointer);
        } break;
        case OpCode::LoadImmediateScalar: {
            executeLoadImmediateScalar(ipChange, instructionPointer);
        } break;
        case OpCode::MemsetUninitialized: {
            executeMemsetUninitialized(*instructionPointer);
        } break;
        case OpCode::SetType: {
            executeSetType(ipChange, instructionPointer);
        } break;
        case OpCode::SetNullType: {
            executeSetNullType(*instructionPointer);
        } break;
        case OpCode::Jump: {
            executeJump(ipChange, *instructionPointer);
        } break;
        case OpCode::JumpIfFalse: {
            executeJumpIfFalse(ipChange, *instructionPointer);
        } break;
        case OpCode::Destruct: {
            executeDestruct(*instructionPointer);
        } break;

        default: {
            sy_assert(static_cast<uint8_t>(opcode) && false, "Unimplemented opcode");
        }
        }
        return OkExecStatus::Continue;
    }();

    if (result.hasValue() && result.value() == OkExecStatus::Continue) {
        Stack::getActiveStack().setInstructionPointer(instructionPointer + ipChange);
    }

    return result;
}

static void executeReturn(const Bytecode b) {
// No meaningful work needs to be done. Compiler should optimize this call away.
#ifndef NDEBUG
    const operators::Return operands = b.toOperands<operators::Return>();
    (void)operands;
#else
    (void)b;
#endif // NDEBUG

    // Frame is automatically unwinded
}

static void executeReturnValue(const Bytecode b) {
    const operators::ReturnValue operands = b.toOperands<operators::ReturnValue>();

    Stack& activeStack = Stack::getActiveStack();

    void* retDst = activeStack.returnDst();
    sy_assert(retDst != nullptr, "Cannot assign return value to null memory");

    const Type* retValType = activeStack.typeAt(operands.src);
    sy_assert(retValType != nullptr, "Cannot return null type");

    std::memcpy(retDst, activeStack.frameValueAt<void>(operands.src), retValType->sizeType);

    // Frame is automatically unwinded
}

static bool pushScriptFunctionArgs(const RawFunction* function, const uint16_t argsCount, const uint16_t* argsSrc) {
    sy_assert(function->argsLen == argsCount, "Mismatched number of arguments passed to function");
    sy_assert(function->tag == FunctionType::Script, "Cannot push script function arguments to non scirpt function");

    RawFunction::CallArgs callArgs = function->startCall();
    Stack& activeStack = Stack::getActiveStack();

    for (uint16_t i = 0; i < argsCount; i++) {
        const uint16_t argSrc = argsSrc[i];
        const Type* type = activeStack.typeAt(argSrc);
        sy_assert(type != nullptr, "Cannot push null type to function");
        if (callArgs.push(activeStack.frameValueAt<void>(argSrc), type) == false) {
            return false;
        }
    }
    return true;
}

static Result<void, ProgramError> setupInterpreterNestedCall(const RawFunction* function, void* retDst,
                                                             const uint16_t argsCount, const uint16_t* argsSrc) {
    if (function->tag == FunctionType::Script) {
        (void)pushScriptFunctionArgs(function, argsCount, argsSrc);
        setupFunctionStackFrame(function, retDst);
    } else {
        sy_assert(false, "Cannot handle C function calling currently");
    }

    return {};
}

static Result<void, ProgramError> executeCallImmediateNoReturn(ptrdiff_t& ipChange, const Bytecode* bytecodes) {
    const operators::CallImmediateNoReturn operands = bytecodes[0].toOperands<operators::CallImmediateNoReturn>();

    Stack& activeStack = Stack::getActiveStack();

    const RawFunction* function = *reinterpret_cast<const RawFunction* const*>(&bytecodes[1]);
    const uint16_t* argsSrcs = reinterpret_cast<const uint16_t*>(&bytecodes[2]);

    ipChange = static_cast<ptrdiff_t>(operators::CallImmediateNoReturn::bytecodeUsed(operands.argCount));

    activeStack.setInstructionPointer(activeStack.getInstructionPointer() + ipChange);

    return setupInterpreterNestedCall(function, nullptr, operands.argCount, argsSrcs);
}

static Result<void, ProgramError> executeCallSrcNoReturn(ptrdiff_t& ipChange, const Bytecode* bytecodes) {
    const operators::CallSrcNoReturn operands = bytecodes[0].toOperands<operators::CallSrcNoReturn>();

    Stack& activeStack = Stack::getActiveStack();

    sy_assert(activeStack.typeAt(operands.src).get()->tag == Type::Tag::Function, "Expected function to call");
    const RawFunction* function = activeStack.frameValueAt<const RawFunction>(operands.src);
    const uint16_t* argsSrcs = reinterpret_cast<const uint16_t*>(&bytecodes[1]);

    ipChange = static_cast<ptrdiff_t>(operators::CallSrcNoReturn::bytecodeUsed(operands.argCount));

    activeStack.setInstructionPointer(activeStack.getInstructionPointer() + ipChange);

    return setupInterpreterNestedCall(function, nullptr, operands.argCount, argsSrcs);
}

static Result<void, ProgramError> executeCallImmediateWithReturn(ptrdiff_t& ipChange, const Bytecode* bytecodes) {
    const operators::CallImmediateWithReturn operands = bytecodes[0].toOperands<operators::CallImmediateWithReturn>();

    const RawFunction* function = *reinterpret_cast<const RawFunction* const*>(&bytecodes[1]);
    const uint16_t* argsSrcs = reinterpret_cast<const uint16_t*>(&bytecodes[2]);

    ipChange = static_cast<ptrdiff_t>(operators::CallImmediateWithReturn::bytecodeUsed(operands.argCount));

    Stack& activeStack = Stack::getActiveStack();
    void* returnDst = activeStack.frameValueAt<void>(operands.retDst);

    activeStack.setInstructionPointer(activeStack.getInstructionPointer() + ipChange);

    return setupInterpreterNestedCall(function, returnDst, operands.argCount, argsSrcs);
}

static Result<void, ProgramError> executeCallSrcWithReturn(ptrdiff_t& ipChange, const Bytecode* bytecodes) {
    const operators::CallSrcWithReturn operands = bytecodes[0].toOperands<operators::CallSrcWithReturn>();

    Stack& activeStack = Stack::getActiveStack();

    sy_assert(activeStack.typeAt(operands.src).get()->tag == Type::Tag::Function, "Expected function to call");
    const RawFunction* function = activeStack.frameValueAt<const RawFunction>(operands.src);
    const uint16_t* argsSrcs = reinterpret_cast<const uint16_t*>(&bytecodes[1]);

    ipChange = static_cast<ptrdiff_t>(operators::CallSrcWithReturn::bytecodeUsed(operands.argCount));

    void* returnDst = activeStack.frameValueAt<void>(operands.retDst);

    activeStack.setInstructionPointer(activeStack.getInstructionPointer() + ipChange);

    return setupInterpreterNestedCall(function, returnDst, operands.argCount, argsSrcs);
}

void executeLoadDefault(ptrdiff_t& ipChange, const Bytecode* bytecodes) {
    const operators::LoadDefault operands = bytecodes[0].toOperands<operators::LoadDefault>();

    Stack& activeStack = Stack::getActiveStack();
    void* destination = activeStack.frameValueAt<void>(operands.dst);

    if (operands.isScalar) {
        const Type* scalarType = scalarTypeFromTag(static_cast<ScalarTag>(operands.scalarTag));
        memset(destination, 0, scalarType->sizeType);
    } else {
        (void)ipChange;
        // TODO call default constructors or default initializer
        sy_assert(false, "Cannot load default for non-scalar types currently");
    }
}

void executeLoadImmediateScalar(ptrdiff_t& ipChange, const Bytecode* bytecodes) {
    const operators::LoadImmediateScalar operands = bytecodes[0].toOperands<operators::LoadImmediateScalar>();
    const ScalarTag scalarTag = static_cast<ScalarTag>(operands.scalarTag);

    Stack& activeStack = Stack::getActiveStack();
    void* destination = activeStack.frameValueAt<void>(operands.dst);
    const Type* type = scalarTypeFromTag(scalarTag);
    if (type->sizeType <= 4) { // 32 bits
        uint32_t rawValue = static_cast<uint32_t>(operands.immediate);
        *reinterpret_cast<uint32_t*>(destination) = rawValue;
    } else {
        // All scalar types have alignment less than or equal to alignof(Bytecode), so this is fine.
        sy_assert(type->alignType <= alignof(Bytecode),
                  "Scalar types must have less than or equal alignment to Bytecode");
        const void* valueMemory = reinterpret_cast<const void*>(&bytecodes[1]);
        memcpy(destination, valueMemory, type->sizeType);

        ipChange = operators::LoadImmediateScalar::bytecodeUsed(scalarTag);

        // TODO how to load immediate string objects? not string slices
    }
}

void executeMemsetUninitialized(const Bytecode bytecode) {
    const operators::MemsetUninitialized operands = bytecode.toOperands<operators::MemsetUninitialized>();

    Stack& activeStack = Stack::getActiveStack();
    void* destination = activeStack.frameValueAt<void>(operands.dst);
#ifndef NDEBUG
    const uint32_t frameLength = activeStack.getCurrentFrame().value().frameLength;
    sy_assert(frameLength >= (static_cast<uint64_t>(operands.dst) + static_cast<uint64_t>(operands.slots)),
              "Trying to uninitialize memory outside of stack frame");
#endif
    const size_t bytesToSet = sizeof(void*) * static_cast<size_t>(operands.slots);
    memset(destination, 0xAA, bytesToSet);
}

void executeSetType(ptrdiff_t& ipChange, const Bytecode* bytecodes) {
    const operators::SetType operands = bytecodes[0].toOperands<operators::SetType>();

    Stack& activeStack = Stack::getActiveStack();
    const Type* type = nullptr;
    if (operands.isScalar) {
        type = scalarTypeFromTag(static_cast<ScalarTag>(operands.scalarTag));
    } else {
        const Bytecode next = bytecodes[1];
        type = *reinterpret_cast<const Type* const*>(&next);
        ipChange = 2;
    }
    activeStack.setTypeAt(Node::TypeOfValue(type, true), operands.dst);
}

void executeSetNullType(const Bytecode bytecode) {
    const operators::SetNullType operands = bytecode.toOperands<operators::SetNullType>();
    Stack& activeStack = Stack::getActiveStack();
    activeStack.setTypeAt(nullptr, operands.dst);
}

static void executeJump(ptrdiff_t& ipChange, const Bytecode bytecode) {
    const operators::Jump operands = bytecode.toOperands<operators::Jump>();
    int32_t jumpAmount = static_cast<int32_t>(operands.amount);
    ipChange = jumpAmount;
}

static void executeJumpIfFalse(ptrdiff_t& ipChange, const Bytecode bytecode) {
    const operators::JumpIfFalse operands = bytecode.toOperands<operators::JumpIfFalse>();

    Stack& activeStack = Stack::getActiveStack();
    const Type* srcType = activeStack.typeAt(operands.src);
    sy_assert(srcType == Type::TYPE_BOOL, "Can only conditionally jump on boolean types");

    if (*activeStack.frameValueAt<bool>(operands.src) == false) {
        int32_t jumpAmount = static_cast<int32_t>(operands.amount);
        ipChange = jumpAmount;
    }
}

static void executeDestruct(const Bytecode bytecode) {
    const operators::Destruct operands = bytecode.toOperands<operators::Destruct>();

    Stack& activeStack = Stack::getActiveStack();

    const Type* srcType = activeStack.typeAt(operands.src);
    sy_assert(srcType != nullptr, "Cannot destruct null typed object");

    void* src = activeStack.frameValueAt<void>(operands.src);
    srcType->destroyObject(src);
    activeStack.setTypeAt(nullptr, operands.src);
}
