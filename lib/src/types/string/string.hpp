//! API
#pragma once
#ifndef SY_TYPES_STRING_STRING_HPP_
#define SY_TYPES_STRING_STRING_HPP_

#include "../../core/core.h"
#include "../../mem/allocator.hpp"
#include "string_slice.hpp"
#include <iosfwd>
#include <string_view>

// namespace sy {

// class StringUnmanaged;

// namespace detail {
// /// @param inCapacity will be rounded up to the nearest multiple of `STRING_ALLOC_ALIGN`.
// /// @return Non-zeroed memory
// Result<char*, AllocErr> mallocStringBuffer(size_t& inCapacity, sy::Allocator alloc);

// void freeStringBuffer(char* buff, size_t inCapacity, sy::Allocator alloc);

// class StringUtils {
//   public:
//     static StringUnmanaged makeRaw(char*& buf, size_t length, size_t capacity, sy::Allocator
//     alloc);
// };
// } // namespace detail

// /// Dynamic, [Small String
// /// Optimized](https://giodicanio.com/2023/04/26/cpp-small-string-optimization/) utf8 string
// class.
// /// It supports using a custom allocator.
// class SY_API StringUnmanaged final {
//   public:
//     StringUnmanaged() = default;

//     ~StringUnmanaged() noexcept;

//     void destroy(Allocator& alloc) noexcept;

//     StringUnmanaged(StringUnmanaged&& other) noexcept;

//     StringUnmanaged& operator=(StringUnmanaged&& other) = delete;

//     void moveAssign(StringUnmanaged&& other, Allocator& alloc) noexcept;

//     StringUnmanaged(const StringUnmanaged&) = delete;

//     [[nodiscard]] static Result<StringUnmanaged, AllocErr>
//     copyConstruct(const StringUnmanaged& other, Allocator& alloc) noexcept;

//     StringUnmanaged& operator=(const StringUnmanaged& other) = delete;

//     [[nodiscard]] Result<void, AllocErr> copyAssign(const StringUnmanaged& other,
//                                                     Allocator& alloc) noexcept;

//     [[nodiscard]] static Result<StringUnmanaged, AllocErr>
//     copyConstructSlice(const StringSlice& slice, Allocator& alloc) noexcept;

//     [[nodiscard]] Result<void, AllocErr> copyAssignSlice(const StringSlice& slice,
//                                                          Allocator& alloc) noexcept;

//     [[nodiscard]] static Result<StringUnmanaged, AllocErr>
//     copyConstructCStr(const char* str, Allocator& alloc) noexcept;

//     [[nodiscard]] Result<void, AllocErr> copyAssignCStr(const char* str, Allocator& alloc)
//     noexcept;

//     [[nodiscard]] static Result<StringUnmanaged, AllocErr> fillConstruct(Allocator& alloc,
//                                                                          size_t size, char
//                                                                          toFill);

//     /// Length in bytes, not utf8 characters or graphemes.
//     [[nodiscard]] size_t len() const { return len_; }

//     /// Any mutation operations on `this` may invalidate the returned slice.
//     [[nodiscard]] StringSlice asSlice() const noexcept;

//     // Get as const char*
//     [[nodiscard]] const char* cstr() const noexcept;

//     [[nodiscard]] char* data() noexcept;

//     [[nodiscard]] size_t hash() const noexcept;

//     [[nodiscard]] Result<void, AllocErr> append(StringSlice slice, Allocator alloc) noexcept;

//   private:
//     friend class detail::StringUtils;

//     bool isSso() const;

//     void setHeapFlag();

//     void setSsoFlag();

//     bool hasEnoughCapacity(const size_t requiredCapacity) const;

//   private:
//     size_t len_ = 0;
//     size_t raw_[3] = {0};
// };

// /// Dynamic, [Small String
// /// Optimized](https://giodicanio.com/2023/04/26/cpp-small-string-optimization/) utf8 string
// class.
// /// It is a wrapper around `StringUnmanaged` that uses the default allocator. Is script
// compatible. class SY_API String final {
//   public:
//     String() = default;

//     String(Allocator inAlloc) noexcept : alloc_(inAlloc) {}

//     ~String() noexcept;

//     String(const String& other);

//     [[nodiscard]] static Result<String, AllocErr> copyConstruct(const String& other);

//     String& operator=(const String& other);

//     [[nodiscard]] Result<void, AllocErr> copyAssign(const String& other);

//     String(String&& other) noexcept;

//     String& operator=(String&& other) noexcept;

//     String(const StringSlice& str);

//     [[nodiscard]] static Result<String, AllocErr> copyConstructSlice(const StringSlice& str,
//                                                                      Allocator alloc);

