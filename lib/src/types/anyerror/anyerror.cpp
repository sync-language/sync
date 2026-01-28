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

    ~Impl() noexcept { message.destroy(alloc); }
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
            return Error(ProgramError::OutOfMemory);
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
