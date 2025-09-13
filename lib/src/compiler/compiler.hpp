//! API
#pragma once
#ifndef SY_COMPILER_COMPILER_HPP_
#define SY_COMPILER_COMPILER_HPP_

#include "../core.h"
#include "../mem/allocator.hpp"
#include "../types/array/dynamic_array.hpp"
#include "../types/result/result.hpp"
#include "../types/string/string_slice.hpp"
#include "module.hpp"

namespace sy {
class SY_API Compiler final {
  public:
    /// @brief The Sync compiler uses the builder pattern to add modules,
    /// configure options, define extern types / functions, and more.
    /// This function will allocate and default initialize a `Compiler`.
    /// @param alloc The memory allocator to use by the compiler.
    /// Defaults to the default allocator (c allocator).
    /// @return Either the `Compiler` object, or a memory allocation error.
    [[nodiscard]] static Result<Compiler, AllocErr> create(Allocator alloc = {}) noexcept;

    ~Compiler() noexcept;

    Compiler(Compiler&& other) noexcept;

    Compiler& operator=(Compiler&& other) noexcept;

    Compiler(const Compiler& other) = delete;

    Compiler& operator=(const Compiler& other) = delete;

    enum class AddEmptyModuleErr : int {
        OutOfMemory = AllocErr::OutOfMemory,
        DuplicateModuleVersion,
    };

    [[nodiscard]] Result<Module*, AddEmptyModuleErr> addEmptyModule(StringSlice name, SemVer version) noexcept;

  private:
    Compiler() : inner_(nullptr){};

    void* inner_;
};
} // namespace sy

#endif // SY_COMPILER_COMPILER_HPP_