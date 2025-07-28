//! API
#pragma once
#ifndef SY_TYPES_STRING_STRING_HPP_
#define SY_TYPES_STRING_STRING_HPP_

#include "../../core.h"
#include "string_slice.hpp"
#include "../../mem/allocator.hpp"
#include <iostream>

namespace sy {

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

        [[nodiscard]] static AllocExpect<StringUnmanaged> copyConstruct(
            const StringUnmanaged& other, Allocator& alloc) noexcept;
        
        StringUnmanaged& operator=(const StringUnmanaged& other) = delete;

        [[nodiscard]] AllocExpect<void> copyAssign(const StringUnmanaged& other, Allocator& alloc) noexcept;

        [[nodiscard]] static AllocExpect<StringUnmanaged> copyConstructSlice(
            const StringSlice& slice, Allocator& alloc) noexcept;

        [[nodiscard]] AllocExpect<void> copyAssignSlice(const StringSlice& slice, Allocator& alloc) noexcept;

        [[nodiscard]] static AllocExpect<StringUnmanaged> copyConstructCStr(
            const char* str, Allocator& alloc) noexcept;

        [[nodiscard]] AllocExpect<void> copyAssignCStr(const char* str, Allocator& alloc) noexcept;

        /// Length in bytes, not utf8 characters or graphemes.
        [[nodiscard]] size_t len() const { return len_; }

        /// Any mutation operations on `this` may invalidate the returned slice.
        [[nodiscard]] StringSlice asSlice() const;

        // Get as const char*
        [[nodiscard]] const char* cstr() const;

    SY_CLASS_TEST_PRIVATE:

        bool isSso() const;

        void setHeapFlag();

        void setSsoFlag();

        bool hasEnoughCapacity(const size_t requiredCapacity) const;

    private:
        
        size_t len_     = 0;
        size_t raw_[3]  = { 0 };
    };

    /// Dynamic, [Small String Optimized](https://giodicanio.com/2023/04/26/cpp-small-string-optimization/) 
    /// utf8 string class. It is a wrapper around `StringUnmanaged` that uses the default allocator.
    /// Is script compatible.
    class SY_API String final {
    public:
    
        String() = default;
        
        ~String() noexcept;

        String(const String &other);

        String& operator=(const String& other);
        
        String(String&& other) noexcept;

        String& operator=(String&& other) noexcept;

        String(const StringSlice& str);

        String& operator=(const StringSlice& str);

        String(const char* str);

        String& operator=(const char* str);

        /// Length in bytes, not utf8 characters or graphemes.
        size_t len() const { return inner.len(); }

        /// Any mutation operations on `this` may invalidate the returned slice.
        StringSlice asSlice() const { return inner.asSlice(); }

        // Get as const char*
        const char* cstr() const { return inner.cstr(); }

    private:

        StringUnmanaged inner;
    };
}

#endif // SY_TYPES_STRING_STRING_HPP_

