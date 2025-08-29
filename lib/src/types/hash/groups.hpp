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
    Header *iterBefore;
    Header *iterAfter;

    void *key(size_t keyAlign);

    const void *key(size_t keyAlign) const;

    void *value(size_t keyAlign, size_t keySize, size_t valueAlign);

    const void *value(size_t keyAlign, size_t keySize, size_t valueAlign) const;

    void destroyKeyOnly(sy::Allocator alloc, void (*destruct)(void *key),
                        size_t keyAlign, size_t keySize);

    void destroyScriptKeyOnly(sy::Allocator alloc, const sy::Type *type);

    void destroyKeyValue(sy::Allocator alloc, void (*destructKey)(void *key),
                         void (*destructValue)(void *value), size_t keyAlign,
                         size_t keySize, size_t valueAlign, size_t valueSize);

    void destroyScriptKeyValue(sy::Allocator alloc, const sy::Type *keyType,
                               const sy::Type *valueType);

    static sy::AllocExpect<Header *>
    createKeyOnly(sy::Allocator alloc, size_t keyAlign, size_t keySize);

    static sy::AllocExpect<Header *>
    createKeyValue(sy::Allocator alloc, size_t keyAlign, size_t keySize,
                   size_t valueAlign, size_t valueSize);
  };

  ByteSimd<16> *hashMasks() {
    return reinterpret_cast<ByteSimd<16> *>(this->hashMasks_);
  };

  const ByteSimd<16> *hashMasks() const {
    return reinterpret_cast<const ByteSimd<16> *>(this->hashMasks_);
  };

  uint32_t simdHashMaskCount() const { return this->capacity_ / 16; }

  Header **headers();

  const Header *const *headers() const;

private:
  uint8_t *hashMasks_;
  // uint32_t itemCount_;
  uint32_t capacity_;
};

#endif // SY_TYPES_HASH_GROUPS_HPP_
