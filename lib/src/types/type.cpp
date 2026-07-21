#include "type.hpp"
#include "../core/core_internal.h"

using namespace sy;

bool Type::isIndirectionLevelMutable(uint32_t level) const noexcept {
    sy_assert(this->indirection <= 32, "`indirection` value out of range");
    sy_assert(level < this->indirection,
              "Expected `level` to be less than or equal to `indirection`");

    return (this->mutableBits & (1u << level)) != 0u;
}

Type Type::reference(bool isMutable) const noexcept {
    // Check from 31 cause the new one would have 32.
    sy_assert(this->indirection <= 31, "`indirection` value out of range");
    Type out = *this;
    out.indirection += 1;
    out.mutableBits |= (static_cast<uint32_t>(isMutable) << this->indirection);
    return out;
}

Type sy::Type::dereference() const noexcept {
    sy_assert(this->indirection <= 32, "`indirection` value out of range");
    sy_assert(this->indirection > 0, "Cannot dereference non-reference type");
    Type out = *this;
    out.indirection -= 1;
    out.mutableBits &= (~(1u << out.indirection));
    return out;
}

bool sy::Type::operator==(const Type& other) const noexcept {
    if (this->indirection != other.indirection) {
        return false;
    }
    if (this->mutableBits != other.mutableBits) {
        return false;
    }
    if (this->base == other.base) {
        return true;
    }

    if (this->base->typeSize != other.base->typeSize) {
        return false;
    }
    if (this->base->typeAlign != other.base->typeAlign) {
        return false;
    }

    // don't check name, it can differ

    if (this->base->extra.tag != other.base->extra.tag) {
        return false;
    }

    using Tag = sy::TypeExtra::Tag;

    const TypeExtra::Info& thisInfo = this->base->extra.info;
    const TypeExtra::Info& otherInfo = other.base->extra.info;
    switch (this->base->extra.tag) {
    case Tag::Bool:
        return true;
    case Tag::Int:
        return (thisInfo.intInfo.isSigned == otherInfo.intInfo.isSigned) &&
               (thisInfo.intInfo.bits == otherInfo.intInfo.bits);
    case Tag::Float:
        return (thisInfo.floatInfo.bits == otherInfo.floatInfo.bits);
    case Tag::Reference:
        return (thisInfo.referenceInfo.isMutable == otherInfo.referenceInfo.isMutable) &&
               // this one recursive... hmm maybe ok
               (thisInfo.referenceInfo.childType == otherInfo.referenceInfo.childType);
    case Tag::Function:
        // recursive...
        const bool retTypeSame = thisInfo.functionInfo.retType == otherInfo.functionInfo.retType;
        const bool argTypesSame = [&thisInfo, &otherInfo]() -> bool {
            if (thisInfo.functionInfo.argLen != otherInfo.functionInfo.argLen)
                return false;
            for (uint16_t i = 0; i < thisInfo.functionInfo.argLen; i++) {
                // very recursive...
                if (thisInfo.functionInfo.argTypes[i] != otherInfo.functionInfo.argTypes[i]) {
                    return false;
                }
            }
            return true;
        }();
        return retTypeSame && argTypesSame;
    default:
        break;
    }

    return true;
}
