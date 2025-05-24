//! API
#pragma once
#ifndef SY_TYPES_STRING_HPP_
#define SY_TYPES_STRING_HPP_

#include "../../core.h"
#include "string_slice.hpp"

namespace sy {

    /// Dynamic, [Small String Optimized](https://giodicanio.com/2023/04/26/cpp-small-string-optimization/) 
    /// string class. It's a primitive script type.
    class SY_API String final {
    public:
    
        String() = default;
        
        ~String() noexcept;

        String(const String &other);

        // Copy assignment
        // String& operator=(const String& other);
        
        String(String&& other) noexcept;

        // Move assignment

        // String Slice constructor

        // const char* constructor

        size_t len() const { return _length; }

        // Get as string slice

        // Get as const char*


    private:

        bool isSso()const;
        void setHeapFlag();

    private:

        static constexpr size_t SSO_CAPACITY = 3 * sizeof(void*);

        struct SsoBuffer {
            char arr[SSO_CAPACITY]={0};
            SsoBuffer() = default;
        };

        struct HeapBuffer {
            char*   ptr = nullptr;
            size_t  capacity = 0;
            char    _unused[sizeof(void*)-1] = {0};
            char    flag = 0;
            HeapBuffer() = default;
        };

        union StringImpl {
            /* data */
            SsoBuffer   sso;
            HeapBuffer  heap;
            size_t      raw[3];
            StringImpl() :sso() {};
        };

        static_assert(sizeof(StringImpl) == sizeof(SsoBuffer));
        static_assert(sizeof(StringImpl) == sizeof(HeapBuffer));
        static_assert(sizeof(StringImpl) == sizeof(size_t[3]));

    //2 members, one is called _length size_t, _data an array of three size_t and set them default to 0
        // members
        size_t      _length=0;
        StringImpl  _impl;
    };
}

#endif // SY_TYPES_STRING_HPP_

