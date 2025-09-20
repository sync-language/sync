//! API
#pragma once
#ifndef SY_TYPES_STRING_STRING_HPP_
#define SY_TYPES_STRING_STRING_HPP_

#include "../../core.h"
#include "../../mem/allocator.hpp"
#include "string_slice.hpp"
#include <iostream>

namespace sy {

class StringUnmanaged;

namespace detail {
/// @param inCapacity will be rounded up to the nearest multiple of `STRING_ALLOC_ALIGN`.
/// @return Non-zeroed memory
Result<char*, AllocErr> mallocStringBuffer(size_t& inCapacity, sy::Allocator alloc);

void freeStringBuffer(char* buff, size_t inCapacity, sy::Allocator alloc);

class StringUtils {
  public:
    static StringUnmanaged makeRaw(char*& buf, size_t length, size_t capacity, sy::Allocator alloc);
};
} // namespace detail

/// Dynamic, [Small String Optimized](https://giodicanio.com/2023/04/26/cpp-small-string-optimization/)
/// utf8 string class. It supports using a custom allocator.
class SY_API StringUnmanaged SY_CLASS_FINAL {
  public:
    StringUnmanaged() = default;

    ~StringUnmanaged() noexcept;

    void destroy(Allocator& alloc) noexcept;

    StringUnmanaged(StringUnmanaged&& other) noexcept;

    StringUnmanaged& operator=(StringUnmanaged&& other) = delete;

    void moveAssign(StringUnmanaged&& other, Allocator& alloc) noexcept;

    StringUnmanaged(const StringUnmanaged&) = delete;

    [[nodiscard]] static Result<StringUnmanaged, AllocErr> copyConstruct(const StringUnmanaged& other,
                                                                         Allocator& alloc) noexcept;

    StringUnmanaged& operator=(const StringUnmanaged& other) = delete;

    [[nodiscard]] Result<void, AllocErr> copyAssign(const StringUnmanaged& other, Allocator& alloc) noexcept;

    [[nodiscard]] static Result<StringUnmanaged, AllocErr> copyConstructSlice(const StringSlice& slice,
                                                                              Allocator& alloc) noexcept;

    [[nodiscard]] Result<void, AllocErr> copyAssignSlice(const StringSlice& slice, Allocator& alloc) noexcept;

    [[nodiscard]] static Result<StringUnmanaged, AllocErr> copyConstructCStr(const char* str,
                                                                             Allocator& alloc) noexcept;

    [[nodiscard]] Result<void, AllocErr> copyAssignCStr(const char* str, Allocator& alloc) noexcept;

    [[nodiscard]] static Result<StringUnmanaged, AllocErr> fillConstruct(Allocator& alloc, size_t size, char toFill);

    /// Length in bytes, not utf8 characters or graphemes.
    [[nodiscard]] size_t len() const { return len_; }

    /// Any mutation operations on `this` may invalidate the returned slice.
    [[nodiscard]] StringSlice asSlice() const;

    // Get as const char*
    [[nodiscard]] const char* cstr() const;

    [[nodiscard]] char* data();

    [[nodiscard]] size_t hash() const;

    SY_CLASS_TEST_PRIVATE :

        friend class detail::StringUtils;

    bool isSso() const;

    void setHeapFlag();

    void setSsoFlag();

    bool hasEnoughCapacity(const size_t requiredCapacity) const;

  private:
    size_t len_ = 0;
    size_t raw_[3] = {0};
};

/// Dynamic, [Small String Optimized](https://giodicanio.com/2023/04/26/cpp-small-string-optimization/)
/// utf8 string class. It is a wrapper around `StringUnmanaged` that uses the default allocator.
/// Is script compatible.
class SY_API String final {
  public:
    String() = default;

    ~String() noexcept;

    String(const String& other);

    [[nodiscard]] static Result<String, AllocErr> copyConstruct(const String& other);

    String& operator=(const String& other);

    [[nodiscard]] Result<void, AllocErr> copyAssign(const String& other);

    String(String&& other) noexcept;

    String& operator=(String&& other) noexcept;

    String(const StringSlice& str);

    String& operator=(const StringSlice& str);

    String(const char* str);

    String& operator=(const char* str);

    /// Length in bytes, not utf8 characters or graphemes.
    [[nodiscard]] size_t len() const { return inner_.len(); }

    /// Any mutation operations on `this` may invalidate the returned slice.
    [[nodiscard]] StringSlice asSlice() const { return inner_.asSlice(); }

    // Get as const char*
    [[nodiscard]] const char* cstr() const { return inner_.cstr(); }

    [[nodiscard]] char* data() { return inner_.data(); };

    [[nodiscard]] size_t hash() const { return inner_.hash(); }

  private:
    String(StringUnmanaged&& inner, Allocator alloc) : inner_(std::move(inner)), alloc_(alloc) {}

  private:
    StringUnmanaged inner_{};
    Allocator alloc_{};
};
} // namespace sy

namespace std {
template <> struct hash<sy::StringUnmanaged> {
    size_t operator()(const sy::StringUnmanaged& obj) { return obj.hash(); }
};
template <> struct hash<sy::String> {
    size_t operator()(const sy::String& obj) { return obj.hash(); }
};
} // namespace std

#endif // SY_TYPES_STRING_STRING_HPP_
