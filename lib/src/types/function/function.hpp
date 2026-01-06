//! API
#pragma once
#ifndef SY_TYPES_FUNCTION_FUNCTION_HPP_
#define SY_TYPES_FUNCTION_FUNCTION_HPP_

#include "../../core/core.h"
#include "../../program/program_error.hpp"
#include "../result/result.hpp"
#include "../string/string_slice.hpp"
#include "../task/task.hpp"
#include "function_align.h"
#include <utility>

namespace sy {
class Type;
class ProgramRuntimeError;
class RawFunction;

enum class FunctionType : int32_t {
    C = 0,
    Script = 1,
};

class FunctionHandler {
    friend class RawFunction;

  public:
    /// Moves a function argument out of the argument buffer, taking ownership of it.
    /// Arguments are numbered and ordered, starting at 0.
    /// # Debug Assert
    /// `argIndex` must be within the bounds of the amount of arguments [0, arg count).
    /// The type `T` must be the correct type (size and alignment).
    template <typename T> T&& takeArg(size_t argIndex) {
        void* argMem = getArgMem(argIndex);
#ifndef NDEBUG
        validateArgTypeMatches(argMem, getArgType(argIndex), sizeof(T), alignof(T));
#endif
        return std::move(*reinterpret_cast<T*>(argMem));
    }

    /// Sets the return value of the function. This cannot be called multiple times.
    /// # Debug Asserts
    /// The function should return a value.
    /// The actual return destination is correctly aligned to `alignof(T)`.
    template <typename T> void setReturn(T&& retValue) {
        void* retDst = this->getRetDst();
#ifndef NDEBUG
        validateReturnDstAligned(retDst, alignof(T));
#endif
        T& asRef = *reinterpret_cast<T*>(retDst);
        asRef = std::move(retValue);
    }

  private:
    FunctionHandler(uint32_t index) : handle(index) {}

    void* getArgMem(size_t argIndex);
    const Type* getArgType(size_t argIndex);
    void validateArgTypeMatches(void* arg, const Type* storedType, size_t sizeType, size_t alignType);
    void* getRetDst();
    void validateReturnDstAligned(void* retDst, size_t alignType);

    uint32_t handle;
};

using c_function_t = Result<void, ProgramError> (*)(FunctionHandler);

class RawFunction {
  public:
    struct CallArgs {
        const RawFunction* func;
        uint16_t pushedCount;
        /// Internal use only.
        uint16_t _offset;

        /// Pushs an argument onto the the script or C stack for the next function call.
        /// @return `true` if the push was successful, or `false`, if the stack would overflow by pushing the argument.
        bool push(void* argMem, const Type* typeInfo);

        Result<void, ProgramError> call(void* retDst);

        Result<RawTask, ProgramError> callParallel();
    };

    CallArgs startCall() const;

    /// Un-namespaced name. For example if `qualifiedName == "example.func"` then `name == "func"`.
    StringSlice name;
    /// As fully qualified name, namespaced.
    StringSlice qualifiedName;
    const Type* returnType;
    const Type** argsTypes;
    uint16_t argsLen;
    uint16_t alignment = SY_FUNCTION_MIN_ALIGN;
    /// If `true`, this function can be called in a comptime context within Sync source code.
    bool comptimeSafe;
    FunctionType tag;
    /// Both for C functions and script functions. Given `tag` and `info`, the function will be correctly called.
    /// For C functions, this should be a function with the signature of `Function::c_function_t`.
    const void* fptr;
};
} // namespace sy

#endif // SY_TYPES_FUNCTION_FUNCTION_HPP_