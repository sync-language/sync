#include "result.hpp"
#include "../../util/assert.hpp"

void sy::detail::debugAssertResultIsOk(bool isOk, const char* errMsg) { sy_assert(isOk, errMsg); }

void sy::detail::debugAssertResultIsErr(bool isErr, const char* errMsg) { sy_assert(isErr, errMsg); }

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"

using sy::Result;
using sy::Error;

TEST_CASE("Result<void, E> default is ok") {
    Result<void, int> res;
    CHECK(res.hasValue());
    CHECK_FALSE(res.hasErr());
    CHECK(res);
    CHECK_FALSE(!res);
}

TEST_CASE("Result same ok and error types constructs ok") {
    Result<int, int> res = 5;
    CHECK(res.hasValue());
    CHECK_FALSE(res.hasErr());
    CHECK(res);
    CHECK_FALSE(!res);
    CHECK_EQ(res.value(), 5);
}

TEST_CASE("Result<void, E> from error") {
    Result<void, int> res = Error<int>(10);
    CHECK_FALSE(res.hasValue());
    CHECK(res.hasErr());
    CHECK_FALSE(res);
    CHECK(!res);
    CHECK_EQ(res.err(), 10);
}

TEST_CASE("Result same ok and error types constructs err") {
    Result<int, int> res = Error<int>(10);
    CHECK_FALSE(res.hasValue());
    CHECK(res.hasErr());
    CHECK_FALSE(res);
    CHECK(!res);
    CHECK_EQ(res.err(), 10);
}

struct ComplexType {
    int* ptr = nullptr;
    inline static int aliveCount = 0;

    ComplexType(int in) {
        ptr = new int(in);
        aliveCount += 1;
    }

    ~ComplexType() {
        if(ptr == nullptr) return;
        delete ptr;
        ptr = nullptr;
        aliveCount -= 1;
    }

    ComplexType(ComplexType&& other) {
        ptr = other.ptr;
        other.ptr = nullptr;
    }
};

TEST_CASE("Result ok does not leak") {
    {
        CHECK_EQ(ComplexType::aliveCount, 0);
        Result<ComplexType, ComplexType> res = ComplexType(2);
        CHECK_EQ(ComplexType::aliveCount, 1); // did construct
        CHECK_EQ(*res.value().ptr, 2);
    } // destructor called
    CHECK_EQ(ComplexType::aliveCount, 0);
}

TEST_CASE("Result error does not leak") {
    {
        CHECK_EQ(ComplexType::aliveCount, 0);
        Error<ComplexType> err = ComplexType(2); 
        CHECK_EQ(ComplexType::aliveCount, 1); // did construct
        Result<ComplexType, ComplexType> res = std::move(err);
        CHECK_EQ(ComplexType::aliveCount, 1); // moved successfully
        CHECK_EQ(*res.err().ptr, 2);
    } // destructor called
    CHECK_EQ(ComplexType::aliveCount, 0);
}

TEST_CASE("Result<void, E> error does not leak") {
    {
        CHECK_EQ(ComplexType::aliveCount, 0);
        Error<ComplexType> err = ComplexType(2); 
        CHECK_EQ(ComplexType::aliveCount, 1); // did construct
        Result<void, ComplexType> res = std::move(err);
        CHECK_EQ(ComplexType::aliveCount, 1); // moved successfully
        CHECK_EQ(*res.err().ptr, 2);
    } // destructor called
    CHECK_EQ(ComplexType::aliveCount, 0);
}

#endif
