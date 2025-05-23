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
    
        // Default constructor 
        String()=default;
        // Destructor

        // Copy constructor

        // Copy assignment

        // Move constructor

        // Move assignment

        // String Slice constructor

        // const char* constructor

        // Get length

        // Get as string slice

        // Get as const char*


    private:

        // allocate buffer

        // free buffer

        // check if using sso

        // set sso flag

    private:
    //2 members, one is called _length size_t, _data an array of three size_t and set them default to 0
        // members
        size_t _length=0;
        size_t _data[3] = {0};
    };
}

#endif // SY_TYPES_STRING_HPP_