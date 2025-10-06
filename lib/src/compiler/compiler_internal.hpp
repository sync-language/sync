#ifndef SY_COMPILER_COMPILER_INTERNAL_HPP_
#define SY_COMPILER_COMPILER_INTERNAL_HPP_

#include "module.hpp"

namespace sy {
struct ModuleVersion {
    StringSlice name;
    SemVer version;

    bool operator==(const ModuleVersion& other) const {
        return this->name == other.name && this->version == other.version;
    }
};
} // namespace sy

namespace std {
template <> struct hash<sy::ModuleVersion> {
    size_t operator()(const sy::ModuleVersion& obj) { return obj.name.hash(); }
};
} // namespace std

#endif // SY_COMPILER_COMPILER_INTERNAL_HPP_
