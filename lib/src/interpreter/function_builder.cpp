#include "function_builder.hpp"
#include "../program/program_internal.hpp"
#include "../types/type_info.hpp"
#include "../util/assert.hpp"

using namespace sy;

sy::FunctionBuilder::FunctionBuilder(Allocator alloc) noexcept : args(alloc), bytecode(alloc) {}

Result<void, AllocErr> sy::FunctionBuilder::addArg(const Type* type) noexcept { return this->args.push(type); }

Result<void, AllocErr> sy::FunctionBuilder::pushBytecode(const Bytecode* bytecodeArr, size_t count) noexcept {
    if (this->bytecode.reserve(this->bytecode.len() + count).hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    for (size_t i = 0; i < count; i++) {
        auto pushResult = this->bytecode.push(bytecodeArr[i]);
        sy_assert(pushResult.hasValue(), "Memory allocation should not have failed here");
    }
    return {};
}
