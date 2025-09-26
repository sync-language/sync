#include "scope.hpp"
#include "../../util/assert.hpp"

using sy::Option;
using sy::Scope;
using sy::StringSlice;

Option<bool> Scope::isSymbolSynced(StringSlice name) const noexcept {
    sy_assert(name.len() > 0, "Expected a non-empty name");
    sy_assert(name.data() != nullptr, "Expected a non-empty name");

    const Scope* checking = this;
    while (checking != nullptr) {
        for (size_t i = 0; i < checking->syncVariables.len(); i++) {
            if (name == checking->syncVariables[i].name) {
                return checking->syncVariables[i].isMutable;
            }
        }

        if (!checking->parent) {
            break;
        }
        checking = checking->parent.value();
    }
    return {};
}
