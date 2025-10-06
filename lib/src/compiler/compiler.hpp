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

    /// @brief Attempts to add a new empty module, or get an existing module.
    /// within the compiler's module list,
    /// @param name Name of the module
    /// @param version Version of the module. Allows using multiple modules of
    /// different versions.
    /// @return If the module described by `name` and `version` already exists,
    /// returns that one, otherwise returns a new module. May fail with an
    /// out of memory error.
    [[nodiscard]] Result<Module*, AllocErr> addOrGetModule(StringSlice name, SemVer version) noexcept;

  private:
    Compiler() : inner_(nullptr){};

    void* inner_;
};
} // namespace sy

#endif // SY_COMPILER_COMPILER_HPP_