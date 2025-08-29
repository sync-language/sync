#include "map.hpp"
#include "../../util/os_callstack.hpp"
#include "groups.hpp"
#include <cstdlib>
#include <iostream>

const Group* asGroups(const void* groups_) { return reinterpret_cast<const Group*>(groups_); }

Group* asGroupsMut(void* groups_) { return reinterpret_cast<Group*>(groups_); }

sy::RawMapUnmanaged::~RawMapUnmanaged() noexcept {
// Ensure no leaks
#ifndef NDEBUG
    if (groupCount_ > 0) {
        try {
            std::cerr << "HashMap not properly destroyed." << std::endl;
            Backtrace bt = Backtrace::generate();
            bt.print();
        } catch (...) {
        }
        std::abort();
    }
#endif
}

void sy::RawMapUnmanaged::destroy(Allocator& alloc, void (*destructKey)(void* ptr), void (*destructValue)(void* ptr),
                                  size_t keySize, size_t keyAlign, size_t valueSize, size_t valueAlign) noexcept {
    Group* groups = asGroupsMut(this->groups_);
    for (size_t i = 0; i < this->groupCount_; i++) {
        groups[i].destroyHeadersKeyValue(alloc, destructKey, destructValue, keyAlign, keySize, valueAlign, valueSize);
    }
    alloc.freeArray(groups, this->groupCount_);
    this->groups_ = nullptr;
    this->groupCount_ = 0;
    this->elementCount_ = 0;
}

void sy::RawMapUnmanaged::destroyScript(Allocator& alloc, const Type* keyType, const Type* valueType) noexcept {
    Group* groups = asGroupsMut(this->groups_);
    for (size_t i = 0; i < this->groupCount_; i++) {
        groups[i].destroyHeadersScriptKeyValue(alloc, keyType, valueType);
    }
    alloc.freeArray(groups, this->groupCount_);
    this->groups_ = nullptr;
    this->groupCount_ = 0;
    this->elementCount_ = 0;
}

struct FoundGroup {
    size_t groupIndex;
    uint32_t valueIndex;
};

static std::optional<FoundGroup> findImpl(const Group* groups, size_t groupCount, size_t hashCode) noexcept {
    Group::IndexBitmask index(hashCode);
    Group::PairBitmask pair(hashCode);
    const size_t groupIndex = index.value % groupCount;

    const Group& group = groups[groupIndex];
    auto foundIndex = group.find(pair);
    if (foundIndex.has_value() == false) {
        return std::nullopt;
    } else {
        FoundGroup actualFound = {groupIndex, foundIndex.value()};
        return std::optional<FoundGroup>(actualFound);
    }
}

std::optional<const void*> sy::RawMapUnmanaged::find(const void* key, size_t (*hash)(const void* key), size_t keyAlign,
                                                     size_t keySize, size_t valueAlign) const noexcept {
    if (this->elementCount_ == 0)
        return std::nullopt;

    const Group* groups = asGroups(this->groups_);
    size_t hashCode = hash(key);
    auto foundIndex = findImpl(groups, this->groupCount_, hashCode);
    if (foundIndex.has_value() == false) {
        return std::nullopt;
    } else {
        FoundGroup actualFound = foundIndex.value();
        const Group& group = groups[actualFound.groupIndex];
        return group.headers()[actualFound.valueIndex]->value(keyAlign, keySize, valueAlign);
    }
}

std::optional<const void*> sy::RawMapUnmanaged::findScript(const void* key, const Type* keyType,
                                                           const Type* valueType) const noexcept {
    if (this->elementCount_ == 0)
        return std::nullopt;

    const Group* groups = asGroups(this->groups_);
    size_t hashCode = keyType->hashObj(key);
    auto foundIndex = findImpl(groups, this->groupCount_, hashCode);
    if (foundIndex.has_value() == false) {
        return std::nullopt;
    } else {
        FoundGroup actualFound = foundIndex.value();
        const Group& group = groups[actualFound.groupIndex];
        return group.headers()[actualFound.valueIndex]->value(keyType->alignType, keyType->sizeType,
                                                              valueType->alignType);
    }
}

std::optional<void*> sy::RawMapUnmanaged::findMut(const void* key, size_t (*hash)(const void* key), size_t keyAlign,
                                                  size_t keySize, size_t valueAlign) const noexcept {
    if (this->elementCount_ == 0)
        return std::nullopt;

    Group* groups = asGroupsMut(this->groups_);
    size_t hashCode = hash(key);
    auto foundIndex = findImpl(groups, this->groupCount_, hashCode);
    if (foundIndex.has_value() == false) {
        return std::nullopt;
    } else {
        FoundGroup actualFound = foundIndex.value();
        Group& group = groups[actualFound.groupIndex];
        return group.headers()[actualFound.valueIndex]->value(keyAlign, keySize, valueAlign);
    }
}

std::optional<void*> sy::RawMapUnmanaged::findMutScript(const void* key, const Type* keyType,
                                                        const Type* valueType) const noexcept {
    if (this->elementCount_ == 0)
        return std::nullopt;

    Group* groups = asGroupsMut(this->groups_);
    size_t hashCode = keyType->hashObj(key);
    auto foundIndex = findImpl(groups, this->groupCount_, hashCode);
    if (foundIndex.has_value() == false) {
        return std::nullopt;
    } else {
        FoundGroup actualFound = foundIndex.value();
        Group& group = groups[actualFound.groupIndex];
        return group.headers()[actualFound.valueIndex]->value(keyType->alignType, keyType->sizeType,
                                                              valueType->alignType);
    }
}
