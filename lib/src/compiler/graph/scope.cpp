#include "scope.hpp"
#include "../../util/assert.hpp"

using sy::Option;
using sy::Scope;
using sy::StringSlice;

size_t std::hash<sy::ScopeSymbol>::operator()(const sy::ScopeSymbol& obj) {
    switch (obj.symbolType) {
    case sy::ScopeSymbol::Tag::LocalVariable: {
        return obj.data.localVariable.hash();
    };
    case sy::ScopeSymbol::Tag::Function: {
        return obj.data.function.hash();
    };
    case sy::ScopeSymbol::Tag::Scope: {
        std::hash<size_t> h;
        return h(obj.data.scope->symbols.len());
    };
    case sy::ScopeSymbol::Tag::Struct: {
        return obj.data.structName.hash();
    };
    case sy::ScopeSymbol::Tag::Global: {
        return obj.data.global.hash();
    };
    }
    return 0;
}

bool sy::ScopeSymbol::operator==(const ScopeSymbol& other) const noexcept {
    if (this->symbolType != other.symbolType)
        return false;

    switch (this->symbolType) {
    case sy::ScopeSymbol::Tag::LocalVariable: {
        return this->data.localVariable == other.data.localVariable;
    };
    case sy::ScopeSymbol::Tag::Function: {
        return this->data.function == other.data.function;
    };
    case sy::ScopeSymbol::Tag::Scope: {
        // TODO should this be for same objects or actual deep checking
        return this->data.scope == other.data.scope;
    };
    case sy::ScopeSymbol::Tag::Struct: {
        return this->data.structName == other.data.structName;
    };
    case sy::ScopeSymbol::Tag::Global: {
        return this->data.global == other.data.global;
    };
    default:
        return false;
    }
}

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
