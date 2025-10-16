//! API
#pragma once
#ifndef SY_COMPILER_COMPILER_HPP_
#define SY_COMPILER_COMPILER_HPP_

#include "../core.h"
#include "../mem/allocator.hpp"
#include "../program/program.hpp"
#include "../program/program_error.hpp"
#include "../types/array/dynamic_array.hpp"
#include "../types/hash/map.hpp"
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

struct ModuleVersion {
    StringSlice name;
    SemVer version;

    bool operator==(const ModuleVersion& other) const {
        return this->name == other.name && this->version == other.version;
    }
};

class Program;
class ProgramError;
class Module;

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

    [[nodiscard]] Result<DynArray<const Module*>, AllocErr> allModules() const noexcept;

    [[nodiscard]] Result<Program, ProgramError> compile() const noexcept;

  private:
    Compiler() : inner_(nullptr){};

    void* inner_;
};

enum class ModuleErr : int {
    OutOfMemory = 0,
    FileNotSyncSource,
    ErrorOpeningSourceFile,
    DuplicateDependency,
    Unknown,
};

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

    [[nodiscard]] const MapUnmanaged<StringSlice, const Module*>& dependencies() const noexcept;

  private:
    friend class Compiler;

    Module() : inner_(nullptr) {}

    static Result<Module*, AllocErr> create(Allocator& alloc, StringSlice inName, SemVer inVersion) noexcept;

    void* inner_;
};
} // namespace sy

namespace std {
template <> struct hash<sy::ModuleVersion> {
    size_t operator()(const sy::ModuleVersion& obj) { return obj.name.hash(); }
};
} // namespace std

#endif // SY_COMPILER_COMPILER_HPP_