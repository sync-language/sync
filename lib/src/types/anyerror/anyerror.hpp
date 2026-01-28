#ifndef _SY_TYPES_ANYERROR_ANYERROR_HPP_
#define _SY_TYPES_ANYERROR_ANYERROR_HPP_

#include "../../mem/allocator.hpp"
#include "../../program/program_error.hpp"
#include "../option/option.hpp"
#include "../result/result.hpp"
#include "../string/string_slice.hpp"

namespace sy {

class Type;

class AnyError {
  public:
    /// Makes an empty error type, holding no data.
    AnyError() = default;

    /// @param alloc Memory allocator.
    /// @param message Can be an empty string.
    /// @param payload If non-null, `payloadType` must also be non-null. Takes ownership, moving the `payload` data into
    /// itself by memcpy.
    /// @param payloadType If non-null, `payload` must also be non-null.
    /// @return The new `AnyError` object, or allocation failure.
    static Result<AnyError, AllocErr> init(Allocator alloc, StringSlice message, void* payload,
                                           const Type* payloadType /* TODO stack trace*/) noexcept;

    /// @param alloc Memory allocator.
    /// @param message Can be an empty string.
    /// @param payload If non-null, `payloadType` must also be non-null. Takes ownership, moving the `payload` data
    /// into itself by memcpy.
    /// @param payloadType If non-null, `payload` must also be non-null.
    /// @warning Calls `AnyError::init()`, calling the sync fatal handler if allocation fails.
    AnyError(Allocator alloc, StringSlice message, void* payload, const Type* payloadType) noexcept;

    /// @param cause The cause of the previous error, allowing chaining multiple errors together. Uses the `cause`'s
    /// allocator.
    /// @param message Can be an empty string.
    /// @param payload If non-null, `payloadType` must also be non-null. Takes ownership, moving the `payload` data
    /// into itself by memcpy.
    /// @param payloadType If non-null, `payload` must also be non-null.
    /// @return The new `AnyError` object, or program error, which could indicate memory allocation failure, or that
    /// the clone function on the payload type failed.
    static Result<AnyError, AllocErr> initCause(AnyError cause, StringSlice message, void* payload,
                                                const Type* payloadType /* TODO stack trace*/) noexcept;

    /// @param cause The cause of the previous error, allowing chaining multiple errors together. Uses the `cause`'s
    /// allocator.
    /// @param message Can be an empty string.
    /// @param payload If non-null, `payloadType` must also be non-null. Takes ownership, moving the `payload` data
    /// into itself by memcpy.
    /// @param payloadType If non-null, `payload` must also be non-null.
    /// @warning Calls `AnyError::initCause()`, calling the sync fatal handler if allocation fails.
    AnyError(AnyError cause, StringSlice message, void* payload, const Type* payloadType) noexcept;

    AnyError(AnyError&& other) noexcept;

    AnyError& operator=(AnyError&& other) noexcept;

    /// @return The new `AnyError` object, or program error, which could indicate memory allocation failure, or that the
    /// clone function on the payload type failed.
    Result<AnyError, ProgramError> clone() const noexcept;

    /// @warning Calls `other.clone()`, calling the sync fatal handler if there is any failure.
    AnyError(const AnyError& other) noexcept;

    /// @warning Calls `other.clone()`, calling the sync fatal handler if there is any failure.
    AnyError& operator=(const AnyError& other) noexcept;

    ~AnyError() noexcept;

    /// @return May return an empty string if the AnyError has no message, or is an empty error
    StringSlice message() const noexcept;

    Option<AnyError&> cause() noexcept;

    Option<const AnyError&> cause() const noexcept;

    Option<void*> rawPayload() noexcept;

    Option<const void*> rawPayload() const noexcept;

    Option<const Type*> payloadType() const noexcept;

    // TODO stack trace and source location (or them together idk)

  private:
    struct Impl;
    friend struct Impl;
    Impl* impl_ = nullptr;
};
} // namespace sy

#endif // _SY_TYPES_ANYERROR_ANYERROR_HPP_