#include "file_literals.hpp"
#include "../../util/unreachable.hpp"
#include "../../util/assert.hpp"
#include "token.hpp"
#include <tuple>

static bool wouldUnsigned64IntAddOverflow(uint64_t a, uint64_t b) 
{
    if(a == 0 || b == 0) return false;
    if(b > (UINT64_MAX - a)) {
        return true;
    }
    return false;
}

static bool wouldUnsigned64IntMulOverflow(uint64_t a, uint64_t b) 
{
    if(a == 0 || b == 0) return false;
    if(b > (UINT64_MAX / a)) {
        return true;
    }
    return false;
}

std::variant<NumberLiteral, sy::CompileError> NumberLiteral::create(
    const sy::StringSlice source, const uint32_t start, const uint32_t end
) {
    bool isNegative = false;
    bool parsingWholeAsInt = true;
    bool foundDecimal = false;
    uint32_t i = start;
    uint64_t wholePartInt = 0;
    double wholePartFloat = 0.0;
    NumberLiteral self{};
    
    if(source[start] == '-') {
        isNegative = true;
        i += 1;
    }

    if(source[i] == '.') {
        return std::variant<NumberLiteral, sy::CompileError>(
            sy::CompileError::createInvalidDecimalNumberLiteral());
    }

    for(; i < end; i++) {
        const char c = source[i];
        if(c == '.') {
            foundDecimal = true;
            i += 1;
            break;
        }
        else if(c >= '0' && c <= '9') {
            const uint64_t num = static_cast<uint64_t>(c - '0');
            if(!parsingWholeAsInt) {
                wholePartFloat *= 10;
                wholePartFloat += static_cast<double>(num);
            } else {
                if(wouldUnsigned64IntMulOverflow(wholePartInt, 10)) {
                    parsingWholeAsInt = false;
                    wholePartFloat = static_cast<double>(wholePartInt);
                    wholePartFloat *= 10;
                    wholePartFloat += static_cast<double>(num);
                    continue;
                }
                
                wholePartInt *= 10;
                if(wouldUnsigned64IntAddOverflow(wholePartInt, num)) {
                    parsingWholeAsInt = false;
                    wholePartFloat = static_cast<double>(wholePartInt);
                    wholePartFloat *= 10;
                    wholePartFloat += static_cast<double>(num);
                    continue;
                } else {
                    wholePartInt += num;
                }
            }
        }
        else {
            return std::variant<NumberLiteral, sy::CompileError>(
                sy::CompileError::createInvalidCharNumberLiteral());
        }
    }

    if(!foundDecimal) {
        sy_assert(i == end, "Invalid number literal string range");

        if(parsingWholeAsInt) {
            if(isNegative) {
                constexpr uint64_t MIN_I64_AS_U64 = 9223372036854775808ULL;
                if(wholePartInt == MIN_I64_AS_U64) {
                    self.kind_ = RepKind::Signed64;
                    self.rep_.signed64 = INT64_MIN;
                    return std::variant<NumberLiteral, sy::CompileError>(self);
                }
                else if(wholePartInt < MIN_I64_AS_U64) {
                    self.kind_ = RepKind::Signed64;
                    self.rep_.signed64 = static_cast<int64_t>(wholePartInt) * -1;
                    return std::variant<NumberLiteral, sy::CompileError>(self);
                }
                else {
                    wholePartFloat = static_cast<double>(wholePartInt);
                    wholePartFloat *= -1;
                    self.kind_ = RepKind::Float64;
                    self.rep_.float64 = wholePartFloat;    
                    return std::variant<NumberLiteral, sy::CompileError>(self);
                }
            } else {          
                self.kind_ = RepKind::Unsigned64;
                self.rep_.unsigned64 = wholePartInt;    
                return std::variant<NumberLiteral, sy::CompileError>(self);
            }
        }
    
        self.kind_ = RepKind::Float64;
        self.rep_.float64 = wholePartFloat;    
        return std::variant<NumberLiteral, sy::CompileError>(self);
    }

    double decimalPart = 0; 
    double denominator = 1;
    for(; i < end; i++) {
        const char c = source[i];
        if(c == '.') {  
            return std::variant<NumberLiteral, sy::CompileError>(
                sy::CompileError::createInvalidDecimalNumberLiteral());
        }
        else if(c >= '0' && c <= '9') {
            const double num = static_cast<double>(c - '0');
            decimalPart *= 10.0;
            decimalPart += num;
            denominator *= 10;
        }
        else {
            return std::variant<NumberLiteral, sy::CompileError>(
                sy::CompileError::createInvalidCharNumberLiteral());
        }
    }

    const double fraction = decimalPart / denominator;
    const double whole = parsingWholeAsInt ? static_cast<double>(wholePartInt) : wholePartFloat;

    double entireNumber = whole + fraction;
    if(isNegative) {
        entireNumber *= -1.0;
    }

    self.kind_ = RepKind::Float64;
    self.rep_.float64 = entireNumber;    
    return std::variant<NumberLiteral, sy::CompileError>(self);
}

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

double NumberLiteral::asFloat64() const
{
    switch(this->kind_) {
        case RepKind::Unsigned64: {
            return static_cast<double>(this->rep_.unsigned64);
        }
        case RepKind::Signed64: {
            return static_cast<double>(this->rep_.signed64);
        }
        case RepKind::Float64: {
            return this->rep_.float64;
        }
        default: unreachable();
    }
}

#ifndef SYNC_LIB_NO_TESTS

#include "../../doctest.h"

TEST_SUITE("number literal") {
    TEST_CASE("positive single digit") {
        for(int i = 0; i < 10; i++) {
            const char asChar = static_cast<char>(i) + '0';
            const char buf[2] = { asChar, '\0'};
            auto result = NumberLiteral::create(sy::StringSlice(buf, 1), 0, 1);
            CHECK(std::holds_alternative<NumberLiteral>(result));
            NumberLiteral num = std::get<NumberLiteral>(result);
            CHECK_EQ(num.asFloat64(), static_cast<double>(i));
            CHECK_EQ(std::get<int64_t>(num.asSigned64()), static_cast<int64_t>(i));
            CHECK_EQ(std::get<uint64_t>(num.asUnsigned64()), static_cast<uint64_t>(i));
        }
    }
}

#endif // SYNC_LIB_NO_TESTS
