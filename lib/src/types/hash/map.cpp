#include "map.hpp"
#include "../../util/assert.hpp"
#include "../../util/os_callstack.hpp"
#include "groups.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <tuple>

const Group* asGroups(const void* groups_) { return reinterpret_cast<const Group*>(groups_); }

Group* asGroupsMut(void* groups_) { return reinterpret_cast<Group*>(groups_); }

sy::RawMapUnmanaged::~RawMapUnmanaged() noexcept {
// Ensure no leaks
#ifndef NDEBUG
    if (groupCount_ > 0) {
        try {
            std::cerr << "HashMap not properly destroyed." << std::endl;
#ifdef SYNC_BACKTRACE_SUPPORTED
            Backtrace bt = Backtrace::generate();
            bt.print();
#endif
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
        groups[i].freeMemory(alloc);
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

static FoundGroup findImpl(const Group* groups, size_t groupCount, size_t hashCode, const void* key,
                           bool (*eq)(const void* key, const void* found), size_t keyAlign) noexcept {
    Group::IndexBitmask index(hashCode);
    Group::PairBitmask pair(hashCode);
    const size_t groupIndex = index.value % groupCount;

    const Group& group = groups[groupIndex];
    auto foundIndex = group.find(pair, key, eq, keyAlign);
    if (foundIndex.has_value() == false) {
        return {0, 0, false};
    } else {
        FoundGroup actualFound = {groupIndex, foundIndex.value(), true};
        return actualFound;
    }
}

static FoundGroup findScriptImpl(const Group* groups, size_t groupCount, size_t hashCode, const void* key,
                                 const sy::Type* keyType) noexcept {
    Group::IndexBitmask index(hashCode);
    Group::PairBitmask pair(hashCode);
    const size_t groupIndex = index.value % groupCount;

    const Group& group = groups[groupIndex];
    auto foundIndex = group.findScript(pair, key, keyType);
    if (foundIndex.has_value() == false) {
        return {0, 0, false};
    } else {
        FoundGroup actualFound = {groupIndex, foundIndex.value(), true};
        return actualFound;
    }
}

std::optional<const void*> sy::RawMapUnmanaged::find(const void* key, size_t (*hash)(const void* key),
                                                     bool (*eq)(const void* key, const void* found), size_t keyAlign,
                                                     size_t keySize, size_t valueAlign) const noexcept {
    if (this->count_ == 0)
        return std::nullopt;

    const Group* groups = asGroups(this->groups_);
    size_t hashCode = hash(key);
    auto foundIndex = findImpl(groups, this->groupCount_, hashCode, key, eq, keyAlign);
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
    auto foundIndex = findScriptImpl(groups, this->groupCount_, hashCode, key, keyType);
    if (foundIndex.hasValue == false) {
        return std::nullopt;
    } else {
        const Group& group = groups[foundIndex.groupIndex];
        return group.headers()[foundIndex.valueIndex]->value(keyType->alignType, keyType->sizeType,
                                                             valueType->alignType);
    }
}

std::optional<void*> sy::RawMapUnmanaged::findMut(const void* key, size_t (*hash)(const void* key),
                                                  bool (*eq)(const void* key, const void* found), size_t keyAlign,
                                                  size_t keySize, size_t valueAlign) noexcept {
    if (this->count_ == 0)
        return std::nullopt;

    Group* groups = asGroupsMut(this->groups_);
    size_t hashCode = hash(key);
    auto foundIndex = findImpl(groups, this->groupCount_, hashCode, key, eq, keyAlign);
    if (foundIndex.hasValue == false) {
        return std::nullopt;
    } else {
        Group& group = groups[foundIndex.groupIndex];
        return group.headers()[foundIndex.valueIndex]->value(keyAlign, keySize, valueAlign);
    }
}

std::optional<void*> sy::RawMapUnmanaged::findMutScript(const void* key, const Type* keyType,
                                                        const Type* valueType) noexcept {
    if (this->count_ == 0)
        return std::nullopt;

    Group* groups = asGroupsMut(this->groups_);
    size_t hashCode = keyType->hashObj(key);
    auto foundIndex = findScriptImpl(groups, this->groupCount_, hashCode, key, keyType);
    if (foundIndex.hasValue == false) {
        return std::nullopt;
    } else {
        Group& group = groups[foundIndex.groupIndex];
        return group.headers()[foundIndex.valueIndex]->value(keyType->alignType, keyType->sizeType,
                                                             valueType->alignType);
    }
}

sy::Result<bool, sy::AllocErr>
sy::RawMapUnmanaged::insert(Allocator& alloc, void* optionalOldValue, void* key, void* value,
                            size_t (*hash)(const void* key), void (*destructKey)(void* ptr),
                            void (*destructValue)(void* ptr), bool (*eq)(const void* searchKey, const void* found),
                            size_t keySize, size_t keyAlign, size_t valueSize, size_t valueAlign) noexcept {
    if (auto ensureCapacityResult = this->ensureCapacityForInsert(alloc); ensureCapacityResult.hasValue() == false) {
        return Error(AllocErr::OutOfMemory);
    }

    size_t hashCode = hash(key);
    Group* groups = asGroupsMut(this->groups_);

    if (this->count_ != 0) {
        auto foundIndex = findImpl(groups, this->groupCount_, hashCode, key, eq, keyAlign);
        if (foundIndex.hasValue) {
            Group& group = groups[foundIndex.groupIndex];
            Group::Header* pair = group.headers()[foundIndex.valueIndex];

            if (destructKey)
                destructKey(key); // don't need duplicates
            void* oldValue = pair->value(keyAlign, keySize, valueAlign);

            if (optionalOldValue) {
                memcpy(optionalOldValue, oldValue, valueSize);
            } else { // discard old value
                if (destructValue)
                    destructValue(oldValue);
            }

            memcpy(oldValue, value, valueSize);
            return true;
        }
    }

    // no duplicate entry
    Group::IndexBitmask index(hashCode);
    const size_t groupIndex = index.value % this->groupCount_;
    Group& group = groups[groupIndex];
    auto insertResult = group.insertKeyValue(alloc, key, value, hashCode, keySize, keyAlign, valueSize, valueAlign,
                                             reinterpret_cast<Group::Header**>(&this->iterFirst_),
                                             reinterpret_cast<Group::Header**>(&this->iterLast_));
    if (insertResult.hasValue()) {
        this->count_ += 1;
        this->available_ -= 1;
        return false;
    }

    return Error(AllocErr::OutOfMemory);
}

sy::Result<bool, sy::AllocErr> sy::RawMapUnmanaged::insertCustomMove(
    Allocator& alloc, void* optionalOldValue, void* key, void* value, size_t (*hash)(const void* key),
    void (*destructKey)(void* ptr), void (*destructValue)(void* ptr),
    bool (*eq)(const void* searchKey, const void* found), void (*keyMoveConstruct)(void* dst, void* src),
    void (*valueMoveConstruct)(void* dst, void* src), size_t keySize, size_t keyAlign, size_t valueSize,
    size_t valueAlign) {
    if (auto ensureCapacityResult = this->ensureCapacityForInsert(alloc); ensureCapacityResult.hasValue() == false) {
        return Error(AllocErr::OutOfMemory);
    }

    size_t hashCode = hash(key);
    Group* groups = asGroupsMut(this->groups_);

    if (this->count_ != 0) {
        auto foundIndex = findImpl(groups, this->groupCount_, hashCode, key, eq, keyAlign);
        if (foundIndex.hasValue) {
            Group& group = groups[foundIndex.groupIndex];
            Group::Header* pair = group.headers()[foundIndex.valueIndex];

            if (destructKey)
                destructKey(key); // don't need duplicates
            void* oldValue = pair->value(keyAlign, keySize, valueAlign);

            if (optionalOldValue) {
                valueMoveConstruct(optionalOldValue, oldValue);
            } else { // discard old value
                if (destructValue)
                    destructValue(oldValue);
            }

            valueMoveConstruct(oldValue, value);
            return true;
        }
    }

    // no duplicate entry
    Group::IndexBitmask index(hashCode);
    const size_t groupIndex = index.value % this->groupCount_;
    Group& group = groups[groupIndex];
    auto insertResult = group.insertKeyValueCustomMove(
        alloc, key, value, hashCode, keyMoveConstruct, valueMoveConstruct, keySize, keyAlign, valueSize, valueAlign,
        reinterpret_cast<Group::Header**>(&this->iterFirst_), reinterpret_cast<Group::Header**>(&this->iterLast_));
    if (insertResult.hasValue()) {
        this->count_ += 1;
        this->available_ -= 1;
        return false;
    }

    return Error(AllocErr::OutOfMemory);
}

sy::Result<bool, sy::AllocErr> sy::RawMapUnmanaged::insertScript(Allocator& alloc, void* optionalOldValue, void* key,
                                                                 void* value, const Type* keyType,
                                                                 const Type* valueType) {
    if (auto ensureCapacityResult = this->ensureCapacityForInsert(alloc); ensureCapacityResult.hasValue() == false) {
        return Error(AllocErr::OutOfMemory);
    }

    size_t hashCode = keyType->hashObj(key);
    Group* groups = asGroupsMut(this->groups_);

    if (this->count_ != 0) {
        auto foundIndex = findScriptImpl(groups, this->groupCount_, hashCode, key, keyType);
        if (foundIndex.hasValue) {
            Group& group = groups[foundIndex.groupIndex];
            Group::Header* pair = group.headers()[foundIndex.valueIndex];

            keyType->destroyObject(key);
            void* oldValue = pair->value(keyType->alignType, keyType->sizeType, valueType->alignType);

            if (optionalOldValue) {
                memcpy(optionalOldValue, oldValue, valueType->sizeType);
            } else { // discard old value
                valueType->destroyObject(oldValue);
            }

            memcpy(oldValue, value, valueType->sizeType);
            return true;
        }
    }

    // no duplicate entry
    Group::IndexBitmask index(hashCode);
    const size_t groupIndex = index.value % this->groupCount_;
    Group& group = groups[groupIndex];
    auto insertResult = group.insertKeyValue(
        alloc, key, value, hashCode, keyType->sizeType, keyType->alignType, valueType->sizeType, valueType->alignType,
        reinterpret_cast<Group::Header**>(&this->iterFirst_), reinterpret_cast<Group::Header**>(&this->iterLast_));
    if (insertResult.hasValue()) {
        this->count_ += 1;
        this->available_ -= 1;
        return false;
    }

    return Error(AllocErr::OutOfMemory);
}

bool sy::RawMapUnmanaged::erase(Allocator& alloc, const void* key, size_t (*hash)(const void* key),
                                void (*destructKey)(void* ptr), void (*destructValue)(void* ptr),
                                bool (*eq)(const void* searchKey, const void* found), size_t keySize, size_t keyAlign,
                                size_t valueSize, size_t valueAlign) noexcept {
    Group* groups = asGroupsMut(this->groups_);
    size_t hashCode = hash(key);
    auto foundIndex = findImpl(groups, this->groupCount_, hashCode, key, eq, keyAlign);
    if (foundIndex.hasValue == false) {
        return false;
    } else {
        Group& group = groups[foundIndex.groupIndex];
        group.erase(alloc, foundIndex.valueIndex, destructKey, destructValue, keySize, keyAlign, valueSize, valueAlign,
                    reinterpret_cast<Group::Header**>(&this->iterFirst_),
                    reinterpret_cast<Group::Header**>(&this->iterLast_));
        this->count_ -= 1;
        this->available_ += 1;
        return true;
    }
}

bool sy::RawMapUnmanaged::eraseScript(Allocator& alloc, const void* key, const Type* keyType, const Type* valueType) {
    Group* groups = asGroupsMut(this->groups_);
    size_t hashCode = keyType->hashObj(key);
    auto foundIndex = findScriptImpl(groups, this->groupCount_, hashCode, key, keyType);
    if (foundIndex.hasValue == false) {
        return false;
    } else {
        Group& group = groups[foundIndex.groupIndex];
        group.eraseScript(alloc, foundIndex.valueIndex, keyType, valueType,
                          reinterpret_cast<Group::Header**>(&this->iterFirst_),
                          reinterpret_cast<Group::Header**>(&this->iterLast_));
        this->count_ -= 1;
        this->available_ += 1;
        return true;
    }
}

sy::Result<void, sy::AllocErr> sy::RawMapUnmanaged::ensureCapacityForInsert(Allocator& alloc) {
    if (this->available_ != 0)
        return {};

    const size_t newGroupCount = [](size_t currentGroupCount) -> size_t {
        constexpr size_t DEFAULT_GROUP_COUNT = 1;
        if (currentGroupCount == 0)
            return DEFAULT_GROUP_COUNT;
        return currentGroupCount * 2;
    }(this->groupCount_);

    auto allocResult = alloc.allocArray<Group>(newGroupCount);
    if (allocResult.hasValue() == false) {
        return Error(AllocErr::OutOfMemory);
    }

    Group* newGroups = allocResult.value();
    for (size_t i = 0; i < newGroupCount; i++) {
        auto groupAllocResult = Group::create(alloc);
        if (groupAllocResult.hasValue() == false) {
            for (size_t j = 0; j < i; j++) {
                newGroups[j].freeMemory(alloc);
            }
            alloc.freeArray(newGroups, newGroupCount);
            return Error(AllocErr::OutOfMemory);
        }

        newGroups[i] = groupAllocResult.takeValue();
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
        return {};
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
                return Error(AllocErr::OutOfMemory);
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

    return {};
}

#pragma region Iterators

bool sy::RawMapUnmanaged::Iterator::operator!=(const Iterator& other) {
    return this->currentHeader_ != other.currentHeader_;
}

sy::RawMapUnmanaged::Iterator::Entry sy::RawMapUnmanaged::Iterator::operator*() const {
    Entry e;
    e.header_ = this->currentHeader_;
    return e;
}

sy::RawMapUnmanaged::Iterator& sy::RawMapUnmanaged::Iterator::operator++() {
    Group::Header* header = reinterpret_cast<Group::Header*>(this->currentHeader_);
    this->currentHeader_ = reinterpret_cast<void*>(header->iterAfter);
    return *this;
}

const void* sy::RawMapUnmanaged::Iterator::Entry::key(size_t keyAlign) const {
    Group::Header* header = reinterpret_cast<Group::Header*>(this->header_);
    return header->key(keyAlign);
}

void* sy::RawMapUnmanaged::Iterator::Entry::value(size_t keyAlign, size_t keySize, size_t valueAlign) const {
    Group::Header* header = reinterpret_cast<Group::Header*>(this->header_);
    return header->value(keyAlign, keySize, valueAlign);
}

sy::RawMapUnmanaged::Iterator sy::RawMapUnmanaged::begin() {
    Iterator it;
    it.map_ = this;
    it.currentHeader_ = this->iterFirst_;
    return it;
}

sy::RawMapUnmanaged::Iterator sy::RawMapUnmanaged::end() {
    Iterator it;
    it.map_ = this;
    it.currentHeader_ = nullptr;
    return it;
}

bool sy::RawMapUnmanaged::ConstIterator::operator!=(const ConstIterator& other) {
    return this->currentHeader_ != other.currentHeader_;
}

sy::RawMapUnmanaged::ConstIterator::Entry sy::RawMapUnmanaged::ConstIterator::operator*() const {
    Entry e;
    e.header_ = this->currentHeader_;
    return e;
}

sy::RawMapUnmanaged::ConstIterator& sy::RawMapUnmanaged::ConstIterator::operator++() {
    const Group::Header* header = reinterpret_cast<const Group::Header*>(this->currentHeader_);
    this->currentHeader_ = reinterpret_cast<const void*>(header->iterAfter);
    return *this;
}

const void* sy::RawMapUnmanaged::ConstIterator::Entry::key(size_t keyAlign) const {
    const Group::Header* header = reinterpret_cast<const Group::Header*>(this->header_);
    return header->key(keyAlign);
}

const void* sy::RawMapUnmanaged::ConstIterator::Entry::value(size_t keyAlign, size_t keySize, size_t valueAlign) const {
    const Group::Header* header = reinterpret_cast<const Group::Header*>(this->header_);
    return header->value(keyAlign, keySize, valueAlign);
}

sy::RawMapUnmanaged::ConstIterator sy::RawMapUnmanaged::begin() const {
    ConstIterator it;
    it.map_ = this;
    it.currentHeader_ = this->iterFirst_;
    return it;
}

sy::RawMapUnmanaged::ConstIterator sy::RawMapUnmanaged::end() const {
    ConstIterator it;
    it.map_ = this;
    it.currentHeader_ = nullptr;
    return it;
}

bool sy::RawMapUnmanaged::ReverseIterator::operator!=(const ReverseIterator& other) {
    return this->currentHeader_ != other.currentHeader_;
}

sy::RawMapUnmanaged::ReverseIterator::Entry sy::RawMapUnmanaged::ReverseIterator::operator*() const {
    Entry e;
    e.header_ = this->currentHeader_;
    return e;
}

sy::RawMapUnmanaged::ReverseIterator& sy::RawMapUnmanaged::ReverseIterator::operator++() {
    Group::Header* header = reinterpret_cast<Group::Header*>(this->currentHeader_);
    this->currentHeader_ = reinterpret_cast<void*>(header->iterBefore);
    return *this;
}

const void* sy::RawMapUnmanaged::ReverseIterator::Entry::key(size_t keyAlign) const {
    Group::Header* header = reinterpret_cast<Group::Header*>(this->header_);
    return header->key(keyAlign);
}

void* sy::RawMapUnmanaged::ReverseIterator::Entry::value(size_t keyAlign, size_t keySize, size_t valueAlign) const {
    Group::Header* header = reinterpret_cast<Group::Header*>(this->header_);
    return header->value(keyAlign, keySize, valueAlign);
}

sy::RawMapUnmanaged::ReverseIterator sy::RawMapUnmanaged::rbegin() {
    ReverseIterator it;
    it.map_ = this;
    it.currentHeader_ = this->iterLast_;
    return it;
}

sy::RawMapUnmanaged::ReverseIterator sy::RawMapUnmanaged::rend() {
    ReverseIterator it;
    it.map_ = this;
    it.currentHeader_ = nullptr;
    return it;
}

bool sy::RawMapUnmanaged::ConstReverseIterator::operator!=(const ConstReverseIterator& other) {
    return this->currentHeader_ != other.currentHeader_;
}

sy::RawMapUnmanaged::ConstReverseIterator::Entry sy::RawMapUnmanaged::ConstReverseIterator::operator*() const {
    Entry e;
    e.header_ = this->currentHeader_;
    return e;
}

sy::RawMapUnmanaged::ConstReverseIterator& sy::RawMapUnmanaged::ConstReverseIterator::operator++() {
    const Group::Header* header = reinterpret_cast<const Group::Header*>(this->currentHeader_);
    this->currentHeader_ = reinterpret_cast<const void*>(header->iterBefore);
    return *this;
}

const void* sy::RawMapUnmanaged::ConstReverseIterator::Entry::key(size_t keyAlign) const {
    const Group::Header* header = reinterpret_cast<const Group::Header*>(this->header_);
    return header->key(keyAlign);
}

const void* sy::RawMapUnmanaged::ConstReverseIterator::Entry::value(size_t keyAlign, size_t keySize,
                                                                    size_t valueAlign) const {
    const Group::Header* header = reinterpret_cast<const Group::Header*>(this->header_);
    return header->value(keyAlign, keySize, valueAlign);
}

sy::RawMapUnmanaged::ConstReverseIterator sy::RawMapUnmanaged::rbegin() const {
    ConstReverseIterator it;
    it.map_ = this;
    it.currentHeader_ = this->iterLast_;
    return it;
}

sy::RawMapUnmanaged::ConstReverseIterator sy::RawMapUnmanaged::rend() const {
    ConstReverseIterator it;
    it.map_ = this;
    it.currentHeader_ = nullptr;
    return it;
}

#pragma endregion

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"

using sy::Allocator;
using sy::RawMapUnmanaged;

TEST_CASE("RawMapUnmanaged default construct/destruct") {
    RawMapUnmanaged map;
    CHECK_EQ(map.len(), 0);
}

TEST_CASE("RawMapUnmanaged empty find no value") {
    RawMapUnmanaged map;
    const size_t key = 10;
    auto hashKey = [](const void* k) { return *reinterpret_cast<const size_t*>(k); };
    auto eqKey = [](const void* inKey, const void* potentialMatch) {
        return *reinterpret_cast<const size_t*>(inKey) == *reinterpret_cast<const size_t*>(potentialMatch);
    };

    constexpr size_t KEY_SIZE = sizeof(size_t);
    constexpr size_t KEY_ALIGN = alignof(size_t);
    constexpr size_t VALUE_ALIGN = alignof(float);

    { // const
        auto findResult = map.find(&key, hashKey, eqKey, KEY_ALIGN, KEY_SIZE, VALUE_ALIGN);
        CHECK_FALSE(findResult);
    }
    { // mutable
        auto findResult = map.findMut(&key, hashKey, eqKey, KEY_ALIGN, KEY_SIZE, VALUE_ALIGN);
        CHECK_FALSE(findResult);
    }
    // { // const script
    //     auto findResult = map.findScript(&key, sy::Type::TYPE_USIZE, sy::Type::TYPE_F32);
    //     CHECK_FALSE(findResult);
    // }
    // { // mutable script
    //     auto findResult = map.findMutScript(&key, sy::Type::TYPE_USIZE, sy::Type::TYPE_F32);
    //     CHECK_FALSE(findResult);
    // }
}

TEST_CASE("RawMapUnmanaged insert one simple") {
    RawMapUnmanaged map;
    size_t key = 10;
    auto hashKey = [](const void* k) { return *reinterpret_cast<const size_t*>(k); };
    auto eqKey = [](const void* inKey, const void* potentialMatch) {
        return *reinterpret_cast<const size_t*>(inKey) == *reinterpret_cast<const size_t*>(potentialMatch);
    };
    float value = 5.0;

    constexpr size_t KEY_SIZE = sizeof(size_t);
    constexpr size_t KEY_ALIGN = alignof(size_t);
    constexpr size_t VALUE_SIZE = sizeof(float);
    constexpr size_t VALUE_ALIGN = alignof(float);

    Allocator alloc;
    auto insertResult = map.insert(alloc, nullptr, &key, &value, hashKey, nullptr, nullptr, eqKey, KEY_SIZE, KEY_ALIGN,
                                   VALUE_SIZE, VALUE_ALIGN);

    CHECK(insertResult.hasValue());    // successfully allocated
    CHECK_FALSE(insertResult.value()); // no old value
    CHECK_EQ(map.len(), 1);

    map.destroy(alloc, nullptr, nullptr, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
}

TEST_CASE("RawMapUnmanaged insert one find") {
    RawMapUnmanaged map;
    size_t key = 11;
    auto hashKey = [](const void* k) { return *reinterpret_cast<const size_t*>(k); };
    auto eqKey = [](const void* inKey, const void* potentialMatch) {
        return *reinterpret_cast<const size_t*>(inKey) == *reinterpret_cast<const size_t*>(potentialMatch);
    };
    float value = 5.5;

    constexpr size_t KEY_SIZE = sizeof(size_t);
    constexpr size_t KEY_ALIGN = alignof(size_t);
    constexpr size_t VALUE_SIZE = sizeof(float);
    constexpr size_t VALUE_ALIGN = alignof(float);

    Allocator alloc;
    auto insertResult = map.insert(alloc, nullptr, &key, &value, hashKey, nullptr, nullptr, eqKey, KEY_SIZE, KEY_ALIGN,
                                   VALUE_SIZE, VALUE_ALIGN);

    CHECK(insertResult.hasValue());    // successfully allocated
    CHECK_FALSE(insertResult.value()); // no old value
    CHECK_EQ(map.len(), 1);

    { // check value is
        auto findResult = map.find(&key, hashKey, eqKey, KEY_ALIGN, KEY_SIZE, VALUE_ALIGN);
        CHECK(findResult.has_value());
        const void* foundValue = findResult.value();
        CHECK_EQ(*reinterpret_cast<const float*>(foundValue), 5.5);
    }

    map.destroy(alloc, nullptr, nullptr, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
}

TEST_CASE("RawMapUnmanaged insert two find") {
    auto hashKey = [](const void* k) { return *reinterpret_cast<const size_t*>(k); };
    auto eqKey = [](const void* inKey, const void* potentialMatch) {
        return *reinterpret_cast<const size_t*>(inKey) == *reinterpret_cast<const size_t*>(potentialMatch);
    };

    RawMapUnmanaged map;

    constexpr size_t KEY_SIZE = sizeof(size_t);
    constexpr size_t KEY_ALIGN = alignof(size_t);
    constexpr size_t VALUE_SIZE = sizeof(float);
    constexpr size_t VALUE_ALIGN = alignof(float);

    Allocator alloc;

    for (size_t i = 0; i < 2; i++) {
        float value = 1.0;
        auto insertResult = map.insert(alloc, nullptr, &i, &value, hashKey, nullptr, nullptr, eqKey, KEY_SIZE,
                                       KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);

        CHECK(insertResult.hasValue());    // successfully allocated
        CHECK_FALSE(insertResult.value()); // no old value
    }

    CHECK_EQ(map.len(), 2);

    const void* first;
    const void* second;

    {
        size_t key = 0;
        auto findFirstResult = map.find(&key, hashKey, eqKey, KEY_ALIGN, KEY_SIZE, VALUE_ALIGN);
        CHECK(findFirstResult.has_value());
        first = findFirstResult.value();
        CHECK_EQ(*reinterpret_cast<const float*>(first), 1);
    }
    {
        size_t key = 1;
        auto findSecondResult = map.find(&key, hashKey, eqKey, KEY_ALIGN, KEY_SIZE, VALUE_ALIGN);
        CHECK(findSecondResult.has_value());
        second = findSecondResult.value();
        CHECK_EQ(*reinterpret_cast<const float*>(second), 1);
    }

    CHECK_NE(first, second);

    map.destroy(alloc, nullptr, nullptr, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
}

TEST_CASE("RawMapUnmanaged insert group max capacity") {
    auto hashKey = [](const void* k) { return *reinterpret_cast<const size_t*>(k); };
    auto eqKey = [](const void* inKey, const void* potentialMatch) {
        return *reinterpret_cast<const size_t*>(inKey) == *reinterpret_cast<const size_t*>(potentialMatch);
    };

    RawMapUnmanaged map;

    constexpr size_t KEY_SIZE = sizeof(size_t);
    constexpr size_t KEY_ALIGN = alignof(size_t);
    constexpr size_t VALUE_SIZE = sizeof(float);
    constexpr size_t VALUE_ALIGN = alignof(float);

    Allocator alloc;

    for (size_t i = 0; i < 16; i++) {
        float value = static_cast<float>(i) + 0.1f;
        auto insertResult = map.insert(alloc, nullptr, &i, &value, hashKey, nullptr, nullptr, eqKey, KEY_SIZE,
                                       KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);

        CHECK(insertResult.hasValue());    // successfully allocated
        CHECK_FALSE(insertResult.value()); // no old value
    }

    CHECK_EQ(map.len(), 16);

    for (size_t i = 0; i < 16; i++) {
        auto findResult = map.find(&i, hashKey, eqKey, KEY_ALIGN, KEY_SIZE, VALUE_ALIGN);
        CHECK(findResult.has_value());
        const void* findValue = findResult.value();
        CHECK_EQ(*reinterpret_cast<const float*>(findValue), static_cast<float>(i) + 0.1f);
    }

    map.destroy(alloc, nullptr, nullptr, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
}

TEST_CASE("RawMapUnmanaged insert a lot") {
    auto hashKey = [](const void* k) { return *reinterpret_cast<const size_t*>(k); };
    auto eqKey = [](const void* inKey, const void* potentialMatch) {
        return *reinterpret_cast<const size_t*>(inKey) == *reinterpret_cast<const size_t*>(potentialMatch);
    };

    RawMapUnmanaged map;

    constexpr size_t KEY_SIZE = sizeof(size_t);
    constexpr size_t KEY_ALIGN = alignof(size_t);
    constexpr size_t VALUE_SIZE = sizeof(float);
    constexpr size_t VALUE_ALIGN = alignof(float);

    Allocator alloc;

    for (size_t i = 0; i < 200; i++) {
        float value = static_cast<float>(i);
        auto insertResult = map.insert(alloc, nullptr, &i, &value, hashKey, nullptr, nullptr, eqKey, KEY_SIZE,
                                       KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);

        CHECK(insertResult.hasValue());    // successfully allocated
        CHECK_FALSE(insertResult.value()); // no old value
    }

    CHECK_EQ(map.len(), 200);

    for (size_t i = 0; i < 200; i++) {
        auto findResult = map.find(&i, hashKey, eqKey, KEY_ALIGN, KEY_SIZE, VALUE_ALIGN);
        CHECK(findResult.has_value());
        const void* findValue = findResult.value();
        CHECK_EQ(*reinterpret_cast<const float*>(findValue), static_cast<float>(i));
    }

    map.destroy(alloc, nullptr, nullptr, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
}

TEST_CASE("RawMapUnmanaged erase one") {
    RawMapUnmanaged map;
    size_t key = 11;
    auto hashKey = [](const void* k) { return *reinterpret_cast<const size_t*>(k); };
    auto eqKey = [](const void* inKey, const void* potentialMatch) {
        return *reinterpret_cast<const size_t*>(inKey) == *reinterpret_cast<const size_t*>(potentialMatch);
    };
    float value = 5.5;

    constexpr size_t KEY_SIZE = sizeof(size_t);
    constexpr size_t KEY_ALIGN = alignof(size_t);
    constexpr size_t VALUE_SIZE = sizeof(float);
    constexpr size_t VALUE_ALIGN = alignof(float);

    Allocator alloc;
    (void)map.insert(alloc, nullptr, &key, &value, hashKey, nullptr, nullptr, eqKey, KEY_SIZE, KEY_ALIGN, VALUE_SIZE,
                     VALUE_ALIGN);

    bool didErase =
        map.erase(alloc, &key, hashKey, nullptr, nullptr, eqKey, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
    CHECK(didErase);
    CHECK_EQ(map.len(), 0);

    auto findResult = map.find(&key, hashKey, eqKey, KEY_ALIGN, KEY_SIZE, VALUE_ALIGN);
    CHECK_FALSE(findResult.has_value());

    map.destroy(alloc, nullptr, nullptr, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
}

TEST_CASE("RawMapUnmanaged erase many entire map") {
    auto hashKey = [](const void* k) { return *reinterpret_cast<const size_t*>(k); };
    auto eqKey = [](const void* inKey, const void* potentialMatch) {
        return *reinterpret_cast<const size_t*>(inKey) == *reinterpret_cast<const size_t*>(potentialMatch);
    };

    RawMapUnmanaged map;

    constexpr size_t KEY_SIZE = sizeof(size_t);
    constexpr size_t KEY_ALIGN = alignof(size_t);
    constexpr size_t VALUE_SIZE = sizeof(float);
    constexpr size_t VALUE_ALIGN = alignof(float);

    Allocator alloc;

    for (size_t i = 0; i < 1000; i++) {
        float value = static_cast<float>(i);
        auto insertResult = map.insert(alloc, nullptr, &i, &value, hashKey, nullptr, nullptr, eqKey, KEY_SIZE,
                                       KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);

        CHECK(insertResult.hasValue());    // successfully allocated
        CHECK_FALSE(insertResult.value()); // no old value
    }

    for (size_t i = 0; i < 1000; i++) {
        bool didErase =
            map.erase(alloc, &i, hashKey, nullptr, nullptr, eqKey, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
        CHECK(didErase);
    }

    CHECK_EQ(map.len(), 0);

    for (size_t i = 0; i < 1000; i++) {
        auto findResult = map.find(&i, hashKey, eqKey, KEY_ALIGN, KEY_SIZE, VALUE_ALIGN);
        CHECK_FALSE(findResult.has_value());
    }

    map.destroy(alloc, nullptr, nullptr, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
}

TEST_CASE("RawMapUnmanaged erase many half map") {
    auto hashKey = [](const void* k) { return *reinterpret_cast<const size_t*>(k); };
    auto eqKey = [](const void* inKey, const void* potentialMatch) {
        return *reinterpret_cast<const size_t*>(inKey) == *reinterpret_cast<const size_t*>(potentialMatch);
    };

    RawMapUnmanaged map;

    constexpr size_t KEY_SIZE = sizeof(size_t);
    constexpr size_t KEY_ALIGN = alignof(size_t);
    constexpr size_t VALUE_SIZE = sizeof(float);
    constexpr size_t VALUE_ALIGN = alignof(float);

    Allocator alloc;

    for (size_t i = 0; i < 1000; i++) {
        float value = static_cast<float>(i);
        auto insertResult = map.insert(alloc, nullptr, &i, &value, hashKey, nullptr, nullptr, eqKey, KEY_SIZE,
                                       KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);

        CHECK(insertResult.hasValue());    // successfully allocated
        CHECK_FALSE(insertResult.value()); // no old value
    }

    for (size_t i = 0; i < 500; i++) {
        bool didErase =
            map.erase(alloc, &i, hashKey, nullptr, nullptr, eqKey, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
        CHECK(didErase);
    }

    CHECK_EQ(map.len(), 500);

    for (size_t i = 0; i < 500; i++) {
        auto findResult = map.find(&i, hashKey, eqKey, KEY_ALIGN, KEY_SIZE, VALUE_ALIGN);
        CHECK_FALSE(findResult.has_value());
    }

    for (size_t i = 500; i < 1000; i++) {
        auto findResult = map.find(&i, hashKey, eqKey, KEY_ALIGN, KEY_SIZE, VALUE_ALIGN);
        CHECK(findResult.has_value());
        const void* findValue = findResult.value();
        CHECK_EQ(*reinterpret_cast<const float*>(findValue), static_cast<float>(i));
    }

    map.destroy(alloc, nullptr, nullptr, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
}

TEST_CASE("RawMapUnmanaged erase entry that isn't in map") {
    RawMapUnmanaged map;
    size_t key = 11;
    auto hashKey = [](const void* k) { return *reinterpret_cast<const size_t*>(k); };
    auto eqKey = [](const void* inKey, const void* potentialMatch) {
        return *reinterpret_cast<const size_t*>(inKey) == *reinterpret_cast<const size_t*>(potentialMatch);
    };
    float value = 5.5;

    constexpr size_t KEY_SIZE = sizeof(size_t);
    constexpr size_t KEY_ALIGN = alignof(size_t);
    constexpr size_t VALUE_SIZE = sizeof(float);
    constexpr size_t VALUE_ALIGN = alignof(float);

    Allocator alloc;
    (void)map.insert(alloc, nullptr, &key, &value, hashKey, nullptr, nullptr, eqKey, KEY_SIZE, KEY_ALIGN, VALUE_SIZE,
                     VALUE_ALIGN);

    size_t nonExistantKey = 12;
    bool didErase = map.erase(alloc, &nonExistantKey, hashKey, nullptr, nullptr, eqKey, KEY_SIZE, KEY_ALIGN, VALUE_SIZE,
                              VALUE_ALIGN);
    CHECK_FALSE(didErase);
    CHECK_EQ(map.len(), 1);

    auto findResult = map.find(&key, hashKey, eqKey, KEY_ALIGN, KEY_SIZE, VALUE_ALIGN);
    CHECK(findResult.has_value());

    map.destroy(alloc, nullptr, nullptr, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
}

template <typename T> struct ComplexType {
    T* ptr;
    inline static int aliveCount = 0;

    ComplexType(T in) {
        ptr = new T(in);
        aliveCount += 1;
    }

    static void destroy(void* self) { reinterpret_cast<ComplexType*>(self)->destroyImpl(); }

    static size_t hash(const void* self) { return reinterpret_cast<const ComplexType*>(self)->hashImpl(); }

    static bool eql(const void* self, const void* potentialMatch) {
        return reinterpret_cast<const ComplexType*>(self)->eqlImpl(
            reinterpret_cast<const ComplexType*>(potentialMatch));
    }

  private:
    void destroyImpl() {
        delete ptr;
        aliveCount -= 1;
    }

    size_t hashImpl() const { return *ptr; }

    bool eqlImpl(const ComplexType* other) const { return (*(this->ptr)) == (*(other->ptr)); }
};

TEST_CASE("RawMapUnmanaged properly calls key and value destructors") {
    RawMapUnmanaged map;

    using KeyT = ComplexType<size_t>;
    using ValueT = ComplexType<float>;

    constexpr size_t KEY_SIZE = sizeof(KeyT);
    constexpr size_t KEY_ALIGN = alignof(KeyT);
    constexpr size_t VALUE_SIZE = sizeof(ValueT);
    constexpr size_t VALUE_ALIGN = alignof(ValueT);

    Allocator alloc;

    for (size_t i = 0; i < 1500; i++) {
        KeyT key(i);
        ValueT value(static_cast<float>(i) + 0.1f);
        (void)map.insert(alloc, nullptr, &key, &value, key.hash, key.destroy, value.destroy, key.eql, KEY_SIZE,
                         KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
    }

    CHECK_EQ(KeyT::aliveCount, 1500);
    CHECK_EQ(ValueT::aliveCount, 1500);

    { // remove entry
        KeyT key(0);
        CHECK(map.erase(alloc, &key, key.hash, key.destroy, ValueT::destroy, key.eql, KEY_SIZE, KEY_ALIGN, VALUE_SIZE,
                        VALUE_ALIGN));
        KeyT::destroy(&key);
    }

    CHECK_EQ(KeyT::aliveCount, 1499);
    CHECK_EQ(ValueT::aliveCount, 1499);

    map.destroy(alloc, KeyT::destroy, ValueT::destroy, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);

    CHECK_EQ(KeyT::aliveCount, 0);
    CHECK_EQ(ValueT::aliveCount, 0);
}

TEST_CASE("RawMapUnmanaged mutable iterator") {
    auto hashKey = [](const void* k) { return *reinterpret_cast<const size_t*>(k); };
    auto eqKey = [](const void* inKey, const void* potentialMatch) {
        return *reinterpret_cast<const size_t*>(inKey) == *reinterpret_cast<const size_t*>(potentialMatch);
    };

    RawMapUnmanaged map;

    constexpr size_t KEY_SIZE = sizeof(size_t);
    constexpr size_t KEY_ALIGN = alignof(size_t);
    constexpr size_t VALUE_SIZE = sizeof(float);
    constexpr size_t VALUE_ALIGN = alignof(float);

    Allocator alloc;

    for (size_t i = 0; i < 300; i++) {
        float value = static_cast<float>(i);
        auto insertResult = map.insert(alloc, nullptr, &i, &value, hashKey, nullptr, nullptr, eqKey, KEY_SIZE,
                                       KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);

        CHECK(insertResult.hasValue());    // successfully allocated
        CHECK_FALSE(insertResult.value()); // no old value
    }

    {
        size_t iter = 0;
        for (auto entry : map) {
            const size_t* key = reinterpret_cast<const size_t*>(entry.key(KEY_ALIGN));
            float* value = reinterpret_cast<float*>(entry.value(KEY_ALIGN, KEY_SIZE, VALUE_ALIGN));
            CHECK_EQ(*key, iter);
            *value += 0.1f;
            iter += 1;
        }
        CHECK_EQ(iter, 300);
    }

    {
        size_t iter = 0;
        for (auto entry : map) {
            const size_t* key = reinterpret_cast<const size_t*>(entry.key(KEY_ALIGN));
            float* value = reinterpret_cast<float*>(entry.value(KEY_ALIGN, KEY_SIZE, VALUE_ALIGN));
            CHECK_EQ(*key, iter);
            CHECK_EQ(*value, static_cast<float>(iter) + 0.1f);
            iter += 1;
        }
    }

    map.destroy(alloc, nullptr, nullptr, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
}

TEST_CASE("RawMapUnmanaged const iterator") {
    auto hashKey = [](const void* k) { return *reinterpret_cast<const size_t*>(k); };
    auto eqKey = [](const void* inKey, const void* potentialMatch) {
        return *reinterpret_cast<const size_t*>(inKey) == *reinterpret_cast<const size_t*>(potentialMatch);
    };

    RawMapUnmanaged map;

    constexpr size_t KEY_SIZE = sizeof(size_t);
    constexpr size_t KEY_ALIGN = alignof(size_t);
    constexpr size_t VALUE_SIZE = sizeof(float);
    constexpr size_t VALUE_ALIGN = alignof(float);

    Allocator alloc;

    for (size_t i = 0; i < 300; i++) {
        float value = static_cast<float>(i);
        auto insertResult = map.insert(alloc, nullptr, &i, &value, hashKey, nullptr, nullptr, eqKey, KEY_SIZE,
                                       KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);

        CHECK(insertResult.hasValue());    // successfully allocated
        CHECK_FALSE(insertResult.value()); // no old value
    }

    [=](const RawMapUnmanaged& m) {
        size_t iter = 0;
        for (auto entry : m) {
            const size_t* key = reinterpret_cast<const size_t*>(entry.key(KEY_ALIGN));
            const float* value = reinterpret_cast<const float*>(entry.value(KEY_ALIGN, KEY_SIZE, VALUE_ALIGN));
            CHECK_EQ(*key, iter);
            CHECK_EQ(*value, static_cast<float>(iter));
            iter += 1;
        }
        CHECK_EQ(iter, 300);
    }(map);

    map.destroy(alloc, nullptr, nullptr, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
}

TEST_CASE("RawMapUnmanaged mutable reverse iterator") {
    auto hashKey = [](const void* k) { return *reinterpret_cast<const size_t*>(k); };
    auto eqKey = [](const void* inKey, const void* potentialMatch) {
        return *reinterpret_cast<const size_t*>(inKey) == *reinterpret_cast<const size_t*>(potentialMatch);
    };

    RawMapUnmanaged map;

    constexpr size_t KEY_SIZE = sizeof(size_t);
    constexpr size_t KEY_ALIGN = alignof(size_t);
    constexpr size_t VALUE_SIZE = sizeof(float);
    constexpr size_t VALUE_ALIGN = alignof(float);

    Allocator alloc;

    for (size_t i = 0; i < 300; i++) {
        float value = static_cast<float>(i);
        auto insertResult = map.insert(alloc, nullptr, &i, &value, hashKey, nullptr, nullptr, eqKey, KEY_SIZE,
                                       KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);

        CHECK(insertResult.hasValue());    // successfully allocated
        CHECK_FALSE(insertResult.value()); // no old value
    }

    {
        size_t iter = 300;
        for (auto riter = map.rbegin(); riter != map.rend(); riter++) {
            auto entry = *riter;
            const size_t* key = reinterpret_cast<const size_t*>(entry.key(KEY_ALIGN));
            float* value = reinterpret_cast<float*>(entry.value(KEY_ALIGN, KEY_SIZE, VALUE_ALIGN));
            CHECK_EQ(*key, iter - 1);
            *value += 0.1f;
            iter -= 1;
        }
        CHECK_EQ(iter, 0);
    }

    {
        size_t iter = 0;
        for (auto entry : map) {
            const size_t* key = reinterpret_cast<const size_t*>(entry.key(KEY_ALIGN));
            float* value = reinterpret_cast<float*>(entry.value(KEY_ALIGN, KEY_SIZE, VALUE_ALIGN));
            CHECK_EQ(*key, iter);
            CHECK_EQ(*value, static_cast<float>(iter) + 0.1f);
            iter += 1;
        }
        CHECK_EQ(iter, 300);
    }

    map.destroy(alloc, nullptr, nullptr, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
}

TEST_CASE("RawMapUnmanaged const reverse iterator") {
    auto hashKey = [](const void* k) { return *reinterpret_cast<const size_t*>(k); };
    auto eqKey = [](const void* inKey, const void* potentialMatch) {
        return *reinterpret_cast<const size_t*>(inKey) == *reinterpret_cast<const size_t*>(potentialMatch);
    };

    RawMapUnmanaged map;

    constexpr size_t KEY_SIZE = sizeof(size_t);
    constexpr size_t KEY_ALIGN = alignof(size_t);
    constexpr size_t VALUE_SIZE = sizeof(float);
    constexpr size_t VALUE_ALIGN = alignof(float);

    Allocator alloc;

    for (size_t i = 0; i < 300; i++) {
        float value = static_cast<float>(i);
        auto insertResult = map.insert(alloc, nullptr, &i, &value, hashKey, nullptr, nullptr, eqKey, KEY_SIZE,
                                       KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);

        CHECK(insertResult.hasValue());    // successfully allocated
        CHECK_FALSE(insertResult.value()); // no old value
    }

    [=](const RawMapUnmanaged& m) {
        size_t iter = 300;
        for (auto riter = m.rbegin(); riter != m.rend(); riter++) {
            auto entry = *riter;
            const size_t* key = reinterpret_cast<const size_t*>(entry.key(KEY_ALIGN));
            const float* value = reinterpret_cast<const float*>(entry.value(KEY_ALIGN, KEY_SIZE, VALUE_ALIGN));
            CHECK_EQ(*key, iter - 1);
            CHECK_EQ(*value, static_cast<float>(iter - 1));
            iter -= 1;
        }
        CHECK_EQ(iter, 0);
    }(map);

    map.destroy(alloc, nullptr, nullptr, KEY_SIZE, KEY_ALIGN, VALUE_SIZE, VALUE_ALIGN);
}

#endif
