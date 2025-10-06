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
    uint32_t major = 0;
    uint32_t minor = 0;
    uint32_t patch = 0;

    constexpr SemVer() = default;

    constexpr bool operator==(const SemVer& other) const {
        return this->major == other.major && this->minor == other.minor && this->patch == other.patch;
    }

    constexpr bool operator!=(const SemVer& other) const { return !(*this == other); }

    constexpr bool operator<(const SemVer& other) const {
        if (this->major < other.major)
            return true;
        else if (this->major > other.major)
            return false;

        if (this->minor < other.minor)
            return true;
        else if (this->minor > other.minor)
            return false;

        if (this->patch < other.patch)
            return true;
        return false;
    }

    constexpr bool operator>(const SemVer& other) const {
        if (this->major > other.major)
            return true;
        else if (this->major < other.major)
            return false;

        if (this->minor > other.minor)
            return true;
        else if (this->minor < other.minor)
            return false;

        if (this->patch > other.patch)
            return true;
        return false;
    }
};

enum class ModuleErr : int {
    OutOfMemory = 0,
    FileNotSyncSource,
    ErrorOpeningSourceFile,
    DuplicateDependency,
    Unknown,
};

class Compiler;
class Module;

class SY_API Module final {
  public:
    ~Module() noexcept;

    Module(Module&& other) noexcept;

    Module& operator=(Module&& other) noexcept;

    Module(const Module& other) = delete;

    Module& operator=(const Module& other) = delete;

    StringSlice name() const;

    SemVer version() const;

    /// @brief Sets the root file of the source tree.
    /// @param path Either absolute or relative
    [[nodiscard]] Result<void, ModuleErr> setRootFileFromDisk(StringSlice path) noexcept;

    [[nodiscard]] Result<void, ModuleErr> addDependency(const Module* module) noexcept;

  private:
    friend class Compiler;

    Module() : inner_(nullptr) {}

    static Result<Module*, AllocErr> create(Allocator& alloc, StringSlice inName, SemVer inVersion) noexcept;

    void* inner_;
};
} // namespace sy

#endif // SY_COMPILER_MODULE_HPP_