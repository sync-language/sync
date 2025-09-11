//! API
#pragma once
#ifndef SY_COMPILER_MODULE_HPP_
#define SY_COMPILER_MODULE_HPP_

#include "../core.h"
#include "../mem/allocator.hpp"
#include "../types/result/result.hpp"
#include "../types/string/string_slice.hpp"

namespace sy {
struct SemVer {
    int major;
    int minor;
    int patch;
};

class Compiler;

class SY_API Module final {
  public:
    ~Module() noexcept;

    Module(Module&& other) noexcept;

    Module& operator=(Module&& other) noexcept;

    Module(const Module& other) = delete;

    Module& operator=(const Module& other) = delete;

    StringSlice name() const;

    SemVer version() const;

  private:
    friend class Compiler;

    Module() : inner_(nullptr) {}

    static Result<Module, AllocErr> create(Allocator& alloc, StringSlice inName, SemVer inVersion) noexcept;

    void* inner_;
};
} // namespace sy

#endif // SY_COMPILER_MODULE_HPP_