//     String& operator=(const StringSlice& str);

//     String(const char* str);

//     String& operator=(const char* str);

//     /// Length in bytes, not utf8 characters or graphemes.
//     [[nodiscard]] size_t len() const { return inner_.len(); }

//     /// Any mutation operations on `this` may invalidate the returned slice.
//     [[nodiscard]] StringSlice asSlice() const { return inner_.asSlice(); }

//     // Get as const char*
//     [[nodiscard]] const char* cstr() const { return inner_.cstr(); }

//     [[nodiscard]] char* data() { return inner_.data(); };

//     [[nodiscard]] size_t hash() const { return inner_.hash(); }

//     Result<void, AllocErr> append(StringSlice slice) noexcept;

//   private:
//     String(StringUnmanaged&& inner, Allocator alloc) : inner_(std::move(inner)), alloc_(alloc) {}

//   private:
//     StringUnmanaged inner_{};
//     Allocator alloc_{};
// };
// } // namespace sy

// namespace std {
// template <> struct hash<sy::StringUnmanaged> {
//     size_t operator()(const sy::StringUnmanaged& obj) { return obj.hash(); }
// };
// template <> struct hash<sy::String> {
//     size_t operator()(const sy::String& obj) { return obj.hash(); }
// };
// } // namespace std

namespace sy {
namespace internal {
struct AtomicStringHeader;
struct Test_String;
struct Test_StringBuilder;
} // namespace internal

class StringBuilder;

class SY_API String {
  public:
    String() = default;

    ~String() noexcept;

    String(const String& other) noexcept;

    String& operator=(const String& other) noexcept;

    String(String&& other) noexcept;

    String& operator=(String&& other) noexcept;

    [[nodiscard]] static Result<String, AllocErr> init(StringSlice str,
                                                       Allocator alloc = {}) noexcept;

    /// Prefer calling `String::init()` for error handling correctness, as memory allocation CAN
    /// fail.
    /// @param str
    /// @param alloc
    [[nodiscard]] String(StringSlice str, Allocator alloc = {}) noexcept;

    [[nodiscard]] size_t len() const noexcept;

    [[nodiscard]] StringSlice asSlice() const noexcept;

    [[nodiscard]] const char* cstr() const noexcept;

    [[nodiscard]] size_t hash() const noexcept;

    [[nodiscard]] operator StringSlice() const noexcept { return asSlice(); }

    [[nodiscard]] operator std::string_view() const noexcept;

    [[nodiscard]] bool operator==(const String& other) const noexcept;

    [[nodiscard]] bool operator==(StringSlice other) const noexcept;

    friend bool operator==(StringSlice lhs, const String& rhs) noexcept;

    [[nodiscard]] Result<String, AllocErr> concat(StringSlice str) const noexcept;

    friend std::ostream& operator<<(std::ostream& os, const String& s);

  private:
    friend struct internal::AtomicStringHeader;
    friend class StringBuilder;
    friend struct internal::Test_String;

    const internal::AtomicStringHeader* impl_ = nullptr;
};
} // namespace sy

namespace std {
template <> struct hash<sy::String> {
    size_t operator()(const sy::String& obj) { return obj.hash(); }
};
} // namespace std

namespace sy {
class SY_API StringBuilder {
  public:
    StringBuilder() = default;

    ~StringBuilder() noexcept;

    StringBuilder(const StringBuilder& other) noexcept = delete;

    StringBuilder& operator=(const StringBuilder& other) noexcept = delete;

    StringBuilder(StringBuilder&& other) noexcept;

    StringBuilder& operator=(StringBuilder&& other) noexcept;

    [[nodiscard]] static Result<StringBuilder, AllocErr> init(Allocator alloc = {}) noexcept;

    [[nodiscard]] static Result<StringBuilder, AllocErr>
    initWithCapacity(size_t inCapacity, Allocator alloc = {}) noexcept;

    /// Invalidates `this`.
    /// @return
    [[nodiscard]] Result<String, AllocErr> build() noexcept;

    [[nodiscard]] Result<void, AllocErr> write(StringSlice str) noexcept;

  private:
    friend struct internal::Test_StringBuilder;

    internal::AtomicStringHeader* impl_ = nullptr;
    /// The total amount of bytes allocated, minus
    /// `internal::AtomicStringHeader::HEADER_NON_STRING_BYTES_USED`.
    /// Meaning the total allocated size is:
    /// `fullAllocatedCapacity_ + internal::AtomicStringHeader::HEADER_NON_STRING_BYTES_USED`.
    size_t fullAllocatedCapacity_ = 0;
};
} // namespace sy

#endif // SY_TYPES_STRING_STRING_HPP_
