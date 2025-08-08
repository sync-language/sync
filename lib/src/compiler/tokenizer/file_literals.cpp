#include "file_literals.hpp"

std::variant<uint64_t, sy::CompileError> NumberLiteral::asUnsigned64() const
{
    uint64_t val = 0;

    switch(this->kind_) {
        case RepKind::Unsigned64: {
            val = this->rep_.unsigned64;
        } break;
        case RepKind::Signed64: {
            const int64_t temp = this->rep_.signed64;
            if(temp < 0) {
                return std::variant<uint64_t, sy::CompileError>(
                    sy::CompileError::createNegativeToUnsignedIntConversion());
            }
            val = static_cast<uint64_t>(temp);
        } break;
        case RepKind::Float64: {
            const double temp = this->rep_.float64;
            if(temp < 0.0) {
                return std::variant<uint64_t, sy::CompileError>(
                    sy::CompileError::createFloatOutsideIntRangeConversion());
            }
            constexpr double maxU64AsDouble = static_cast<double>(UINT64_MAX);
            if(temp > maxU64AsDouble) {
                return std::variant<uint64_t, sy::CompileError>(
                    sy::CompileError::createFloatOutsideIntRangeConversion());
            }
            val = static_cast<uint64_t>(temp);
        } break;
    }

    return std::variant<uint64_t, sy::CompileError>(val);
}

std::variant<int64_t, sy::CompileError> NumberLiteral::asSigned64() const
{
    int64_t val = 0;

    switch(this->kind_) {
        case RepKind::Unsigned64: {
            const uint64_t temp = this->rep_.unsigned64;
            if(temp > static_cast<uint64_t>(INT64_MAX)) {
                return std::variant<int64_t, sy::CompileError>(
                    sy::CompileError::createUnsignedOutsideIntRangeConversion());
            }
            val = static_cast<int64_t>(temp);
        } break;
        case RepKind::Signed64: {
            val = this->rep_.signed64;
        } break;
        case RepKind::Float64: {
            const double temp = this->rep_.float64;
            constexpr double minI64AsDouble = static_cast<double>(INT64_MIN);
            if(temp < minI64AsDouble) {
                return std::variant<int64_t, sy::CompileError>(
                    sy::CompileError::createFloatOutsideIntRangeConversion());
            }
            constexpr double maxI64AsDouble = static_cast<double>(INT64_MAX);
            if(temp > maxI64AsDouble) {
                return std::variant<int64_t, sy::CompileError>(
                    sy::CompileError::createFloatOutsideIntRangeConversion());
            }
            val = static_cast<uint64_t>(temp);
        } break;
    }

    return std::variant<int64_t, sy::CompileError>(val);
}
