#include "anyerror.hpp"
#include "../../core/core_internal.h"
#include "../string/string.hpp"
#include "../type_info.hpp"
#include <cstring>
#include <new>

using namespace sy;

struct AnyError::Impl {
    Allocator alloc;
    // May be empty
    StringUnmanaged message{};
    Option<AnyError> cause{};
    // If has a value, so does `payloadType`
    Option<void*> payload{};
    // If has a value, so does `payload`
    Option<const Type*> payloadType{};
    // TODO stack trace and source location

    ~Impl() noexcept {
        message.destroy(alloc);
        if (payload.hasValue()) {
            payloadType.value()->destroyObject(payload.value());
        }
    }
};

Result<AnyError, AllocErr> sy::AnyError::init(Allocator alloc, StringSlice message, void* payload,
                                              const Type* payloadType) noexcept {
    const bool nullPayload = payload == nullptr;
    const bool nullType = payloadType == nullptr;
    if (nullPayload) {
        sy_assert(nullType, "For no payload, expected no type");
    }
    if (nullType) {
        sy_assert(nullPayload, "Expected payload for supplied type, but none was provided");
    }

    auto implRes = alloc.allocObject<Impl>();
    if (implRes.hasErr()) {
        return Error(AllocErr::OutOfMemory);
    }

    auto strRes = StringUnmanaged::copyConstructSlice(message, alloc);
    if (strRes.hasErr()) {
        alloc.freeObject(implRes.value());
        return Error(AllocErr::OutOfMemory);
    }

    void* payloadMem = nullptr;
    if (!nullPayload) {
        auto payloadRes = alloc.allocAlignedArray<uint8_t>(payloadType->sizeType, payloadType->alignType);
        if (payloadRes.hasErr()) {
            alloc.freeObject(implRes.value());
            strRes.value().destroy(alloc);
            return Error(AllocErr::OutOfMemory);
        }
        payloadMem = payloadRes.value();
        memcpy(payloadMem, payload, payloadType->sizeType);
    }

    Impl* self = implRes.value();
    new (self) Impl();
    self->alloc = alloc;
    // new (&self->message) StringUnmanaged(strRes.takeValue());
    self->message.moveAssign(strRes.takeValue(), alloc);

    if (payloadMem) {
        self->payload = payloadMem;
        self->payloadType = payloadType;
    }

    AnyError e;
    e.impl_ = self;
    return e;
}

AnyError::AnyError(Allocator alloc, StringSlice message, void* payload, const Type* payloadType) noexcept {
    AnyError err = AnyError::init(alloc, message, payload, payloadType).takeValue();
    this->impl_ = err.impl_;
    err.impl_ = nullptr;
}

Result<AnyError, AllocErr> sy::AnyError::initCause(AnyError cause, StringSlice message, void* payload,
                                                   const Type* payloadType) noexcept {
    sy_assert(cause.impl_ != nullptr, "Expected valid cause");

    auto errRes = AnyError::init(cause.impl_->alloc, message, payload, payloadType);
    if (errRes.hasValue()) {
        new (&errRes.value().impl_->cause) Option<AnyError>(std::move(cause));
    }

    return errRes;
}

AnyError::AnyError(AnyError cause, StringSlice message, void* payload, const Type* payloadType) noexcept {
    AnyError err = AnyError::initCause(std::move(cause), message, payload, payloadType).takeValue();
    this->impl_ = err.impl_;
    err.impl_ = nullptr;
}

AnyError::AnyError(AnyError&& other) noexcept : impl_(other.impl_) { other.impl_ = nullptr; }

AnyError& sy::AnyError::operator=(AnyError&& other) noexcept {
    if (this == &other)
        return *this;

    if (this->impl_ != nullptr) {
        Allocator alloc = this->impl_->alloc;
        this->impl_->~Impl();
        alloc.freeObject(this->impl_);
    }

    this->impl_ = other.impl_;
    other.impl_ = nullptr;
    return *this;
}

