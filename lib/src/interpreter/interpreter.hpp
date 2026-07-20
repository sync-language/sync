#pragma once
#ifndef SY_INTERPRETER_INTERPRETER_HPP_
#define SY_INTERPRETER_INTERPRETER_HPP_

#include "../types/anyerror/anyerror.hpp"
#include "../types/result/result.hpp"

namespace sy {
class RawFunction;

/// Begins execution of a function. Does not handle pushing the arguments of the function.
/// The return of the call will be stored in `outReturnValue`, provided the function does return a value.
Result<void, AnyError> interpreterExecuteScriptFunction(const RawFunction* scriptFunction, void* outReturnValue);
} // namespace sy

#endif // SY_INTERPRETER_INTERPRETER_HPP_