#include "map.hpp"
#include "../../util/assert.hpp"
#include "../../util/os_callstack.hpp"
#include "groups.hpp"
#include <cstdlib>
#include <cstring>
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
    this->count_ = 0;
}

void sy::RawMapUnmanaged::destroyScript(Allocator& alloc, const Type* keyType, const Type* valueType) noexcept {
    Group* groups = asGroupsMut(this->groups_);
    for (size_t i = 0; i < this->groupCount_; i++) {
        groups[i].destroyHeadersScriptKeyValue(alloc, keyType, valueType);
    }
    alloc.freeArray(groups, this->groupCount_);
    this->groups_ = nullptr;
    this->groupCount_ = 0;
    this->count_ = 0;
}

struct FoundGroup {
    size_t groupIndex;
    uint32_t valueIndex;
    bool hasValue;
};

static FoundGroup findImpl(const Group* groups, size_t groupCount, size_t hashCode) noexcept {
    Group::IndexBitmask index(hashCode);
    Group::PairBitmask pair(hashCode);
    const size_t groupIndex = index.value % groupCount;

    const Group& group = groups[groupIndex];
    auto foundIndex = group.find(pair);
    if (foundIndex.has_value() == false) {
        return {0, 0, false};
    } else {
        FoundGroup actualFound = {groupIndex, foundIndex.value(), true};
        return actualFound;
    }
}

std::optional<const void*> sy::RawMapUnmanaged::find(const void* key, size_t (*hash)(const void* key), size_t keyAlign,
                                                     size_t keySize, size_t valueAlign) const noexcept {
    if (this->count_ == 0)
        return std::nullopt;

    const Group* groups = asGroups(this->groups_);
    size_t hashCode = hash(key);
    auto foundIndex = findImpl(groups, this->groupCount_, hashCode);
    if (foundIndex.hasValue == false) {
        return std::nullopt;
    } else {
        const Group& group = groups[foundIndex.groupIndex];
        return group.headers()[foundIndex.valueIndex]->value(keyAlign, keySize, valueAlign);
    }
}

std::optional<const void*> sy::RawMapUnmanaged::findScript(const void* key, const Type* keyType,
                                                           const Type* valueType) const noexcept {
    if (this->count_ == 0)
        return std::nullopt;

    const Group* groups = asGroups(this->groups_);
    size_t hashCode = keyType->hashObj(key);
    auto foundIndex = findImpl(groups, this->groupCount_, hashCode);
    if (foundIndex.hasValue == false) {
        return std::nullopt;
    } else {
        const Group& group = groups[foundIndex.groupIndex];
        return group.headers()[foundIndex.valueIndex]->value(keyType->alignType, keyType->sizeType,
                                                             valueType->alignType);
    }
}

std::optional<void*> sy::RawMapUnmanaged::findMut(const void* key, size_t (*hash)(const void* key), size_t keyAlign,
                                                  size_t keySize, size_t valueAlign) const noexcept {
    if (this->count_ == 0)
        return std::nullopt;

    Group* groups = asGroupsMut(this->groups_);
    size_t hashCode = hash(key);
    auto foundIndex = findImpl(groups, this->groupCount_, hashCode);
    if (foundIndex.hasValue == false) {
        return std::nullopt;
    } else {
        Group& group = groups[foundIndex.groupIndex];
        return group.headers()[foundIndex.valueIndex]->value(keyAlign, keySize, valueAlign);
    }
}

std::optional<void*> sy::RawMapUnmanaged::findMutScript(const void* key, const Type* keyType,
                                                        const Type* valueType) const noexcept {
    if (this->count_ == 0)
        return std::nullopt;

    Group* groups = asGroupsMut(this->groups_);
    size_t hashCode = keyType->hashObj(key);
    auto foundIndex = findImpl(groups, this->groupCount_, hashCode);
    if (foundIndex.hasValue == false) {
        return std::nullopt;
    } else {
        Group& group = groups[foundIndex.groupIndex];
        return group.headers()[foundIndex.valueIndex]->value(keyType->alignType, keyType->sizeType,
                                                             valueType->alignType);
    }
}

