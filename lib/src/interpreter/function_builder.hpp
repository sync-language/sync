#ifndef SY_INTERPRETER_FUNCTION_BUILDER_HPP_
#define SY_INTERPRETER_FUNCTION_BUILDER_HPP_

#include "../mem/allocator.hpp"
#include "../types/array/list.hpp"
#include "../types/option/option.hpp"
#include "../types/result/result.hpp"
#include "../types/type.hpp"
#include "bytecode.hpp"

namespace sy {

struct FunctionBuilder {
    Option<Type> retType{};
    List<Type> args;
    List<Bytecode> bytecode;
    List<int16_t> unwindSlots;
    size_t stackSpaceRequired = 0;

    FunctionBuilder(Allocator alloc) noexcept;

    ~FunctionBuilder() noexcept = default;

    [[nodiscard]] Result<void, AllocErr> addArg(Type type) noexcept;

    [[nodiscard]] Result<void, AllocErr> pushBytecode(const Bytecode* bytecodeArr,
                                                      size_t count) noexcept;

    /// @param slot Which slot to unwind. Must be in range of [0 - `stackSpaceRequired`)
    [[nodiscard]] Result<void, AllocErr> pushUnwindSlot(int16_t slot) noexcept;
};
} // namespace sy

#endif // SY_INTERPRETER_FUNCTION_BUILDER_HPP_