Result<AnyError, ProgramError> sy::AnyError::clone() const noexcept {
    if (this->impl_ == nullptr) {
        return AnyError();
    }

    const Type* type = nullptr;
    if (this->impl_->payloadType.hasValue()) {
        type = this->impl_->payloadType.value();
        sy_assert(type->copyConstructor.hasValue(),
                  "Cannot clone AnyError payload that doesn't have a copy constructor");
    }

    Allocator alloc = this->impl_->alloc;

    auto implRes = alloc.allocObject<Impl>();
    if (implRes.hasErr()) {
        return Error(ProgramError::OutOfMemory);
    }

    auto strRes = StringUnmanaged::copyConstruct(this->impl_->message, alloc);
    if (strRes.hasErr()) {
        alloc.freeObject(implRes.value());
        return Error(ProgramError::OutOfMemory);
    }

    void* payloadMem = nullptr;
    if (type != nullptr) {
        auto payloadRes = alloc.allocAlignedArray<uint8_t>(type->sizeType, type->alignType);
        if (payloadRes.hasErr()) {
            alloc.freeObject(implRes.value());
            strRes.value().destroy(alloc);
            return Error(ProgramError::OutOfMemory);
        }
        payloadMem = payloadRes.value();
        auto copyErr = type->copyConstructObj(payloadMem, this->impl_->payload.value());
        if (copyErr.hasErr()) {
            alloc.freeObject(implRes.value());
            alloc.freeAlignedArray(payloadRes.value(), type->sizeType, type->alignType);
            strRes.value().destroy(alloc);
            return Error(copyErr.takeErr());
        }
    }

    Impl* newErr = implRes.value();
    new (newErr) Impl();
    newErr->alloc = alloc;
    new (&newErr->message) StringUnmanaged(strRes.takeValue());

    if (payloadMem) {
        newErr->payload = payloadMem;
        newErr->payloadType = type;
    }

    // recursive chicanery here.
    if (this->impl_->cause.hasValue()) {
        auto cloneRes = this->impl_->cause.value().clone();
        if (cloneRes.hasErr()) {
            newErr->~Impl();
            alloc.freeObject(newErr);
            return cloneRes;
        }

        new (&newErr->cause) Option<AnyError>(cloneRes.takeValue());
    }

    AnyError e;
    e.impl_ = newErr;
    return e;
}

AnyError::AnyError(const AnyError& other) noexcept { new (this) AnyError(other.clone().takeValue()); }

AnyError& sy::AnyError::operator=(const AnyError& other) noexcept {
    if (this == &other)
        return *this;

    if (this->impl_ != nullptr) {
        Allocator alloc = this->impl_->alloc;
        this->impl_->~Impl();
        alloc.freeObject(this->impl_);
    }

    new (this) AnyError(other.clone().takeValue());
    return *this;
}

sy::AnyError::~AnyError() noexcept {
    if (this->impl_ == nullptr)
        return;

    Allocator alloc = this->impl_->alloc;
    this->impl_->~Impl();
    alloc.freeObject(this->impl_);
    this->impl_ = nullptr;
}

StringSlice sy::AnyError::message() const noexcept {
    if (this->impl_ == nullptr)
        return StringSlice();

    return this->impl_->message.asSlice();
}

Option<AnyError&> sy::AnyError::cause() noexcept {
    if (this->impl_ == nullptr || !this->impl_->cause.hasValue())
        return {};

    return Option<AnyError&>(this->impl_->cause.value());
}

Option<const AnyError&> sy::AnyError::cause() const noexcept {
    if (this->impl_ == nullptr || !this->impl_->cause.hasValue())
        return {};

    return Option<const AnyError&>(this->impl_->cause.value());
}

Option<void*> sy::AnyError::rawPayload() noexcept {
    if (this->impl_ == nullptr)
        return {};

    return this->impl_->payload;
}

