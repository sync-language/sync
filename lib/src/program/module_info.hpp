//! API
#ifndef SY_PROGRAM_MODULE_INFO_HPP_
#define SY_PROGRAM_MODULE_INFO_HPP_

#include "../core.h"
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
} // namespace sy

#endif // SY_PROGRAM_MODULE_INFO_HPP_