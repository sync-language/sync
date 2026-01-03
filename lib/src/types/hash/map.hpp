//! API
#pragma once
#ifndef SY_TYPES_HASH_MAP_HPP_
#define SY_TYPES_HASH_MAP_HPP_

#include "../../core/core.h"
#include "../../mem/allocator.hpp"
#include "../option/option.hpp"
#include "../template_type_operations.hpp"
#include <new>
#include <optional>
#include <type_traits>
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

    [[nodiscard]] Result<bool, AllocErr>
    insertCustomMove(Allocator& alloc, void* optionalOldValue, void* key, void* value, size_t (*hash)(const void* key),
                     void (*destructKey)(void* ptr), void (*destructValue)(void* ptr),
                     bool (*eq)(const void* searchKey, const void* found),
                     void (*keyMoveConstruct)(void* dst, void* src), void (*valueMoveConstruct)(void* dst, void* src),
                     size_t keySize, size_t keyAlign, size_t valueSize, size_t valueAlign);

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

template <typename K, typename V> class MapUnmanaged final {

  public:
    MapUnmanaged() = default;

    ~MapUnmanaged() noexcept = default;

    void destroy(Allocator& alloc) noexcept;

    [[nodiscard]] size_t len() const { return this->inner_.len(); }

    [[nodiscard]] Option<V&> find(const K& key) noexcept;

    [[nodiscard]] Option<const V&> find(const K& key) const noexcept;

    [[nodiscard]] Result<Option<V>, AllocErr> insert(Allocator& alloc, K&& key, V&& value) noexcept;

    [[nodiscard]] Result<Option<V>, AllocErr> insert(Allocator& alloc, const K& key, V&& value) noexcept;

    [[nodiscard]] Result<Option<V>, AllocErr> insert(Allocator& alloc, K&& key, const V& value) noexcept;

    [[nodiscard]] Result<Option<V>, AllocErr> insert(Allocator& alloc, const K& key, const V& value) noexcept;

    bool erase(Allocator& alloc, const K& key) noexcept;

    class SY_API Iterator final {
        friend class MapUnmanaged;
        RawMapUnmanaged::Iterator iter_;
        Iterator(RawMapUnmanaged::Iterator it) : iter_(it) {}

      public:
        struct SY_API Entry final {
            const K& key;
            V& value;
        };

        bool operator!=(const Iterator& other) { return this->iter_ != other.iter_; }
        Entry operator*() const {
            auto entry = *this->iter_;
            return {
                *reinterpret_cast<const K*>(entry.key(alignof(K))),
                *reinterpret_cast<V*>(entry.value(alignof(K), sizeof(K), alignof(V))),
            };
        }
        Iterator& operator++() {
            ++(this->iter_);
            return *this;
        }
        Iterator& operator++(int) {
            ++(*this);
            return *this;
        };
    };

    Iterator begin() { return Iterator(this->inner_.begin()); }

    Iterator end() { return Iterator(this->inner_.end()); }

    class SY_API ConstIterator final {
        friend class MapUnmanaged;
        RawMapUnmanaged::ConstIterator iter_;
        ConstIterator(RawMapUnmanaged::ConstIterator it) : iter_(it) {}

      public:
        struct SY_API Entry final {
            const K& key;
            const V& value;
        };

        bool operator!=(const ConstIterator& other) { return this->iter_ != other.iter_; }
        Entry operator*() const {
            auto entry = *this->iter_;
            return {
                *reinterpret_cast<const K*>(entry.key(alignof(K))),
                *reinterpret_cast<const V*>(entry.value(alignof(K), sizeof(K), alignof(V))),
            };
        }
        ConstIterator& operator++() {
            ++(this->iter_);
            return *this;
        }
        ConstIterator& operator++(int) {
            ++(*this);
            return *this;
        };
    };

    ConstIterator begin() const { return ConstIterator(this->inner_.begin()); }

    ConstIterator end() const { return ConstIterator(this->inner_.end()); }

    class SY_API ReverseIterator final {
        friend class MapUnmanaged;
        RawMapUnmanaged::ReverseIterator iter_;
        ReverseIterator(RawMapUnmanaged::ReverseIterator it) : iter_(it) {}

      public:
        struct SY_API Entry final {
            const K& key;
            V& value;
        };

        bool operator!=(const ReverseIterator& other) { return this->iter_ != other.iter_; }
        Entry operator*() const {
            auto entry = *this->iter_;
            return {
                *reinterpret_cast<const K*>(entry.key(alignof(K))),
                *reinterpret_cast<V*>(entry.value(alignof(K), sizeof(K), alignof(V))),
            };
        }
        ReverseIterator& operator++() {
            ++(this->iter_);
            return *this;
        }
        ReverseIterator& operator++(int) {
            ++(*this);
            return *this;
        };
    };

    ReverseIterator rbegin() { return ReverseIterator(this->inner_.rbegin()); }

    ReverseIterator rend() { return ReverseIterator(this->inner_.rend()); }

    class SY_API ConstReverseIterator final {
        friend class MapUnmanaged;
        RawMapUnmanaged::ConstReverseIterator iter_;
        ConstReverseIterator(RawMapUnmanaged::ConstReverseIterator it) : iter_(it) {}

      public:
        struct SY_API Entry final {
            const K& key;
            const V& value;
        };

        bool operator!=(const ConstReverseIterator& other) { return this->iter_ != other.iter_; }
        Entry operator*() const {
            auto entry = *this->iter_;
            return {
                *reinterpret_cast<const K*>(entry.key(alignof(K))),
                *reinterpret_cast<const V*>(entry.value(alignof(K), sizeof(K), alignof(V))),
            };
        }
        ConstReverseIterator& operator++() {
            ++(this->iter_);
            return *this;
        }
        ConstReverseIterator& operator++(int) {
            ++(*this);
            return *this;
        };
    };

    ConstReverseIterator rbegin() const { return ConstReverseIterator(this->inner_.rbegin()); }

    ConstReverseIterator rend() const { return ConstReverseIterator(this->inner_.rend()); }

  private:
    constexpr static detail::DestructFn keyDestruct = detail::makeDestructor<K>();
    constexpr static detail::DestructFn valueDestruct = detail::makeDestructor<V>();
    constexpr static detail::HashKeyFn hashKey = detail::makeHashKey<K>();
    constexpr static detail::EqualKeyFn equalKey = detail::makeEqualKey<K>();
    constexpr static detail::MoveConstructFn keyMoveConstruct = detail::makeMoveConstructor<K>();
    constexpr static detail::MoveConstructFn valueMoveConstruct = detail::makeMoveConstructor<V>();
    constexpr static bool keyTriviallyCopyable = std::is_trivially_copyable_v<K>;
    constexpr static bool valueTriviallyCopyable = std::is_trivially_copyable_v<V>;

  private:
    RawMapUnmanaged inner_;
};