Option<const void*> sy::AnyError::rawPayload() const noexcept {
    if (this->impl_ == nullptr || !this->impl_->payload.hasValue())
        return {};

    return Option<const void*>(this->impl_->payload.value());
}

Option<const Type*> sy::AnyError::payloadType() const noexcept {
    if (this->impl_ == nullptr)
        return {};

    return this->impl_->payloadType;
}

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"

namespace {
struct Example {
    int a;
};

Result<void, ProgramError> fallibleCopyExample(FunctionHandler) { return Error(ProgramError::Unknown); }

const Type* exampleGoodType = Type::makeType<Example>("Example", Type::Tag::Int, {});

const Type* makeExampleFallibleType() {
    static const Type::ExtraInfo::Reference constRefInfo = {false, exampleGoodType};
    static const Type::ExtraInfo::Reference mutRefInfo = {true, exampleGoodType};

    static const Type::ExtraInfo constRefExtra{constRefInfo};
    static const Type::ExtraInfo mutRefExtra{mutRefInfo};

    static Type constRefType = {sizeof(const Example*),
                                static_cast<uint16_t>(alignof(const Example*)),
                                "ConstRef",
                                {},
                                {},
                                {},
                                {},
                                {},
                                Type::Tag::Reference,
                                constRefExtra,
                                nullptr,
                                nullptr};

    static Type mutRefType = {sizeof(Example*),
                              static_cast<uint16_t>(alignof(Example*)),
                              "MutRef",
                              {},
                              {},
                              {},
                              {},
                              {},
                              Type::Tag::Reference,
                              mutRefExtra,
                              nullptr,
                              nullptr};

    static const Type* copyArgTypes[2] = {&mutRefType, &constRefType};
    static const RawFunction fallibleCopyFunction = {"fallibleCopyExample",
                                                     "fallibleCopyExample",
                                                     nullptr,
                                                     copyArgTypes,
                                                     2,
                                                     SY_FUNCTION_MIN_ALIGN,
                                                     true,
                                                     FunctionType::C,
                                                     reinterpret_cast<const void*>(fallibleCopyExample)};

    static Type exampleFallibleType = {sizeof(Example),
                                       static_cast<uint16_t>(alignof(Example)),
                                       "Example",
                                       exampleGoodType->destructor,
                                       &fallibleCopyFunction,
                                       {},
                                       {},
                                       {},
                                       Type::Tag::Int,
                                       {},
                                       &constRefType,
                                       &mutRefType};

    constRefType.constRef = &constRefType;
    constRefType.mutRef = &mutRefType;
    mutRefType.constRef = &constRefType;
    mutRefType.mutRef = &mutRefType;

    return &exampleFallibleType;
}
} // namespace

TEST_CASE("[AnyError] init with empty message and no payload") {
    Allocator alloc;

    auto result = AnyError::init(alloc, StringSlice(), nullptr, nullptr);

    CHECK(result.hasValue());
    AnyError err = result.takeValue();

    CHECK_GE(err.message().len(), 0);
    CHECK_FALSE(err.rawPayload().hasValue());
    CHECK_FALSE(err.payloadType().hasValue());
    CHECK_FALSE(err.cause().hasValue());
}

TEST_CASE("[AnyError] init with message and no payload") {
    Allocator alloc;
    StringSlice msg = "Something went wrong";

    auto result = AnyError::init(alloc, msg, nullptr, nullptr);

    REQUIRE(result.hasValue());
    AnyError err = result.takeValue();

    CHECK_EQ(err.message(), msg);
    CHECK_FALSE(err.rawPayload().hasValue());
    CHECK_FALSE(err.payloadType().hasValue());
    CHECK_FALSE(err.cause().hasValue());
}

