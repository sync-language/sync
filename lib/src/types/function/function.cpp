#include "function.hpp"
#include "../type_info.hpp"
#include "../../util/assert.hpp"
#include "../../interpreter/stack.hpp"
#include "../../interpreter/interpreter.hpp"
#include "../../program/program_internal.hpp"
#include <utility>

using sy::c::SyFunction;
using sy::c::SyFunctionCallArgs;
using sy::c::SyType;
using sy::c::SyProgramRuntimeError;
using sy::Function;
using sy::Type;

extern "C" {
    SY_API SyFunctionCallArgs sy_function_start_call(const SyFunction* self) {
        SyFunctionCallArgs args = {0, 0, 0};
        args.func = self;
        return args;
    }

    SY_API bool sy_function_call_args_push(SyFunctionCallArgs* self, const void* argMem, const SyType* typeInfo) {
        const Function* function = reinterpret_cast<const Function*>(self->func);
        sy_assert(self->pushedCount < function->argsLen, "Cannot push more arguments than the function takes");
        sy_assert(function->tag == Function::Type::Script, "Can only do script function calling currently");

        const bool result = Stack::getActiveStack().pushScriptFunctionArg(
            argMem, reinterpret_cast<const Type*>(typeInfo), self->_offset);

        if(result == false) {
            return false;
        }

        const size_t roundedToMultipleOf8 = typeInfo->sizeType % 8 == 0 ? 
            typeInfo->sizeType : 
            typeInfo->sizeType + (8 - (typeInfo->sizeType % 8));
        const size_t slotsOccupied = roundedToMultipleOf8 / 8;
        sy_assert(slotsOccupied <= UINT16_MAX, "Cannot push argument. Too big");

        const size_t newOffset = self->_offset + slotsOccupied;
        sy_assert(newOffset <= UINT16_MAX, "Cannot push argument. Would overflow script stack");
        const sy::InterpreterFunctionScriptInfo* scriptInfo = 
            reinterpret_cast<const sy::InterpreterFunctionScriptInfo*>(function->fptr);
        sy_assert(newOffset <= scriptInfo->stackSpaceRequired, "Pushing argument would overflow this function's script stack");

        self->_offset = static_cast<uint16_t>(slotsOccupied);

        return true;
    }

    union ErrContain {
        sy::ProgramRuntimeError cppErr;
        SyProgramRuntimeError cErr;

        ErrContain() : cppErr() {}
    };

    SY_API SyProgramRuntimeError sy_function_call(SyFunctionCallArgs self, void* retDst) {
        Function::CallArgs callArgs = {self};
        ErrContain err;
        err.cppErr = callArgs.call(retDst);
        return err.cErr;
    }
}

bool sy::Function::CallArgs::push(const void *argMem, const Type *typeInfo)
{
    return c::sy_function_call_args_push(&this->info, argMem, reinterpret_cast<const SyType*>(typeInfo));
}

sy::ProgramRuntimeError sy::Function::CallArgs::call(void *retDst)
{
    const Function* function = reinterpret_cast<const Function*>(this->info.func);
    sy_assert(this->info.pushedCount == function->argsLen , "Did not push enough arguments for function");
    sy_assert(function->tag == Function::Type::Script, "Can only do script function calling currently");

    return interpreterExecuteFunction(function, retDst);
}

sy::Function::CallArgs sy::Function::startCall() const
{
    CallArgs callArgs = {{0, 0, 0}};
    callArgs.info.func = reinterpret_cast<const SyFunction*>(this);
    return callArgs;
}
