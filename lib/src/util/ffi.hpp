#pragma once
#ifndef SY_UTIL_FFI_HPP_
#define SY_UTIL_FFI_HPP_

#include "../core.h"

namespace sy {
    class Function;
    struct Type;

    namespace ffi {
        class ArgBuf {
        public:
            ArgBuf() = default;

            ~ArgBuf();

            struct Arg {
                const void* mem;
                const Type* type;
            };

            void push(const Arg& arg);

            Arg at(size_t index) const; 

        private:

            static constexpr size_t INVALID_OFFSET = -1;

            /// May return INVALID_OFFSET, in which case reallocation is required.
            size_t nextOffset(const size_t sizeType, const size_t alignType) const;

            void reallocate(const size_t sizeNewType, const size_t alignNewType);

        private:
            uint8_t* values = nullptr;
            const Type** types = nullptr;
            size_t* offsets;
            size_t count = 0;
            size_t valuesCapacity = 0;
            size_t typesAndOffsetsCapacity = 0;
        };
        void executeCall(const void* fptr);
    }
}

#endif //  SY_UTIL_FFI_HPP_