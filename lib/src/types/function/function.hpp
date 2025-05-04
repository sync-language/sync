//! API
#pragma once
#ifndef SY_TYPES_FUNCTION_FUNCTION_HPP_
#define SY_TYPES_FUNCTION_FUNCTION_HPP_

#include "../../core.h"
#include "../string/string_slice.hpp"

namespace sy {
    namespace c {
        #include "function.h"
    }

    struct Type;

    class Function {
    public:

        enum class Type : int32_t {
            C = c::SyFunctionTypeC,
            Script = c::SyFunctionTypeScript,            
        };

        StringSlice     name;
        StringSlice     identifierName;
        const Type*     returnType;
        const Type**    argsTypes;
        uint16_t        argsLen;
        uint16_t        alignment = 16;
        Type            tag;
        const void*     fptr;
    };
}

#endif // SY_TYPES_FUNCTION_FUNCTION_HPP_