TEST_CASE("[AnyError] init with empty message and payload") {
    Allocator alloc;
    Example payload = {42};

    auto result = AnyError::init(alloc, StringSlice(), &payload, exampleGoodType);

    REQUIRE(result.hasValue());
    AnyError err = result.takeValue();

    CHECK_GE(err.message().len(), 0);
    REQUIRE(err.rawPayload().hasValue());
    REQUIRE(err.payloadType().hasValue());
    CHECK_EQ(err.payloadType().value(), exampleGoodType);

    const Example* storedPayload = static_cast<const Example*>(err.rawPayload().value());
    CHECK(storedPayload->a == 42);
}

TEST_CASE("[AnyError] init with message and payload") {
    StringSlice msg = "Error with payload";
    Example payload = {123};

    auto result = AnyError::init({}, msg, &payload, exampleGoodType);

    REQUIRE(result.hasValue());
    AnyError err = result.takeValue();

    CHECK_EQ(err.message(), msg);
    REQUIRE(err.rawPayload().hasValue());
    REQUIRE(err.payloadType().hasValue());
    CHECK_EQ(err.payloadType().value(), exampleGoodType);

    const Example* storedPayload = static_cast<const Example*>(err.rawPayload().value());
    CHECK_EQ(storedPayload->a, 123);
}

TEST_CASE("[AnyError] init with empty message and payload fallible copy doesn't fail cause not copy") {
    const Type* fallibleType = makeExampleFallibleType();
    Example payload = {999};

    auto result = AnyError::init({}, StringSlice(), &payload, fallibleType);

    REQUIRE(!result.hasErr());
}

TEST_CASE("[AnyError] init with message and payload doesn't fail cause not copy") {
    StringSlice msg = "This should fail";
    const Type* fallibleType = makeExampleFallibleType();
    Example payload = {777};

    auto result = AnyError::init({}, msg, &payload, fallibleType);

    REQUIRE(!result.hasErr());
}

TEST_CASE("[AnyError] init with long message") {
    StringSlice longMsg = "some super long string that goes way past any possible SSO";

    auto result = AnyError::init({}, longMsg, nullptr, nullptr);

    REQUIRE(result.hasValue());
    AnyError err = result.takeValue();

    CHECK_EQ(err.message(), longMsg);
}

TEST_CASE("[AnyError] initCause with single cause") {
    auto causeResult = AnyError::init({}, "Root cause", nullptr, nullptr);
    REQUIRE(causeResult.hasValue());
    AnyError cause = causeResult.takeValue();

    auto result = AnyError::initCause(std::move(cause), "Top-level error", nullptr, nullptr);

    REQUIRE(result.hasValue());
    AnyError err = result.takeValue();

    CHECK_EQ(err.message(), "Top-level error");
    REQUIRE(err.cause().hasValue());
    CHECK_EQ(err.cause().value().message(), "Root cause");
    CHECK_FALSE(err.cause().value().cause().hasValue());
}

TEST_CASE("[AnyError] initCause with message and payload") {
    Example causePayload = {100};
    auto causeResult = AnyError::init({}, "Database error", &causePayload, exampleGoodType);
    REQUIRE(causeResult.hasValue());
    AnyError cause = causeResult.takeValue();

    Example topPayload = {200};
    auto result = AnyError::initCause(std::move(cause), "Failed to load user", &topPayload, exampleGoodType);

    REQUIRE(result.hasValue());
    AnyError err = result.takeValue();

    CHECK_EQ(err.message(), "Failed to load user");
    REQUIRE(err.rawPayload().hasValue());
    CHECK_EQ(static_cast<const Example*>(err.rawPayload().value())->a, 200);

    REQUIRE(err.cause().hasValue());
    CHECK_EQ(err.cause().value().message(), "Database error");
    REQUIRE(err.cause().value().rawPayload().hasValue());
    CHECK_EQ(static_cast<const Example*>(err.cause().value().rawPayload().value())->a, 100);
}

