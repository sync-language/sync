#include "groups.hpp"
#include "../../util/align.hpp"

void Group::destroyHeadersKeyOnly(sy::Allocator alloc, void (*destruct)(void* key), size_t keyAlign, size_t keySize) {
    for (uint32_t i = 0; i < capacity_; i++) {
        if (hashMasks_[i] == 0)
            continue;

        Header* header = headers()[i];
        header->destroyKeyOnly(alloc, destruct, keyAlign, keySize);
    }
}

void Group::destroyHeadersScriptKeyOnly(sy::Allocator alloc, const sy::Type* type) {
    for (uint32_t i = 0; i < capacity_; i++) {
        if (hashMasks_[i] == 0)
            continue;

        Header* header = headers()[i];
        header->destroyScriptKeyOnly(alloc, type);
    }
}

void Group::destroyHeadersKeyValue(sy::Allocator alloc, void (*destructKey)(void* key),
                                   void (*destructValue)(void* value), size_t keyAlign, size_t keySize,
                                   size_t valueAlign, size_t valueSize) {
    for (uint32_t i = 0; i < capacity_; i++) {
        if (hashMasks_[i] == 0)
            continue;

        Header* header = headers()[i];
        header->destroyKeyValue(alloc, destructKey, destructValue, keyAlign, keySize, valueAlign, valueSize);
    }
}

void Group::destroyHeadersScriptKeyValue(sy::Allocator alloc, const sy::Type* keyType, const sy::Type* valueType) {
    for (uint32_t i = 0; i < capacity_; i++) {
        if (hashMasks_[i] == 0)
            continue;

        Header* header = headers()[i];
        header->destroyScriptKeyValue(alloc, keyType, valueType);
    }
}

Group::Header** Group::headers() { return reinterpret_cast<Header**>(&this->hashMasks_[this->capacity_]); }

const Group::Header* const* Group::headers() const {
    return reinterpret_cast<const Header* const*>(&this->hashMasks_[this->capacity_]);
}

std::optional<uint32_t> Group::find(PairBitmask pair) const {
    const ByteSimd<16>* simdMasks = this->hashMasks();
    const uint32_t maskCount = this->simdHashMaskCount();
    for (uint32_t i = 0; i < maskCount; i++) {
        SimdMask<16> found = simdMasks->equalMask(pair.value);
        auto begin = found.begin();
        auto end = found.end();
        if (begin != end) {
            std::optional<uint32_t>((i * 16) + *begin);
        }
    }
    return std::nullopt;
}

void* Group::Header::key(size_t keyAlign) {
    const size_t byteOffset = byteOffsetForAlignedMember(sizeof(Header), keyAlign);
    uint8_t* asBytes = reinterpret_cast<uint8_t*>(this);
    return &asBytes[byteOffset];
}

const void* Group::Header::key(size_t keyAlign) const {
    const size_t byteOffset = byteOffsetForAlignedMember(sizeof(Header), keyAlign);
    const uint8_t* asBytes = reinterpret_cast<const uint8_t*>(this);
    return &asBytes[byteOffset];
}

void* Group::Header::value(size_t keyAlign, size_t keySize, size_t valueAlign) {
    const size_t byteOffsetForKey = byteOffsetForAlignedMember(sizeof(Header), keyAlign);
    const size_t byteOffsetForValue = byteOffsetForAlignedMember(byteOffsetForKey + keySize, valueAlign);
    uint8_t* asBytes = reinterpret_cast<uint8_t*>(this);
    return &asBytes[byteOffsetForValue];
}

const void* Group::Header::value(size_t keyAlign, size_t keySize, size_t valueAlign) const {
    const size_t byteOffsetForKey = byteOffsetForAlignedMember(sizeof(Header), keyAlign);
    const size_t byteOffsetForValue = byteOffsetForAlignedMember(byteOffsetForKey + keySize, valueAlign);
    const uint8_t* asBytes = reinterpret_cast<const uint8_t*>(this);
    return &asBytes[byteOffsetForValue];
}

void Group::Header::destroyKeyOnly(sy::Allocator alloc, void (*destruct)(void* key), size_t keyAlign, size_t keySize) {
    const size_t byteOffsetForKey = byteOffsetForAlignedMember(sizeof(Header), keyAlign);
    uint8_t* asBytes = reinterpret_cast<uint8_t*>(this);
    destruct(&asBytes[byteOffsetForKey]);

    const size_t allocSize = byteOffsetForKey + keySize;
    const size_t allocAlign = keyAlign > alignof(Header) ? keyAlign : alignof(Header);
    alloc.freeAlignedArray(asBytes, allocSize, allocAlign);
}

void Group::Header::destroyScriptKeyOnly(sy::Allocator alloc, const sy::Type* type) {
    const size_t byteOffsetForKey = byteOffsetForAlignedMember(sizeof(Header), type->alignType);
    uint8_t* asBytes = reinterpret_cast<uint8_t*>(this);
    type->destroyObject(&asBytes[byteOffsetForKey]);

    const size_t allocSize = byteOffsetForKey + type->sizeType;
    const size_t allocAlign = type->alignType > alignof(Header) ? type->alignType : alignof(Header);
    alloc.freeAlignedArray(asBytes, allocSize, allocAlign);
}