sy::AllocExpect<bool> sy::RawMapUnmanaged::insert(Allocator& alloc, void* optionalOldValue, void* key, void* value,
                                                  size_t (*hash)(const void* key), void (*destructKey)(void* ptr),
                                                  void (*destructValue)(void* ptr), size_t keySize, size_t keyAlign,
                                                  size_t valueSize, size_t valueAlign) {
    if (auto ensureCapacityResult = this->ensureCapacityForInsert(alloc); ensureCapacityResult.hasValue() == false) {
        return sy::AllocExpect<bool>();
    }

    size_t hashCode = hash(key);
    Group* groups = asGroupsMut(this->groups_);

    if (this->count_ != 0) {
        auto foundIndex = findImpl(groups, this->groupCount_, hashCode);
        if (foundIndex.hasValue) {
            Group& group = groups[foundIndex.groupIndex];
            Group::Header* pair = group.headers()[foundIndex.valueIndex];

            destructKey(key); // don't need duplicates
            void* oldValue = pair->value(keyAlign, keySize, valueAlign);

            if (optionalOldValue) {
                memcpy(optionalOldValue, oldValue, keySize);
            } else { // discard old value
                destructValue(oldValue);
            }

            memcpy(oldValue, value, valueSize);
            return sy::AllocExpect<bool>(true);
        }
    }

    // no duplicate entry
    Group::IndexBitmask index(hashCode);
    const size_t groupIndex = index.value % this->groupCount_;
    Group& group = groups[groupIndex];
    auto insertResult = group.insertKeyValue(alloc, key, value, hashCode, keySize, keyAlign, valueSize, valueAlign);
    if(insertResult.hasValue()) {
        this->count_ += 1;
        return sy::AllocExpect<bool>(false);
    }
    
    return sy::AllocExpect<bool>();
}

sy::AllocExpect<void> sy::RawMapUnmanaged::ensureCapacityForInsert(Allocator& alloc) {

    if (this->available_ != 0)
        return sy::AllocExpect<void>(std::true_type{});
    ;

    const size_t newGroupCount = [](size_t currentGroupCount) -> size_t {
        constexpr size_t DEFAULT_GROUP_COUNT = 1;
        if (currentGroupCount == 0)
            return DEFAULT_GROUP_COUNT;
        return currentGroupCount * 2;
    }(this->groupCount_);

    auto allocResult = alloc.allocArray<Group>(newGroupCount);
    if (allocResult.hasValue() == false) {
        return sy::AllocExpect<void>();
    }

    Group* newGroups = allocResult.value();
    for (size_t i = 0; i < newGroupCount; i++) {
        auto groupAllocResult = Group::create(alloc);
        if (groupAllocResult.hasValue() == false) {
            for (size_t j = 0; j < i; j++) {
                newGroups[j].freeMemory(alloc);
            }
            alloc.freeArray(newGroups, newGroupCount);
            return sy::AllocExpect<void>();
        }

        newGroups[i] = groupAllocResult.take();
    }

    const size_t newAvailable = [newGroupCount](size_t cnt) {
        const size_t newLoadFactor = calculateLoadFactor(newGroupCount * 16); // 16 items per group by default
        sy_assert(newLoadFactor > cnt, "Failed to increase avaiable space");
        return newLoadFactor - cnt;
    }(this->count_);

    if (this->groupCount_ == 0) {
        this->available_ = newAvailable;
        this->groupCount_ = newGroupCount;
        this->groups_ = newGroups;
        return sy::AllocExpect<void>(std::true_type{});
    }

    // move over all pairs
    for (size_t oldGroupIter = 0; oldGroupIter < this->groupCount_; oldGroupIter++) {
        Group& oldGroup = asGroupsMut(this->groups_)[oldGroupIter];
        if (oldGroup.itemCount() == 0)
            continue;

        for (uint32_t hashMaskIter = 0; hashMaskIter < oldGroup.capacity(); hashMaskIter++) {
            if (oldGroup.hashMasksAsBytes()[hashMaskIter] == 0)
                continue;

            Group::Header* pair = oldGroup.headers()[hashMaskIter];
            const Group::IndexBitmask index(pair->hashCode);
            const size_t groupIndex = index.value % newGroupCount;
            Group& newGroup = newGroups[groupIndex];

            auto newGroupAllocResult = newGroup.ensureCapacityFor(alloc, newGroup.itemCount() + 1);
            // Free new memory if allocation failed
            if (newGroupAllocResult.hasValue() == false) {
                for (size_t cleanupIter = 0; cleanupIter < newGroupCount; cleanupIter++) {
                    newGroups[cleanupIter].freeMemory(alloc);
                }
                alloc.freeArray(newGroups, newGroupCount);
                return sy::AllocExpect<void>();
            }

            newGroup.setMaskAt(newGroup.itemCount(), pair->hashCode);
            newGroup.headers()[newGroup.itemCount()] = pair;
            newGroup.setItemCount(newGroup.itemCount() + 1);
        }
    }

    // All allocations succeeded, cleanup old
    {
        Group* oldGroups = asGroupsMut(this->groups_);
        for (size_t i = 0; i < this->groupCount_; i++) {
            oldGroups[i].freeMemory(alloc);
        }
        alloc.freeArray(oldGroups, this->groupCount_);
    }

    this->groups_ = reinterpret_cast<void*>(newGroups);
    this->groupCount_ = newGroupCount;
    this->available_ = newAvailable;

    return sy::AllocExpect<void>(std::true_type{});
}