TEST_CASE("[AnyError] initCause with nested causes (2 levels deep)") {
    auto deepestResult = AnyError::init({}, "Network timeout", nullptr, nullptr);
    REQUIRE(deepestResult.hasValue());
    AnyError deepest = deepestResult.takeValue();

    auto middleResult = AnyError::initCause(std::move(deepest), "Connection failed", nullptr, nullptr);
    REQUIRE(middleResult.hasValue());
    AnyError middle = middleResult.takeValue();

    auto topResult = AnyError::initCause(std::move(middle), "Failed to fetch data", nullptr, nullptr);
    REQUIRE(topResult.hasValue());
    AnyError top = topResult.takeValue();

    CHECK_EQ(top.message(), "Failed to fetch data");

    REQUIRE(top.cause().hasValue());
    CHECK_EQ(top.cause().value().message(), "Connection failed");

    REQUIRE(top.cause().value().cause().hasValue());
    CHECK_EQ(top.cause().value().cause().value().message(), "Network timeout");

    CHECK_FALSE(top.cause().value().cause().value().cause().hasValue());
}

TEST_CASE("[AnyError] initCause with fallible payload copy doesn't fail not copy") {
    auto causeResult = AnyError::init({}, "Valid cause", nullptr, nullptr);
    REQUIRE(causeResult.hasValue());
    AnyError cause = causeResult.takeValue();

    const Type* fallibleType = makeExampleFallibleType();
    Example payload = {42};

    auto result = AnyError::initCause(std::move(cause), "Should fail", &payload, fallibleType);

    REQUIRE(!result.hasErr());
}

TEST_CASE("[AnyError] clone with no payload or cause") {
    auto initResult = AnyError::init({}, "Simple error", nullptr, nullptr);
    REQUIRE(initResult.hasValue());
    const AnyError& original = initResult.value();

    auto cloneResult = original.clone();

    REQUIRE(cloneResult.hasValue());
    AnyError cloned = cloneResult.takeValue();

    CHECK_EQ(cloned.message(), "Simple error");
    CHECK_FALSE(cloned.rawPayload().hasValue());
    CHECK_FALSE(cloned.cause().hasValue());

    CHECK_EQ(original.message(), "Simple error");
}

TEST_CASE("[AnyError] clone with payload") {
    Example payload = {777};
    auto initResult = AnyError::init({}, "Error with data", &payload, exampleGoodType);
    REQUIRE(initResult.hasValue());
    const AnyError& original = initResult.value();

    auto cloneResult = original.clone();

    REQUIRE(cloneResult.hasValue());
    AnyError cloned = cloneResult.takeValue();

    CHECK_EQ(cloned.message(), "Error with data");
    REQUIRE(cloned.rawPayload().hasValue());
    CHECK_EQ(static_cast<const Example*>(cloned.rawPayload().value())->a, 777);

    CHECK_EQ(static_cast<const Example*>(original.rawPayload().value())->a, 777);
}

TEST_CASE("[AnyError] clone with single cause") {
    auto causeResult = AnyError::init({}, "Original cause", nullptr, nullptr);
    REQUIRE(causeResult.hasValue());
    AnyError cause = causeResult.takeValue();

    auto initResult = AnyError::initCause(std::move(cause), "Top error", nullptr, nullptr);
    REQUIRE(initResult.hasValue());
    const AnyError& original = initResult.value();

    auto cloneResult = original.clone();

    REQUIRE(cloneResult.hasValue());
    AnyError cloned = cloneResult.takeValue();

    CHECK_EQ(cloned.message(), "Top error");
    REQUIRE(cloned.cause().hasValue());
    CHECK_EQ(cloned.cause().value().message(), "Original cause");
}

