#include "groups.hpp"
#include "../../util/align.hpp"
#include "../../util/assert.hpp"
#include <cstring>

constexpr static uint32_t groupAllocationSize(uint32_t requiredCapacity) {
    return requiredCapacity + (sizeof(Group::Header*) * requiredCapacity);
}

constexpr size_t GROUP_ALLOC_ALIGN = 16;

sy::AllocExpect<Group> Group::create(sy::Allocator& alloc) {
    constexpr uint32_t INITIAL_CAPACITY = 16;
    constexpr uint32_t INITIAL_ALLOCATION_SIZE = groupAllocationSize(INITIAL_CAPACITY);

    auto allocResult = alloc.allocAlignedArray<uint8_t>(INITIAL_ALLOCATION_SIZE, GROUP_ALLOC_ALIGN);
    if (allocResult.hasValue() == false) {
        return sy::AllocExpect<Group>();
    }

    Group group;
    group.hashMasks_ = allocResult.value();
    memset(group.hashMasks_, 0, INITIAL_CAPACITY);
    group.capacity_ = INITIAL_CAPACITY;
    group.itemCount_ = 0;
    return sy::AllocExpect<Group>(group);
}

void Group::freeMemory(sy::Allocator& alloc) {
    const size_t allocSize = groupAllocationSize(this->capacity_);
    alloc.freeAlignedArray(this->hashMasks_, allocSize, GROUP_ALLOC_ALIGN);
}

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

std::optional<uint32_t> Group::find(PairBitmask pair, const void* inKey, bool (*eq)(const void* key, const void* found),
                                    size_t keyAlign) const {
    const ByteSimd<16>* simdMasks = this->hashMasks();
    const uint32_t maskCount = this->simdHashMaskCount();
    for (uint32_t i = 0; i < maskCount; i++) {
        SimdMask<16> found = simdMasks->equalMask(pair.value);
        auto begin = found.begin();
        auto end = found.end();
        if (begin != end) {
            const uint32_t index = (i * 16) + *begin;
            const void* potentialMatch = this->headers()[index]->key(keyAlign);
            if (eq(inKey, potentialMatch)) {
                return std::optional<uint32_t>(index);
            }
        }
    }
    return std::nullopt;
}

sy::AllocExpect<void> Group::ensureCapacityFor(sy::Allocator& alloc, uint32_t minCapacity) {
    if (minCapacity <= this->capacity_)
        sy::AllocExpect<void>(std::true_type{});

    const uint32_t pairAllocCapacity = [minCapacity]() {
        const uint32_t remainder = minCapacity % 16;
        if (remainder == 0)
            return minCapacity;
        return minCapacity + (16 - remainder);
    }();
    const uint32_t allocSize = groupAllocationSize(pairAllocCapacity);

    auto allocResult = alloc.allocAlignedArray<uint8_t>(allocSize, GROUP_ALLOC_ALIGN);
    if (allocResult.hasValue() == false) {
        return sy::AllocExpect<void>();
    }

    uint8_t* newHashMasks = allocResult.value();
    memcpy(newHashMasks, this->hashMasks_, this->capacity_);
    memset(&newHashMasks[this->capacity_], 0, pairAllocCapacity - this->capacity_);

    this->freeMemory(alloc);

    this->hashMasks_ = newHashMasks;
    this->capacity_ = pairAllocCapacity;
    return sy::AllocExpect<void>(std::true_type{});
}

void Group::setMaskAt(uint32_t index, PairBitmask pairMask) {
    sy_assert(index < this->capacity_, "Index out of bounds");
    this->hashMasks_[index] = pairMask.value;
}

sy::AllocExpect<void> Group::insertKeyValue(sy::Allocator& alloc, void* key, void* value, size_t hashCode,
                                            size_t keySize, size_t keyAlign, size_t valueSize, size_t valueAlign,
                                            Header** iterFirst, Header** iterLast) {
    if (auto ensureResult = this->ensureCapacityFor(alloc, this->itemCount_ + 1); ensureResult.hasValue() == false) {
        return ensureResult;
    }

    auto headerCreate = Header::createKeyValue(alloc, keyAlign, keySize, valueAlign, valueSize);
    if (headerCreate.hasValue() == false) {
        return sy::AllocExpect<void>();
    }

    Header* newHeader = headerCreate.value();
    memcpy(newHeader->key(keyAlign), key, keySize);
    memcpy(newHeader->value(keyAlign, keySize, valueAlign), value, valueSize);
    // first element in map
    if (*iterFirst == nullptr) {
        *iterFirst = newHeader;
    }

    // set last element or update linked list
    if (*iterLast == nullptr) {
        *iterLast = newHeader;
    } else {
        (*iterLast)->iterAfter = newHeader;
        *iterLast = newHeader;
    }

    const uint32_t zeroIndex = this->firstZeroIndex().value();

    this->headers()[zeroIndex] = newHeader;
    this->setMaskAt(zeroIndex, hashCode);
    this->itemCount_ += 1;

    return sy::AllocExpect<void>(std::true_type{});
}

std::optional<uint32_t> Group::firstZeroIndex() const {
    const ByteSimd<16>* simd = this->hashMasks();
    for (uint32_t i = 0; i < this->simdHashMaskCount(); i++) {
        auto fz = simd->firstZeroIndex();
        if (fz.has_value()) {
            return std::optional<uint32_t>(fz.value() + (i * 16));
        }
    }
    return std::optional<uint32_t>();
}

void Group::erase(sy::Allocator& alloc, uint32_t index, void (*destructKey)(void* ptr),
                  void (*destructValue)(void* ptr), size_t keySize, size_t keyAlign, size_t valueSize,
                  size_t valueAlign, Header** iterFirst, Header** iterLast) {
    Header* thisHeader = this->headers()[index];
    Header* before = thisHeader->iterBefore;
    Header* after = thisHeader->iterAfter;

    if (before != nullptr) {
        before->iterAfter = after;
    } else {
        *iterFirst = after;
    }

    if (after != nullptr) {
        after->iterBefore = before;
    } else {
        *iterLast = before;
    }

    this->headers()[index]->destroyKeyValue(alloc, destructKey, destructValue, keyAlign, keySize, valueAlign,
                                            valueSize);
    this->hashMasks_[index] = 0;
    this->itemCount_ -= 1;
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
    if (destruct)
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
    if (destructKey)
        destructKey(&asBytes[byteOffsetForKey]);
    if (destructValue)
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
            self->destroyKeyOnly(Allocator(), [](void*) {}, alignof(uint8_t), sizeof(uint8_t));
        }
        { // same as header align
            auto result = Header::createKeyOnly(Allocator(), alignof(void*), sizeof(void*));
            CHECK(result.hasValue());

            Header* self = result.value();
            self->destroyKeyOnly(Allocator(), [](void*) {}, alignof(void*), alignof(void*));
        }
        { // greater than header align
            auto result = Header::createKeyOnly(Allocator(), alignof(ByteSimd<64>), sizeof(ByteSimd<64>));
            CHECK(result.hasValue());

            Header* self = result.value();
            self->destroyKeyOnly(Allocator(), [](void*) {}, alignof(ByteSimd<64>), alignof(ByteSimd<64>));
        }
    }
}

#endif // SYNC_LIB_NO_TESTS
