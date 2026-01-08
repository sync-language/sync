#include "option.hpp"
#include "../../core/core_internal.h"

static_assert(sizeof(sy::Option<void*>) == sizeof(void*), "Optional pointer is same size as pointer always");
static_assert(sizeof(sy::Option<int*>) == sizeof(int*), "Optional pointer is same size as pointer always");
static_assert(sizeof(sy::Option<sy::Option<int>*>) == sizeof(void*), "Optional pointer is same size as pointer always");

void SY_API sy::detail::debugAssertPtrNotNull(const void* ptr) {
    sy_assert(ptr != nullptr, "Bad Option Access: Optional pointer was null");
}

void SY_API sy::detail::debugAssertOptionHasValue(bool hasVal) {
    sy_assert(hasVal, "Bad Option Access: Optional has no value");
}

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"

using sy::Option;

TEST_CASE("Option ptr empty") {
    Option<void*> opt;
    CHECK_FALSE(opt.hasValue());
    CHECK_FALSE(opt);
    CHECK(!opt);
}

TEST_CASE("Option ptr not empty") {
    void* somePtr = reinterpret_cast<void*>(1);
    Option<void*> opt = somePtr;
    CHECK(opt.hasValue());
    CHECK(opt);
    CHECK_FALSE(!opt);
    CHECK_EQ(opt.value(), somePtr);
}

TEST_CASE("Option ptr empty non void") {
    Option<int*> opt;
    CHECK_FALSE(opt.hasValue());
    CHECK_FALSE(opt);
    CHECK(!opt);
}

TEST_CASE("Option ptr not empty non void") {
    int val = 5;
    Option<int*> opt = &val;
    CHECK(opt.hasValue());
    CHECK(opt);
    CHECK_FALSE(!opt);
    CHECK_EQ(*opt.value(), 5);
}

TEST_CASE("Option reference empty") {
    Option<int&> opt;
    CHECK_FALSE(opt.hasValue());
    CHECK_FALSE(opt);
    CHECK(!opt);
}

TEST_CASE("Option reference not empty") {
    int val = 5;
    Option<int&> opt = val;
    CHECK(opt.hasValue());
    CHECK(opt);
    CHECK_FALSE(!opt);
    CHECK_EQ(opt.value(), 5);
    opt.value() = 6;
    CHECK_EQ(opt.value(), 6);
    CHECK_EQ(val, 6);
}

TEST_CASE("Option const reference not empty") {
    int val = 5;
    Option<const int&> opt = val;
    CHECK(opt.hasValue());
    CHECK(opt);
    CHECK_FALSE(!opt);
    CHECK_EQ(opt.value(), 5);
}

TEST_CASE("Option value empty") {
    Option<int> opt;
    CHECK_FALSE(opt.hasValue());
    CHECK_FALSE(opt);
    CHECK(!opt);
}

TEST_CASE("Option value not empty") {
    int val = 5;
    Option<int> opt = val;
    CHECK(opt.hasValue());
    CHECK(opt);
    CHECK_FALSE(!opt);
    CHECK_EQ(opt.value(), 5);
    opt.value() = 6;
    CHECK_EQ(opt.value(), 6);
    CHECK_EQ(val, 5); // does not change initial value cause is copy
}

struct ComplexType {
    int* ptr = nullptr;
    inline static int aliveCount = 0;

    ComplexType(int in) {
        ptr = new int(in);
        aliveCount += 1;
    }

    ~ComplexType() {
        if (ptr == nullptr)
            return;
        delete ptr;
        ptr = nullptr;
        aliveCount -= 1;
    }

    ComplexType(ComplexType&& other) {
        ptr = other.ptr;
        other.ptr = nullptr;
    }
};

TEST_CASE("Option complex type empty") {
    CHECK_EQ(ComplexType::aliveCount, 0);
    Option<ComplexType> opt;
    CHECK_EQ(ComplexType::aliveCount, 0); // didn't construct
    CHECK_FALSE(opt.hasValue());
    CHECK_FALSE(opt);
    CHECK(!opt);
}

TEST_CASE("Option complex type empty") {
    {
        CHECK_EQ(ComplexType::aliveCount, 0);
        Option<ComplexType> opt = ComplexType(5);
        CHECK_EQ(ComplexType::aliveCount, 1); // did construct
        CHECK(opt.hasValue());
        CHECK(opt);
        CHECK_FALSE(!opt);
        CHECK_EQ(*opt.value().ptr, 5);
        *opt.value().ptr = 6;
        CHECK_EQ(*opt.value().ptr, 6);
    } // destructor called
    CHECK_EQ(ComplexType::aliveCount, 0);
}

#endif