TEST_CASE("[AnyError] clone with nested causes (2 levels deep)") {
    auto deepResult = AnyError::init({}, "Deep root cause", nullptr, nullptr);
    REQUIRE(deepResult.hasValue());
    AnyError deep = deepResult.takeValue();

    auto middleResult = AnyError::initCause(std::move(deep), "Middle cause", nullptr, nullptr);
    REQUIRE(middleResult.hasValue());
    AnyError middle = middleResult.takeValue();

    auto topResult = AnyError::initCause(std::move(middle), "Top error", nullptr, nullptr);
    REQUIRE(topResult.hasValue());
    const AnyError& original = topResult.value();

    auto cloneResult = original.clone();

    REQUIRE(cloneResult.hasValue());
    AnyError cloned = cloneResult.takeValue();

    CHECK_EQ(cloned.message(), "Top error");

    REQUIRE(cloned.cause().hasValue());
    CHECK_EQ(cloned.cause().value().message(), "Middle cause");

    REQUIRE(cloned.cause().value().cause().hasValue());
    CHECK_EQ(cloned.cause().value().cause().value().message(), "Deep root cause");

    CHECK_FALSE(cloned.cause().value().cause().value().cause().hasValue());

    CHECK_EQ(original.message(), "Top error");
    REQUIRE(original.cause().hasValue());
    CHECK_EQ(original.cause().value().message(), "Middle cause");
}

TEST_CASE("[AnyError] clone with nested causes and payloads") {
    Example deepPayload = {111};
    auto deepResult = AnyError::init({}, "Deep error", &deepPayload, exampleGoodType);
    REQUIRE(deepResult.hasValue());
    AnyError deep = deepResult.takeValue();

    Example topPayload = {222};
    auto topResult = AnyError::initCause(std::move(deep), "Top error", &topPayload, exampleGoodType);
    REQUIRE(topResult.hasValue());
    const AnyError& original = topResult.value();

    auto cloneResult = original.clone();

    REQUIRE(cloneResult.hasValue());
    AnyError cloned = cloneResult.takeValue();

    CHECK_EQ(cloned.message(), "Top error");
    REQUIRE(cloned.rawPayload().hasValue());
    CHECK_EQ(static_cast<const Example*>(cloned.rawPayload().value())->a, 222);

    REQUIRE(cloned.cause().hasValue());
    CHECK_EQ(cloned.cause().value().message(), "Deep error");
    REQUIRE(cloned.cause().value().rawPayload().hasValue());
    CHECK_EQ(static_cast<const Example*>(cloned.cause().value().rawPayload().value())->a, 111);
}

TEST_CASE("[AnyError] clone empty") {
    AnyError empty;

    auto cloneResult = empty.clone();

    REQUIRE(cloneResult.hasValue());
    AnyError cloned = cloneResult.takeValue();

    CHECK_GE(cloned.message().len(), 0);
    CHECK_FALSE(cloned.rawPayload().hasValue());
    CHECK_FALSE(cloned.cause().hasValue());
}

TEST_CASE("[AnyError] clone with fallible payload copy fails") {
    const Type* fallibleType = makeExampleFallibleType();
    Example payload = {999};

    auto initResult = AnyError::init({}, "Will fail to clone", &payload, fallibleType);
    REQUIRE(initResult.hasValue());
    const AnyError& original = initResult.value();

    auto cloneResult = original.clone();

    REQUIRE(cloneResult.hasErr());
    CHECK_EQ(cloneResult.err(), ProgramError::Unknown);
}

TEST_CASE("[AnyError] clone fails if nested cause has fallible payload") {
    const Type* fallibleType = makeExampleFallibleType();
    Example badPayload = {666};
    auto causeResult = AnyError::init({}, "Bad cause", &badPayload, fallibleType);
    REQUIRE(causeResult.hasValue());
    AnyError cause = causeResult.takeValue();

    Example goodPayload = {123};
    auto topResult = AnyError::initCause(std::move(cause), "Top error", &goodPayload, exampleGoodType);
    REQUIRE(topResult.hasValue());
    const AnyError& original = topResult.value();

    auto cloneResult = original.clone();

    REQUIRE(cloneResult.hasErr());
    CHECK_EQ(cloneResult.err(), ProgramError::Unknown);
}

#endif // SYNC_LIB_WITH_TESTS