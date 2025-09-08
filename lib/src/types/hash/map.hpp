//! API
#pragma once
#ifndef SY_TYPES_HASH_MAP_HPP_
#define SY_TYPES_HASH_MAP_HPP_

#include "../../core.h"
#include "../../mem/allocator.hpp"
#include <optional>
#include <utility>

namespace sy {
class Type;

class SY_API RawMapUnmanaged final {
  public:
    RawMapUnmanaged() = default;

    ~RawMapUnmanaged() noexcept;

    void destroy(Allocator& alloc, void (*destructKey)(void* ptr), void (*destructValue)(void* ptr), size_t keySize,
                 size_t keyAlign, size_t valueSize, size_t valueAlign) noexcept;

    void destroyScript(Allocator& alloc, const Type* keyType, const Type* valueType) noexcept;

    [[nodiscard]] size_t len() const { return this->count_; }

    [[nodiscard]] std::optional<const void*> find(const void* key, size_t (*hash)(const void* key),
                                                  bool (*eq)(const void* key, const void* found), size_t keyAlign,
                                                  size_t keySize, size_t valueAlign) const noexcept;

    [[nodiscard]] std::optional<const void*> findScript(const void* key, const Type* keyType,
                                                        const Type* valueType) const noexcept;

    [[nodiscard]] std::optional<void*> findMut(const void* key, size_t (*hash)(const void* key),
                                               bool (*eq)(const void* key, const void* found), size_t keyAlign,
                                               size_t keySize, size_t valueAlign) noexcept;

    [[nodiscard]] std::optional<void*> findMutScript(const void* key, const Type* keyType,
                                                     const Type* valueType) noexcept;

    [[nodiscard]] Result<bool, AllocErr> insert(Allocator& alloc, void* optionalOldValue, void* key, void* value,
                                                size_t (*hash)(const void* key), void (*destructKey)(void* ptr),
                                                void (*destructValue)(void* ptr),
                                                bool (*eq)(const void* searchKey, const void* found), size_t keySize,
                                                size_t keyAlign, size_t valueSize, size_t valueAlign) noexcept;

    [[nodiscard]] Result<bool, AllocErr> insertScript(Allocator& alloc, void* optionalOldValue, void* key, void* value,
                                                      const Type* keyType, const Type* valueType);

    bool erase(Allocator& alloc, const void* key, size_t (*hash)(const void* key), void (*destructKey)(void* ptr),
               void (*destructValue)(void* ptr), bool (*eq)(const void* searchKey, const void* found), size_t keySize,
               size_t keyAlign, size_t valueSize, size_t valueAlign) noexcept;

    bool eraseScript(Allocator& alloc, const void* key, const Type* keyType, const Type* valueType);

    class SY_API Iterator final {
        friend class RawMapUnmanaged;
        RawMapUnmanaged* map_;
        void* currentHeader_;

      public:
        class SY_API Entry final {
            friend class Iterator;
            void* header_;

          public:
            const void* key(size_t keyAlign) const;
            void* value(size_t keyAlign, size_t keySize, size_t valueAlign) const;
        };

        bool operator!=(const Iterator& other);
        Entry operator*() const;
        Iterator& operator++();
        Iterator& operator++(int) {
            ++(*this);
            return *this;
        };
    };

    Iterator begin();

    Iterator end();

    class SY_API ConstIterator final {
        friend class RawMapUnmanaged;
        const RawMapUnmanaged* map_;
        const void* currentHeader_;

      public:
        class SY_API Entry final {
            friend class ConstIterator;
            const void* header_;

          public:
            const void* key(size_t keyAlign) const;
            const void* value(size_t keyAlign, size_t keySize, size_t valueAlign) const;
        };

        bool operator!=(const ConstIterator& other);
        Entry operator*() const;
        ConstIterator& operator++();
        ConstIterator& operator++(int) {
            ++(*this);
            return *this;
        };
    };

    ConstIterator begin() const;

    ConstIterator end() const;

    class SY_API ReverseIterator final {
        friend class RawMapUnmanaged;
        RawMapUnmanaged* map_;
        void* currentHeader_;

      public:
        class SY_API Entry final {
            friend class ReverseIterator;
            void* header_;

          public:
            const void* key(size_t keyAlign) const;
            void* value(size_t keyAlign, size_t keySize, size_t valueAlign) const;
        };

        bool operator!=(const ReverseIterator& other);
        Entry operator*() const;
        ReverseIterator& operator++();
        ReverseIterator& operator++(int) {
            ++(*this);
            return *this;
        };
    };

    ReverseIterator rbegin();

    ReverseIterator rend();

    class SY_API ConstReverseIterator final {
        friend class RawMapUnmanaged;
        const RawMapUnmanaged* map_;
        const void* currentHeader_;

      public:
        class SY_API Entry final {
            friend class ConstReverseIterator;
            const void* header_;

          public:
            const void* key(size_t keyAlign) const;
            const void* value(size_t keyAlign, size_t keySize, size_t valueAlign) const;
        };

        bool operator!=(const ConstReverseIterator& other);
        Entry operator*() const;
        ConstReverseIterator& operator++();
        ConstReverseIterator& operator++(int) {
            ++(*this);
            return *this;
        };
    };

    ConstReverseIterator rbegin() const;

    ConstReverseIterator rend() const;

  private:
    friend class Iterator;

    Result<void, AllocErr> ensureCapacityForInsert(Allocator& alloc);

  private:
    size_t count_ = 0;
    void* groups_ = nullptr;
    size_t groupCount_ = 0;
    size_t available_ = 0;
    void* iterFirst_ = nullptr;
    void* iterLast_ = nullptr;
};
} // namespace sy

#endif // SY_TYPES_HASH_MAP_HPP_