void Group::Header::destroyKeyValue(sy::Allocator alloc, void (*destructKey)(void* key),
                                    void (*destructValue)(void* value), size_t keyAlign, size_t keySize,
                                    size_t valueAlign, size_t valueSize) {
    const size_t byteOffsetForKey = byteOffsetForAlignedMember(sizeof(Header), keyAlign);
    const size_t byteOffsetForValue = byteOffsetForAlignedMember(byteOffsetForKey + keySize, valueAlign);
    uint8_t* asBytes = reinterpret_cast<uint8_t*>(this);
    destructKey(&asBytes[byteOffsetForKey]);
    destructValue(&asBytes[byteOffsetForValue]);

    const size_t allocSize = byteOffsetForValue + valueSize;
    const size_t allocAlign = [keyAlign, valueAlign]() {
        if (keyAlign > valueAlign) {
            if (keyAlign > alignof(Header))
                return keyAlign;
            return alignof(Header);
        } else {
            if (valueAlign > alignof(Header))
                return valueAlign;
            return alignof(Header);
        }
    }();
    alloc.freeAlignedArray(asBytes, allocSize, allocAlign);
}

void Group::Header::destroyScriptKeyValue(sy::Allocator alloc, const sy::Type* keyType, const sy::Type* valueType) {
    const size_t byteOffsetForKey = byteOffsetForAlignedMember(sizeof(Header), keyType->alignType);
    const size_t byteOffsetForValue =
        byteOffsetForAlignedMember(byteOffsetForKey + keyType->sizeType, valueType->alignType);
    uint8_t* asBytes = reinterpret_cast<uint8_t*>(this);
    keyType->destroyObject(&asBytes[byteOffsetForKey]);
    valueType->destroyObject(&asBytes[byteOffsetForValue]);

    const size_t allocSize = byteOffsetForValue + valueType->sizeType;
    const size_t allocAlign = [keyType, valueType]() -> size_t {
        if (keyType->alignType > valueType->alignType) {
            if (keyType->alignType > alignof(Header))
                return keyType->alignType;
            return alignof(Header);
        } else {
            if (valueType->alignType > alignof(Header))
                return valueType->alignType;
            return alignof(Header);
        }
    }();
    alloc.freeAlignedArray(asBytes, allocSize, allocAlign);
}

sy::AllocExpect<Group::Header*> Group::Header::createKeyOnly(sy::Allocator alloc, size_t keyAlign, size_t keySize) {
    const size_t byteOffsetForKey = byteOffsetForAlignedMember(sizeof(Header), keyAlign);
    const size_t allocSize = byteOffsetForKey + keySize;
    const size_t allocAlign = keyAlign > alignof(Header) ? keyAlign : alignof(Header);

    auto result = alloc.allocAlignedArray<uint8_t>(allocSize, allocAlign);
    if (result.hasValue() == false) {
        return sy::AllocExpect<Header*>();
    }
    return sy::AllocExpect<Header*>(reinterpret_cast<Header*>(result.value()));
}

sy::AllocExpect<Group::Header*> Group::Header::createKeyValue(sy::Allocator alloc, size_t keyAlign, size_t keySize,
                                                              size_t valueAlign, size_t valueSize) {
    const size_t byteOffsetForKey = byteOffsetForAlignedMember(sizeof(Header), keyAlign);
    const size_t byteOffsetForValue = byteOffsetForAlignedMember(byteOffsetForKey + keySize, valueAlign);
    const size_t allocSize = byteOffsetForValue + valueSize;
    const size_t allocAlign = [keyAlign, valueAlign]() {
        if (keyAlign > valueAlign) {
            if (keyAlign > alignof(Header))
                return keyAlign;
            return alignof(Header);
        } else {
            if (valueAlign > alignof(Header))
                return valueAlign;
            return alignof(Header);
        }
    }();

    auto result = alloc.allocAlignedArray<uint8_t>(allocSize, allocAlign);
    if (result.hasValue() == false) {
        return sy::AllocExpect<Header*>();
    }

    Header* self = reinterpret_cast<Header*>(result.value());
    self->hashCode = 0;
    self->iterBefore = nullptr;
    self->iterAfter = nullptr;
    return sy::AllocExpect<Header*>(self);
}

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"

using sy::Allocator;
using Header = Group::Header;

TEST_SUITE("header") {
    TEST_CASE("createKeyOnly / destroyKeyOnly no destructor") {
        { // less than or header align
            auto result = Header::createKeyOnly(Allocator(), alignof(uint8_t), sizeof(uint8_t));
            CHECK(result.hasValue());

            Header* self = result.value();
            self->destroyKeyOnly(
                Allocator(), [](void*) {}, alignof(uint8_t), sizeof(uint8_t));
        }
        { // same as header align
            auto result = Header::createKeyOnly(Allocator(), alignof(void*), sizeof(void*));
            CHECK(result.hasValue());

            Header* self = result.value();
            self->destroyKeyOnly(
                Allocator(), [](void*) {}, alignof(void*), alignof(void*));
        }
        { // greater than header align
            auto result = Header::createKeyOnly(Allocator(), alignof(ByteSimd<64>), sizeof(ByteSimd<64>));
            CHECK(result.hasValue());

            Header* self = result.value();
            self->destroyKeyOnly(
                Allocator(), [](void*) {}, alignof(ByteSimd<64>), alignof(ByteSimd<64>));
        }
    }
}

#endif // SYNC_LIB_NO_TESTS
