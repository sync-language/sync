#include "ffi.hpp"
#include <cstring>
#include "../types/type_info.hpp"
#include "../mem/allocator.hpp"
#include "assert.hpp"
#if _MSC_VER
#include <new>
#endif

namespace sy {
    namespace ffi {
        #if _MSC_VER
        constexpr size_t VALUES_ALIGNMENT = std::hardware_destructive_interference_size * 2;
        #else
        constexpr size_t VALUES_ALIGNMENT = 64 * 2;
        #endif

        static thread_local ArgBuf ffiArgBuf;

        ArgBuf::~ArgBuf()
        {
            Allocator alloc;
            if(this->valuesCapacity > 0) {
                alloc.freeAlignedArray(this->values, this->valuesCapacity, VALUES_ALIGNMENT);
            }
            if(this->typesAndOffsetsCapacity > 0) {
                alloc.freeArray(this->types, this->typesAndOffsetsCapacity);
                alloc.freeArray(this->offsets, this->typesAndOffsetsCapacity);
            }
            this->count = 0;
            this->valuesCapacity = 0;
            this->typesAndOffsetsCapacity = 0;
        }

        void ArgBuf::push(const Arg &arg)
        {
            sy_assert(arg.type->sizeType > 0, "Cannot push zero sized arguments");
            sy_assert(arg.type->alignType > 0, "Cannot push zero aligned arguments");

            this->reallocate(arg.type->sizeType, arg.type->alignType);
            size_t offset = this->nextOffset(arg.type->sizeType, arg.type->alignType);

            // Lambda because this is unlikely to execute. In real world scenarios, may never execute at any point.
            auto loopingReallocate = [&]() {
                size_t i = 1;
                while(offset == INVALID_OFFSET) {
                    this->reallocate(arg.type->sizeType << i, arg.type->alignType);
                    offset = this->nextOffset(arg.type->sizeType, arg.type->alignType);
                }
            };

            if(offset == INVALID_OFFSET) {
                loopingReallocate();
            }

            memcpy(&this->values[offset], arg.mem, arg.type->sizeType);
            this->types[this->count] = arg.type;
            this->offsets[this->count] = offset;
            this->count += 1;
        }

        ArgBuf::Arg ArgBuf::at(size_t index) const
        {
            sy_assert(index < this->count, "Index out of bounds");
            Arg a{};
            a.mem = reinterpret_cast<const void*>(&this->values[this->offsets[index]]);
            a.type = this->types[index];
            return a;
        }

        size_t ArgBuf::nextOffset(const size_t sizeType, const size_t alignType) const
        {
            sy_assert(this->values != nullptr, "Cannot get the next offset from an invalid memory address");

            const size_t valuesAddr = reinterpret_cast<size_t>(this->values);
            const size_t requiredShift = alignType - (valuesAddr % alignType);

            if(this->count == 0) {
                if((requiredShift + sizeType) > this->valuesCapacity) {
                    return INVALID_OFFSET;
                }

                return requiredShift;

            } else {
                if((this->offsets[this->count] + requiredShift + sizeType) > this->valuesCapacity) {
                    return INVALID_OFFSET;
                }

                return this->offsets[this->count] + requiredShift;
            }
        }

        void ArgBuf::reallocate(const size_t sizeNewType, const size_t alignNewType)
        {
            size_t newValuesCapacity = 0;
            size_t newTypesAndOffsetCapacity = 0;

            if(this->count == 0) {
                // Overallocate. Guaranteed to get a valid offset.
                newValuesCapacity = sizeNewType * 2;
                if(newValuesCapacity < VALUES_ALIGNMENT) {
                    newValuesCapacity = VALUES_ALIGNMENT;
                }
                newTypesAndOffsetCapacity = 2;
            } else {
                newValuesCapacity = (this->valuesCapacity * 2) + (sizeNewType * 2);
                newTypesAndOffsetCapacity = this->typesAndOffsetsCapacity * 2;
            }

            Allocator alloc;
            uint8_t* newValues = alloc.allocAlignedArray<uint8_t>(newValuesCapacity, VALUES_ALIGNMENT).get();
            const Type** newTypes = alloc.allocArray<const Type*>(newTypesAndOffsetCapacity).get();
            size_t* newOffsets = alloc.allocArray<size_t>(newTypesAndOffsetCapacity).get();

            if(this->count > 0) {
                memcpy(newValues, this->values, this->valuesCapacity);
                for(size_t i = 0; i < this->count; i++) {
                    newTypes[i] = this->types[i];
                    newOffsets[i] = this->offsets[i];
                }
                alloc.freeAlignedArray(this->values, this->valuesCapacity, VALUES_ALIGNMENT);
                alloc.freeArray(this->types, this->typesAndOffsetsCapacity);
                alloc.freeArray(this->offsets, this->typesAndOffsetsCapacity);
            }

            this->values = newValues;
            this->types = newTypes;
            this->offsets = newOffsets;
            this->valuesCapacity = newValuesCapacity;
            this->typesAndOffsetsCapacity = newTypesAndOffsetCapacity;
        }
    }
}

void sy::ffi::executeCall(const void *fptr)
{
    #if defined(__aarch64__) || defined(_M_ARM64)
    // https://learn.microsoft.com/en-us/cpp/build/arm64-windows-abi-conventions?view=msvc-170



    #endif
}


#if SYNC_LIB_TEST

#include "../doctest.h"
#include <iostream>

void something1() {
    std::cout << "Something 1\n" << std::endl;
}

void something2() {
    std::cout << "Something 2\n" << std::endl;
}

TEST_CASE("inline asm") {
    const uintptr_t p1 = reinterpret_cast<uintptr_t>(&something1);
    const uintptr_t p2 = reinterpret_cast<uintptr_t>(&something2);

    uintptr_t p = 0;
    srand((unsigned int)time(NULL));

    for(int i = 0; i < 10; i++) {
        const int randVal = rand();
        std::cout << randVal;

        if(randVal > (RAND_MAX / 2)) { p = p1; }
        else { p = p2; }

        __asm (
            "blr %[p]"
            : [p]"=r"(p)
        );
    }
    
}

#endif