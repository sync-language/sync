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

        // members
    };
}

#endif // SY_TYPES_STRING_HPP_