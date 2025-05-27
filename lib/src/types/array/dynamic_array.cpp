#include "dynamic_array.h"
#include "dynamic_array.hpp"
#include "../../mem/allocator.hpp"
#include "../../util/assert.hpp"

#if SYNC_LIB_TEST

#include "../../doctest.h"

using sy::DynArray;

TEST_CASE("default construction") {
    DynArray<size_t> arr;
    CHECK_EQ(arr.len(), 0);
}

TEST_SUITE("push const T&") {
    TEST_CASE("push 1") {
        DynArray<size_t> arr;
        const size_t element = 2;
        arr.push(element); 

        CHECK_EQ(arr.len(), 1);
        CHECK_EQ(arr[0], element);
    }


    TEST_CASE("push 1") {
        DynArray<size_t> arr;
        const size_t element1 = 5;
        const size_t element2 = 10;
        arr.push(element1); 
        arr.push(element2); 

        CHECK_EQ(arr.len(), 2);
        CHECK_EQ(arr[0], element1);
        CHECK_EQ(arr[1], element2);
    }
}

#endif
