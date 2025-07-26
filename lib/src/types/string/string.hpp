//! API
#pragma once
#ifndef SY_TYPES_STRING_STRING_HPP_
#define SY_TYPES_STRING_STRING_HPP_

#include "../../core.h"
#include "string_slice.hpp"
#include "../../mem/allocator.hpp"

namespace sy {

    /// Dynamic, [Small String Optimized](https://giodicanio.com/2023/04/26/cpp-small-string-optimization/) 
    /// utf8 string class. It supports using a custom allocator.
    class SY_API StringUnmanaged SY_CLASS_FINAL {
    public:
    
        StringUnmanaged() = default;

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
        size_t len() const { return _length; }

        /// Any mutation operations on `this` may invalidate the returned slice.
        StringSlice asSlice() const;

        // Get as const char*
        const char* cstr() const;

    private:

        bool isSso()const;

        void setHeapFlag();

        void setSsoFlag();

        static constexpr size_t SSO_CAPACITY = 3 * sizeof(size_t);
        // Not including null terminator
        static constexpr size_t MAX_SSO_LEN = SSO_CAPACITY - 1;

        struct SsoBuffer {
            char arr[SSO_CAPACITY] = { '\0' };
            SsoBuffer() = default;
        };

        struct HeapBuffer {
            char*   ptr = nullptr;
            size_t  capacity = 0;
            char    _unused[sizeof(size_t) - 1] = { 0 };
            char    flag = 0;
            HeapBuffer() = default;
        };

        const SsoBuffer* asSso() const;
        SsoBuffer* asSso();
        const HeapBuffer* asHeap() const;
        HeapBuffer* asHeap();

        bool hasEnoughCapacity(const size_t requiredCapacity) const;

        static_assert(sizeof(SsoBuffer) == sizeof(HeapBuffer));
        static_assert(sizeof(SsoBuffer) == sizeof(size_t[3]));

    private:

        size_t      _length = 0;
        size_t      _raw[3] = { 0 };
    };
}

#endif // SY_TYPES_STRING_STRING_HPP_

