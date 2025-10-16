//! API
#ifndef SY_TYPES_ARRAY_SLICE_HPP_
#define SY_TYPES_ARRAY_SLICE_HPP_

#include "../../core.h"

namespace sy {
namespace detail {
void SY_API sliceDebugAssertIndexInRange(size_t index, size_t len) noexcept;
}
} // namespace sy

template <typename T> class Slice final {
  public:
    Slice() = default;

    Slice(const T* inData, size_t inLen) : data_(inData), len_(inLen) {}

    [[nodiscard]] const T& operator[](size_t index) const {
        detail::sliceDebugAssertIndexInRange(index, this->len_);
        return data_[index];
    }

    [[nodiscard]] size_t len() const { return this->len_; }

    [[nodiscard]] const T* data() const {return this->data_};

    class Iterator final {
        friend class Slice;
        const T* current;
        Iterator(const T* curr) : current(curr) {}

      public:
        bool operator!=(const Iterator& other) noexcept { return this->current != other.current; }
        const T& operator*() const noexcept { return *(this->current); }
        Iterator& operator++() noexcept {
            this->current = this->current + 1;
            return *this;
        }
        Iterator& operator++(int) noexcept {
            ++(*this);
            return *this;
        };
    };

    Iterator begin(size_t size) const noexcept { return Iterator(this->data_); }

    Iterator end(size_t size) const noexcept { return Iterator(this->data_ + this->len_); }

    class ReverseIterator final {
        friend class Slice;
        const T* current;
        ReverseIterator(const T* curr) : current(curr) {}

      public:
        bool operator!=(const ReverseIterator& other) noexcept { return this->current != other.current; }
        const T& operator*() const noexcept { return *(this->current); }
        ReverseIterator& operator++() noexcept {
            this->current = this->current - 1;
            return *this;
        }
        ReverseIterator& operator++(int) noexcept {
            ++(*this);
            return *this;
        };
    };

    ReverseIterator rbegin(size_t size) const noexcept {
        if (this->data_ == nullptr) {
            return Iterator(nullptr);
        }
        return Iterator(this->data_ + (this->len_ - 1));
    }

    ReverseIterator rend(size_t size) const noexcept {
        if (this->data_ == nullptr) {
            return Iterator(nullptr);
        }
        return Iterator(this->data_ - 1);
    }

  private:
    const T* data_ = nullptr;
    size_t len_ = 0;
};

template <typename T> class MutSlice final {
  public:
    MutSlice() = default;

    MutSlice(const T* inData, size_t inLen) : data_(inData), len_(inLen) {}

    [[nodiscard]] const T& operator[](size_t index) const {
        detail::sliceDebugAssertIndexInRange(index, this->len_);
        return data_[index];
    }

    [[nodiscard]] size_t len() const { return this->len_; }

    [[nodiscard]] const T* data() const {return this->data_};

    class Iterator final {
        friend class MutSlice;
        T* current;
        Iterator(T* curr) : current(curr) {}

      public:
        bool operator!=(const Iterator& other) noexcept { return this->current != other.current; }
        T& operator*() const noexcept { return *(this->current); }
        Iterator& operator++() noexcept {
            this->current = this->current + 1;
            return *this;
        }
        Iterator& operator++(int) noexcept {
            ++(*this);
            return *this;
        };
    };

    Iterator begin(size_t size) noexcept { return Iterator(this->data_); }

    Iterator end(size_t size) noexcept { return Iterator(this->data_ + this->len_); }

    class ConstIterator final {
        friend class MutSlice;
        const T* current;
        ConstIterator(const T* curr) : current(curr) {}

      public:
        bool operator!=(const ConstIterator& other) noexcept { return this->current != other.current; }
        const T& operator*() const noexcept { return *(this->current); }
        ConstIterator& operator++() noexcept {
            this->current = this->current + 1;
            return *this;
        }
        ConstIterator& operator++(int) noexcept {
            ++(*this);
            return *this;
        };
    };

    ConstIterator begin(size_t size) const noexcept { return ConstIterator(this->data_); }

    ConstIterator end(size_t size) const noexcept { return ConstIterator(this->data_ + this->len_); }

    class ReverseIterator final {
        friend class MutSlice;
        T* current;
        ReverseIterator(T* curr) : current(curr) {}

      public:
        bool operator!=(const ReverseIterator& other) noexcept { return this->current != other.current; }
        T& operator*() const noexcept { return *(this->current); }
        ReverseIterator& operator++() noexcept {
            this->current = this->current - 1;
            return *this;
        }
        ReverseIterator& operator++(int) noexcept {
            ++(*this);
            return *this;
        };
    };

    ReverseIterator rbegin(size_t size) noexcept {
        if (this->data_ == nullptr) {
            return Iterator(nullptr);
        }
        return Iterator(this->data_ + (this->len_ - 1));
    }

    ReverseIterator rend(size_t size) noexcept {
        if (this->data_ == nullptr) {
            return Iterator(nullptr);
        }
        return Iterator(this->data_ - 1);
    }

    class ReverseConstIterator final {
        friend class MutSlice;
        const T* current;
        ReverseConstIterator(const T* curr) : current(curr) {}

      public:
        bool operator!=(const ReverseConstIterator& other) noexcept { return this->current != other.current; }
        const T& operator*() const noexcept { return *(this->current); }
        ReverseConstIterator& operator++() noexcept {
            this->current = this->current - 1;
            return *this;
        }
        ReverseConstIterator& operator++(int) noexcept {
            ++(*this);
            return *this;
        };
    };

    ReverseConstIterator rbegin(size_t size) const noexcept {
        if (this->data_ == nullptr) {
            return Iterator(nullptr);
        }
        return Iterator(this->data_ + (this->len_ - 1));
    }

    ReverseConstIterator rend(size_t size) const noexcept {
        if (this->data_ == nullptr) {
            return Iterator(nullptr);
        }
        return Iterator(this->data_ - 1);
    }

  private:
    const T* data_ = nullptr;
    size_t len_ = 0;
};

#endif // SY_TYPES_ARRAY_SLICE_HPP_