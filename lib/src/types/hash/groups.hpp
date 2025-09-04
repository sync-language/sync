#pragma once
#ifndef SY_TYPES_HASH_GROUPS_HPP_
#define SY_TYPES_HASH_GROUPS_HPP_

#include "../../core.h"
#include "../../mem/allocator.hpp"
#include "../../util/simd.hpp"
#include "../type_info.hpp"

class Group {
  public:
    struct Header;
    /// Doubly linked list of the items within the hash map/set.
    /// The header is data before the actual relevant key and optional value.
    struct Header {
        size_t hashCode;
        Header* iterBefore;
        Header* iterAfter;

        void* key(size_t keyAlign);

        const void* key(size_t keyAlign) const;

        void* value(size_t keyAlign, size_t keySize, size_t valueAlign);

        const void* value(size_t keyAlign, size_t keySize, size_t valueAlign) const;

        void destroyKeyOnly(sy::Allocator alloc, void (*destruct)(void* key), size_t keyAlign, size_t keySize);

        void destroyScriptKeyOnly(sy::Allocator alloc, const sy::Type* type);

        void destroyKeyValue(sy::Allocator alloc, void (*destructKey)(void* key), void (*destructValue)(void* value),
                             size_t keyAlign, size_t keySize, size_t valueAlign, size_t valueSize);

        void destroyScriptKeyValue(sy::Allocator alloc, const sy::Type* keyType, const sy::Type* valueType);

        static sy::AllocExpect<Header*> createKeyOnly(sy::Allocator alloc, size_t keyAlign, size_t keySize);

        static sy::AllocExpect<Header*> createKeyValue(sy::Allocator alloc, size_t keyAlign, size_t keySize,
                                                       size_t valueAlign, size_t valueSize);
    };

    static sy::AllocExpect<Group> create(sy::Allocator& alloc);

    void freeMemory(sy::Allocator& alloc);

    void destroyHeadersKeyOnly(sy::Allocator alloc, void (*destruct)(void* key), size_t keyAlign, size_t keySize);

    void destroyHeadersScriptKeyOnly(sy::Allocator alloc, const sy::Type* type);

    void destroyHeadersKeyValue(sy::Allocator alloc, void (*destructKey)(void* key), void (*destructValue)(void* value),
                                size_t keyAlign, size_t keySize, size_t valueAlign, size_t valueSize);

    void destroyHeadersScriptKeyValue(sy::Allocator alloc, const sy::Type* keyType, const sy::Type* valueType);

    ByteSimd<16>* hashMasks() { return reinterpret_cast<ByteSimd<16>*>(this->hashMasks_); };

    const ByteSimd<16>* hashMasks() const { return reinterpret_cast<const ByteSimd<16>*>(this->hashMasks_); };

    uint32_t simdHashMaskCount() const { return this->capacity_ / 16; }

    Header** headers();

    const Header* const* headers() const;

    struct IndexBitmask {
        static constexpr size_t BITMASK = ~0b01111111ULL;
        size_t value;

        constexpr IndexBitmask(size_t hashCode) : value((hashCode & BITMASK) >> 7) {}
    };

    struct PairBitmask {
        static constexpr uint8_t BITMASK = 0b01111111;
        uint8_t value;

        constexpr PairBitmask(size_t hashCode) : value((hashCode & BITMASK) | 0b10000000) {}
    };

    std::optional<uint32_t> find(PairBitmask pair, const void* inKey, bool (*eq)(const void* key, const void* found),
                                 size_t keyAlign) const;

    sy::AllocExpect<void> ensureCapacityFor(sy::Allocator& alloc, uint32_t minCapacity);

    void setMaskAt(uint32_t index, PairBitmask pairMask);

    const uint8_t* hashMasksAsBytes() const { return hashMasks_; }

    uint32_t itemCount() const { return itemCount_; }

    void setItemCount(uint32_t newCount) { itemCount_ = newCount; }

    uint32_t capacity() const { return capacity_; }

    sy::AllocExpect<void> insertKeyValue(sy::Allocator& alloc, void* key, void* value, size_t hashCode, size_t keySize,
                                         size_t keyAlign, size_t valueSize, size_t valueAlign, Header** iterFirst,
                                         Header** iterLast);

    std::optional<uint32_t> firstZeroIndex() const;

    void erase(sy::Allocator& alloc, uint32_t index, void (*destructKey)(void* ptr), void (*destructValue)(void* ptr),
               size_t keySize, size_t keyAlign, size_t valueSize, size_t valueAlign, Header** iterFirst,
               Header** iterLast);

  private:
    uint8_t* hashMasks_;
    uint32_t capacity_;
    uint32_t itemCount_;
};

inline constexpr size_t calculateLoadFactor(size_t totalEntryCapacity) {
    // load factor of 0.8
    return (totalEntryCapacity * 4) / 5;
}

#endif // SY_TYPES_HASH_GROUPS_HPP_
