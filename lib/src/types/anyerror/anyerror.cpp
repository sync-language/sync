#include "anyerror.hpp"
#include "../../core/core_internal.h"
#include "../string/string.hpp"
#include "../type_info.hpp"
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

Result<AnyError, ProgramError> sy::AnyError::init(Allocator alloc, StringSlice message, void* payload,
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
        return Error(ProgramError::OutOfMemory);
    }

    auto strRes = StringUnmanaged::copyConstructSlice(message, alloc);
    if (strRes.hasErr()) {
        alloc.freeObject(implRes.value());
        return Error(ProgramError::OutOfMemory);
    }

    void* payloadMem = nullptr;
    if (!nullPayload) {
        auto payloadRes = alloc.allocAlignedArray<uint8_t>(payloadType->sizeType, payloadType->alignType);
        if (payloadRes.hasErr()) {
            alloc.freeObject(implRes.value());
            strRes.value().destroy(alloc);
            return Error(ProgramError::OutOfMemory);
        }
        payloadMem = payloadRes.value();
        auto copyErr = payloadType->copyConstructObj(payloadMem, payload);
        if (copyErr.hasErr()) {
            alloc.freeObject(implRes.value());
            alloc.freeAlignedArray(payloadRes.value(), payloadType->sizeType, payloadType->alignType);
            strRes.value().destroy(alloc);
            return Error(copyErr.takeErr());
        }
    }

    Impl* self = implRes.value();
    new (self) Impl();
    self->alloc = alloc;
    new (&self->message) StringUnmanaged(strRes.takeValue());

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

Result<AnyError, ProgramError> sy::AnyError::initCause(AnyError cause, StringSlice message, void* payload,
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
            alloc.freeAlignedArray(implRes.value(), type->sizeType, type->alignType);
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

TEST_CASE("[AnyError] init with empty message and payload fallible copy fails") {
    const Type* fallibleType = makeExampleFallibleType();
    Example payload = {999};

    auto result = AnyError::init({}, StringSlice(), &payload, fallibleType);

    REQUIRE(result.hasErr());
    CHECK_EQ(result.err(), ProgramError::Unknown);
}

TEST_CASE("[AnyError] init with message and payload (fallible copy fails)") {
    StringSlice msg = "This should fail";
    const Type* fallibleType = makeExampleFallibleType();
    Example payload = {777};

    auto result = AnyError::init({}, msg, &payload, fallibleType);

    REQUIRE(result.hasErr());
    CHECK_EQ(result.err(), ProgramError::Unknown);
}

TEST_CASE("[AnyError] init cleans up on payload copy failure") {
    StringSlice msg = "Allocated message that should be cleaned up";
    const Type* fallibleType = makeExampleFallibleType();
    Example payload = {456};

    auto result = AnyError::init({}, msg, &payload, fallibleType);

    REQUIRE(result.hasErr());
    CHECK_EQ(result.err(), ProgramError::Unknown);
}

TEST_CASE("[AnyError] init with long message") {
    StringSlice longMsg = "some super long string that goes way past any possible SSO";

    auto result = AnyError::init({}, longMsg, nullptr, nullptr);

    REQUIRE(result.hasValue());
    AnyError err = result.takeValue();

    CHECK_EQ(err.message(), longMsg);
}

#endif // SYNC_LIB_WITH_TESTS