template <typename K, typename V> void MapUnmanaged<K, V>::destroy(Allocator& alloc) noexcept {
    this->inner_.destroy(alloc, keyDestruct, valueDestruct, sizeof(K), alignof(K), sizeof(V), alignof(V));
}

template <typename K, typename V> inline Option<V&> MapUnmanaged<K, V>::find(const K& key) noexcept {
    std::optional<void*> found = this->inner_.findMut(&key, hashKey, equalKey, alignof(K), sizeof(K), alignof(V));
    if (found.has_value()) {
        void* ptr = found.value();
        return *reinterpret_cast<V*>(ptr);
    }
    return {};
}

template <typename K, typename V> inline Option<const V&> MapUnmanaged<K, V>::find(const K& key) const noexcept {
    std::optional<const void*> found = this->inner_.find(&key, hashKey, equalKey, alignof(K), sizeof(K), alignof(V));
    if (found.has_value()) {
        const void* ptr = found.value();
        return *reinterpret_cast<const V*>(ptr);
    }
    return {};
}

template <typename K, typename V>
inline Result<Option<V>, AllocErr> MapUnmanaged<K, V>::insert(Allocator& alloc, K&& key, V&& value) noexcept {
    K* keyPtr = &key;
    V* valuePtr = &value;
    alignas(alignof(V)) uint8_t outBuffer[sizeof(V)];
    Result<bool, AllocErr> result = [this, keyPtr, valuePtr, &outBuffer, &alloc]() {
        if constexpr (keyTriviallyCopyable && valueTriviallyCopyable) {
            return this->inner_.insert(alloc, outBuffer, keyPtr, valuePtr, hashKey, keyDestruct, valueDestruct,
                                       equalKey, sizeof(K), alignof(K), sizeof(V), alignof(V));
        } else {
            return this->inner_.insertCustomMove(alloc, outBuffer, keyPtr, valuePtr, hashKey, keyDestruct,
                                                 valueDestruct, equalKey, keyMoveConstruct, valueMoveConstruct,
                                                 sizeof(K), alignof(K), sizeof(V), alignof(V));
        }
    }();

    if (result.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    bool replacedValue = result.value();
    if (replacedValue) {
        return Option<V>(std::move(*reinterpret_cast<V*>(outBuffer)));
    } else {
        return Option<V>();
    }
}

template <typename K, typename V>
inline Result<Option<V>, AllocErr> MapUnmanaged<K, V>::insert(Allocator& alloc, const K& key, V&& value) noexcept {
    K k = key;
    return this->insert(alloc, std::move(k), std::move(value));
}

template <typename K, typename V>
inline Result<Option<V>, AllocErr> MapUnmanaged<K, V>::insert(Allocator& alloc, K&& key, const V& value) noexcept {
    V v = value;
    return this->insert(alloc, std::move(key), std::move(v));
}

template <typename K, typename V>
inline Result<Option<V>, AllocErr> MapUnmanaged<K, V>::insert(Allocator& alloc, const K& key, const V& value) noexcept {
    K k = key;
    V v = value;
    return this->insert(alloc, std::move(k), std::move(v));
}

template <typename K, typename V> inline bool MapUnmanaged<K, V>::erase(Allocator& alloc, const K& key) noexcept {
    return this->inner_.erase(alloc, &key, hashKey, keyDestruct, valueDestruct, equalKey, sizeof(K), alignof(K),
                              sizeof(V), alignof(V));
}

} // namespace sy

#endif // SY_TYPES_HASH_MAP_HPP_