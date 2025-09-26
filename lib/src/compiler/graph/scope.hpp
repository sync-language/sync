#ifndef SY_COMPILER_GRAPH_SCOPE_HPP_
#define SY_COMPILER_GRAPH_SCOPE_HPP_

#include "../../types/array/dynamic_array.hpp"
#include "../../types/hash/map.hpp"
#include "../../types/option/option.hpp"
#include "../../types/string/string_slice.hpp"

namespace sy {
struct SyncVariable {
    StringSlice name;
    bool isMutable;
};

struct ScopeSymbol {
    enum class Tag : int {
        /// Local to a function
        LocalVariable = 0,
        Function = 1,
        Struct = 2,
        Global = 3,
    };

    Tag symbolType;
    StringSlice name;

    bool operator==(const ScopeSymbol& other) const { return symbolType == other.symbolType && name == other.name; }
};
} // namespace sy

namespace std {
template <> struct hash<sy::ScopeSymbol> {
    size_t operator()(const sy::ScopeSymbol& obj) { return obj.name.hash(); }
};
} // namespace std

namespace sy {
struct Scope;
struct Scope {
    /// If this scope is within a function, then variables may be stack variables.
    bool isInFunction;
    /// Notes that this is a sync block, allowing the accessing of `Owned`,
    /// `Shared`, and `Weak` types.
    bool isSync;
    /// Will contain elements if `isSync == true`
    DynArrayUnmanaged<SyncVariable> syncVariables;
    /// The symbols defined within this scope.
    MapUnmanaged<ScopeSymbol, bool> symbols;
    Option<Scope*> parent;

    /// Returns true if the symbol named `symbolName` is found and synchronized
    /// within this scope, or it's parents scopes, otherwise returns false.
    /// @returns An option containing a boolean, in which if `true`, the symbol
    /// is mutable synced, and if `false` is read-only synced. If the option is
    /// empty, the symbol is not synced.
    Option<bool> isSymbolSynced(StringSlice name) const noexcept;
};
} // namespace sy

#endif // SY_COMPILER_GRAPH_SCOPE_HPP_