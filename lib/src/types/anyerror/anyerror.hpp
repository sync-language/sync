#ifndef _SY_TYPES_ANYERROR_ANYERROR_HPP_
#define _SY_TYPES_ANYERROR_ANYERROR_HPP_

#include "../../core/exceptional.hpp"
#include "../../mem/allocator.hpp"
#include "../../util/move_and_leak.hpp"
#include "../option/option.hpp"
#include "../reflect_fwd.hpp"
#include "../result/result.hpp"
#include "../string/string_slice.hpp"

namespace sy {

class Type;
class AnyError;

namespace internal {
SY_API Result<AnyError, AllocErr> sy_anyerror_init_impl(StringSlice msg, void* payload,
                                                        const Type* payloadType,
                                                        Option<AnyError> cause,
                                                        Allocator alloc) noexcept;
}

class AnyError {
  public:
    /// Makes an empty error type, holding no data.
    AnyError() = default;

    /// Makes an `AnyError` containing a `Exceptional` error value. Does not allocate any memory,
    /// so this operation is infallible.
    AnyError(Exceptional err) noexcept;

    template <typename T>
    static Result<AnyError, AllocErr> init(StringSlice msg, Option<T> payload,
                                           Option<AnyError> cause, Allocator alloc = {}) noexcept;

    AnyError(AnyError&& other) noexcept;

    AnyError& operator=(AnyError&& other) noexcept;

    /// @return The new `AnyError` object, or program error, which could indicate memory allocation
    /// failure, or that the clone function on the payload type failed.
    Result<AnyError, AnyError> clone() const noexcept;

    /// @warning Calls `other.clone()`, calling the sync fatal handler if there is any failure.
    AnyError(const AnyError& other) noexcept;

    /// @warning Calls `other.clone()`, calling the sync fatal handler if there is any failure.
    AnyError& operator=(const AnyError& other) noexcept;

    ~AnyError() noexcept;

    Option<Exceptional> exceptional() const noexcept;

    /// @return May return an empty string if the AnyError has no message, or is an empty error.
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
    friend SY_API Result<AnyError, AllocErr>
    internal::sy_anyerror_init_impl(StringSlice msg, void* payload, const Type* payloadType,
                                    Option<AnyError> cause, Allocator alloc) noexcept;

    uintptr_t impl_ = 0;
};

template <typename T>
inline Result<AnyError, AllocErr> AnyError::init(StringSlice msg, Option<T> payload,
                                                 Option<AnyError> cause, Allocator alloc) noexcept {
    void* payloadMem = nullptr;
    const Type* payloadType = nullptr;
    if (payload.hasValue()) {
        payloadMem = &payload.value();
        payloadType = ::sy::Reflect<T>::get();
    }
    auto res = internal::sy_anyerror_init_impl(msg, payloadMem, payloadType, std::move(cause), alloc);
    internal::moveAndLeak(payload);
    return res;
}

} // namespace sy

#endif // _SY_TYPES_ANYERROR_ANYERROR_HPP_
