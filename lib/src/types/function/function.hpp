//! API
#pragma once
#ifndef SY_TYPES_FUNCTION_FUNCTION_HPP_
#define SY_TYPES_FUNCTION_FUNCTION_HPP_

#include "../../core/core.h"
#include "../../program/program_error.hpp"
#include "../reflect_fwd.hpp"
#include "../result/result.hpp"
#include "../string/string_slice.hpp"
#include "../task/task.hpp"
#include "function_align.h"
#include <type_traits>
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

    /// Returns the `RawFunction` currently being invoked. Useful for trampolines that need to
    /// recover per-instance state stored on the `RawFunction` itself (e.g. `innerFn`).
    const RawFunction* function() const noexcept;

  private:
    FunctionHandler(uint32_t index) : handle(index) {}

    void* getArgMem(size_t argIndex);
    const Type* getArgType(size_t argIndex);
    void validateArgTypeMatches(void* arg, const Type* storedType, size_t sizeType,
                                size_t alignType);
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

        ~CallArgs() noexcept;

        /// Pushs an argument onto the the script or C stack for the next function call.
        /// @return `true` if the push was successful, or `false`, if the stack would overflow by
        /// pushing the argument.
        bool push(void* argMem, const Type* typeInfo);

        Result<void, ProgramError> call(void* retDst) noexcept;

        Result<RawTask, ProgramError> callParallel() noexcept;
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
    /// Both for C functions and script functions. Given `tag` and `info`, the function will be
    /// correctly called. For C functions, this should be a function with the signature of
    /// `Function::c_function_t`.
    void* fptr;
    /// Used only for the C++ (probably Rust and Zig) APIs to enable simpler usage. Can be nullptr
    /// most of the time.
    void* innerFn = nullptr;
};

template <typename Sig> class Function;

template <typename Ret, typename... Args> class Function<Ret(Args...)> {
  public:
    using NormalFn = Ret (*)(Args...);
    using FallibleFn = Result<Ret, ProgramError> (*)(Args...);

    Function(NormalFn fn) noexcept {
        initRaw();
        raw_.fptr = reinterpret_cast<void*>(&infallibleTrampoline);
        raw_.innerFn = reinterpret_cast<void*>(fn);
    }

    Function(FallibleFn fn) noexcept {
        initRaw();
        raw_.fptr = reinterpret_cast<void*>(&fallibleTrampoline);
        raw_.innerFn = reinterpret_cast<void*>(fn);
    }

    Result<Ret, ProgramError> call(Args... args) const noexcept {
        RawFunction::CallArgs callArgs = raw_.startCall();
        const bool pushedAll =
            (... && callArgs.push(const_cast<void*>(static_cast<const void*>(&args)),
                                  Reflect<Args>::get()));
        if (!pushedAll) {
            return Error(ProgramError::BufferTooSmall);
        }
        if constexpr (std::is_void_v<Ret>) {
            return callArgs.call(nullptr);
        } else {
            Ret result{};
            auto err = callArgs.call(&result);
            if (err.hasErr()) {
                return Error(err.takeErr());
            }
            return Result<Ret, ProgramError>{std::move(result)};
        }
    }

    const RawFunction* raw() const noexcept { return &raw_; }

    RawFunction* rawMut() noexcept { return &raw_; }

  private:
    void initRaw() noexcept {
        raw_.name = StringSlice("externFn");
        raw_.qualifiedName = StringSlice("externFn");
        if constexpr (std::is_void_v<Ret>) {
            raw_.returnType = nullptr;
        } else {
            raw_.returnType = Reflect<Ret>::get();
        }
        raw_.argsTypes = getArgsTypes();
        raw_.argsLen = static_cast<uint16_t>(sizeof...(Args));
        raw_.alignment = SY_FUNCTION_MIN_ALIGN;
        raw_.comptimeSafe = true;
        raw_.tag = FunctionType::C;
    }

    static const Type** getArgsTypes() noexcept {
        if constexpr (sizeof...(Args) > 0) {
            static const Type* arr[sizeof...(Args)] = {Reflect<Args>::get()...};
            return arr;
        } else {
            return nullptr;
        }
    }

    static Result<void, ProgramError> infallibleTrampoline(FunctionHandler h) noexcept {
        return invokeInfallible(h, std::index_sequence_for<Args...>{});
    }

    template <size_t... Is>
    static Result<void, ProgramError> invokeInfallible(FunctionHandler h,
                                                       std::index_sequence<Is...>) noexcept {
        const RawFunction* rf = h.function();
        NormalFn fn = reinterpret_cast<NormalFn>(rf->innerFn);
        if constexpr (std::is_void_v<Ret>) {
            fn(h.template takeArg<Args>(Is)...);
        } else {
            Ret result = fn(h.template takeArg<Args>(Is)...);
            h.template setReturn<Ret>(std::move(result));
        }
        return {};
    }

    static Result<void, ProgramError> fallibleTrampoline(FunctionHandler h) noexcept {
        return invokeFallible(h, std::index_sequence_for<Args...>{});
    }

    template <size_t... Is>
    static Result<void, ProgramError> invokeFallible(FunctionHandler h,
                                                     std::index_sequence<Is...>) noexcept {
        const RawFunction* rf = h.function();
        FallibleFn fn = reinterpret_cast<FallibleFn>(rf->innerFn);
        if constexpr (std::is_void_v<Ret>) {
            return fn(h.template takeArg<Args>(Is)...);
        } else {
            Result<Ret, ProgramError> result = fn(h.template takeArg<Args>(Is)...);
            if (result.hasErr()) {
                return Error(result.takeErr());
            }
            h.template setReturn<Ret>(result.takeValue());
            return {};
        }
    }

    RawFunction raw_{};
};
} // namespace sy

#endif // SY_TYPES_FUNCTION_FUNCTION_HPP_
