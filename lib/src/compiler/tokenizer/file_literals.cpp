#include "file_literals.hpp"
#include "../../util/assert.hpp"
#include "../../util/unreachable.hpp"
#include "token.hpp"
#include <new>
#include <tuple>
#include <cstring>

static bool wouldUnsigned64IntAddOverflow(uint64_t a, uint64_t b) {
  if (a == 0 || b == 0)
    return false;
  if (b > (UINT64_MAX - a)) {
    return true;
  }
  return false;
}

static bool wouldUnsigned64IntMulOverflow(uint64_t a, uint64_t b) {
  if (a == 0 || b == 0)
    return false;
  if (b > (UINT64_MAX / a)) {
    return true;
  }
  return false;
}

std::variant<NumberLiteral, sy::CompileError>
NumberLiteral::create(const sy::StringSlice source, const uint32_t start,
                      const uint32_t end) {
  bool isNegative = false;
  bool parsingWholeAsInt = true;
  bool foundDecimal = false;
  uint32_t i = start;
  uint64_t wholePartInt = 0;
  double wholePartFloat = 0.0;
  NumberLiteral self{};

  if (source[start] == '-') {
    isNegative = true;
    i += 1;
  }

  if (source[i] == '.') {
    return std::variant<NumberLiteral, sy::CompileError>(
        sy::CompileError::createInvalidDecimalNumberLiteral());
  }

  for (; i < end; i++) {
    const char c = source[i];
    if (c == '.') {
      foundDecimal = true;
      i += 1;
      break;
    } else if (c >= '0' && c <= '9') {
      const uint64_t num = static_cast<uint64_t>(c - '0');
      if (!parsingWholeAsInt) {
        wholePartFloat *= 10;
        wholePartFloat += static_cast<double>(num);
      } else {
        if (wouldUnsigned64IntMulOverflow(wholePartInt, 10)) {
          parsingWholeAsInt = false;
          wholePartFloat = static_cast<double>(wholePartInt);
          wholePartFloat *= 10;
          wholePartFloat += static_cast<double>(num);
          continue;
        }

        wholePartInt *= 10;
        if (wouldUnsigned64IntAddOverflow(wholePartInt, num)) {
          parsingWholeAsInt = false;
          wholePartFloat = static_cast<double>(wholePartInt);
          wholePartFloat += static_cast<double>(num);
          continue;
        } else {
          wholePartInt += num;
        }
      }
    } else {
      return std::variant<NumberLiteral, sy::CompileError>(
          sy::CompileError::createInvalidCharNumberLiteral());
    }
  }

  if (!foundDecimal) {
    sy_assert(i == end, "Invalid number literal string range");

    if (parsingWholeAsInt) {
      if (isNegative) {
        constexpr uint64_t MIN_I64_AS_U64 = 9223372036854775808ULL;
        if (wholePartInt == MIN_I64_AS_U64) {
          self.kind_ = RepKind::Signed64;
          self.rep_.signed64 = INT64_MIN;
          return std::variant<NumberLiteral, sy::CompileError>(self);
        } else if (wholePartInt < MIN_I64_AS_U64) {
          self.kind_ = RepKind::Signed64;
          self.rep_.signed64 = static_cast<int64_t>(wholePartInt) * -1;
          return std::variant<NumberLiteral, sy::CompileError>(self);
        } else {
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
  for (; i < end; i++) {
    const char c = source[i];
    if (c == '.') {
      return std::variant<NumberLiteral, sy::CompileError>(
          sy::CompileError::createInvalidDecimalNumberLiteral());
    } else if (c >= '0' && c <= '9') {
      const double num = static_cast<double>(c - '0');
      decimalPart *= 10.0;
      decimalPart += num;
      denominator *= 10;
    } else {
      return std::variant<NumberLiteral, sy::CompileError>(
          sy::CompileError::createInvalidCharNumberLiteral());
    }
  }

  const double fraction = decimalPart / denominator;
  const double whole =
      parsingWholeAsInt ? static_cast<double>(wholePartInt) : wholePartFloat;

  double entireNumber = whole + fraction;
  if (isNegative) {
    entireNumber *= -1.0;
  }

  self.kind_ = RepKind::Float64;
  self.rep_.float64 = entireNumber;
  return std::variant<NumberLiteral, sy::CompileError>(self);
}

std::variant<uint64_t, sy::CompileError> NumberLiteral::asUnsigned64() const {
  uint64_t val = 0;

  switch (this->kind_) {
  case RepKind::Unsigned64: {
    val = this->rep_.unsigned64;
  } break;
  case RepKind::Signed64: {
    const int64_t temp = this->rep_.signed64;
    if (temp < 0) {
      return std::variant<uint64_t, sy::CompileError>(
          sy::CompileError::createNegativeToUnsignedIntConversion());
    }
    val = static_cast<uint64_t>(temp);
  } break;
  case RepKind::Float64: {
    const double temp = this->rep_.float64;
    if (temp < 0.0) {
      return std::variant<uint64_t, sy::CompileError>(
          sy::CompileError::createNegativeToUnsignedIntConversion());
    }
    constexpr double maxU64AsDouble = static_cast<double>(UINT64_MAX);
    if (temp > maxU64AsDouble) {
      return std::variant<uint64_t, sy::CompileError>(
          sy::CompileError::createFloatOutsideIntRangeConversion());
    }
    if (temp ==
        18446744073709551616.0) { // explicitly handle float rounding errors
      return std::variant<uint64_t, sy::CompileError>(
          sy::CompileError::createFloatOutsideIntRangeConversion());
    }
    val = static_cast<uint64_t>(temp);
  } break;
  }

  return std::variant<uint64_t, sy::CompileError>(val);
}

std::variant<int64_t, sy::CompileError> NumberLiteral::asSigned64() const {
  int64_t val = 0;

  switch (this->kind_) {
  case RepKind::Unsigned64: {
    const uint64_t temp = this->rep_.unsigned64;
    if (temp > static_cast<uint64_t>(INT64_MAX)) {
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
    if (temp < minI64AsDouble) {
      return std::variant<int64_t, sy::CompileError>(
          sy::CompileError::createFloatOutsideIntRangeConversion());
    }
    constexpr double maxI64AsDouble = static_cast<double>(INT64_MAX);
    if (temp > maxI64AsDouble) {
      return std::variant<int64_t, sy::CompileError>(
          sy::CompileError::createFloatOutsideIntRangeConversion());
    }
    if (temp == 9223372036854775808.0) { // explicitly handle float rounding
                                         // errors positive
      return std::variant<int64_t, sy::CompileError>(
          sy::CompileError::createFloatOutsideIntRangeConversion());
    }
    if (temp == -9223372036854775809.0) { // explicitly handle float rounding
                                          // errors negative
      return std::variant<int64_t, sy::CompileError>(
          sy::CompileError::createFloatOutsideIntRangeConversion());
    }
    val = static_cast<uint64_t>(temp);
  } break;
  }

  return std::variant<int64_t, sy::CompileError>(val);
}

double NumberLiteral::asFloat64() const {
  switch (this->kind_) {
  case RepKind::Unsigned64: {
    return static_cast<double>(this->rep_.unsigned64);
  }
  case RepKind::Signed64: {
    return static_cast<double>(this->rep_.signed64);
  }
  case RepKind::Float64: {
    return this->rep_.float64;
  }
  default:
    unreachable();
  }
}

static std::variant<CharLiteral, sy::CompileError>
parseEscapeSequence(const char *const start) {
  sy_assert((*start) == '\\',
            "Beginning of escape sequence must be the \'\\\' character");

  CharLiteral self;

  // https://en.wikipedia.org/wiki/Escape_sequences_in_C
  const char firstEscapedCharacter = start[1];
  switch (firstEscapedCharacter) {
  case 'a': {
    self.val = '\a';
  } break;
  case 'b': {
    self.val = '\b';
  } break;
  // '\e' maybe not supported?
  case 'f': {
    self.val = '\f';
  } break;
  case 'n': {
    self.val = '\n';
  } break;
  case 'r': {
    self.val = '\r';
  } break;
  case 't': {
    self.val = '\t';
  } break;
  case 'v': {
    self.val = '\v';
  } break;
  case '\\': {
    self.val = '\\';
  } break;
  case '\'': {
    self.val = '\'';
  } break;
  case '\"': {
    self.val = '\"';
  } break;
  case '?': {
    self.val = '\?';
  } break;
  // don't want to support octals at least for now
  case 'x':
  case 'X': {
    // TODO byte?
    return std::variant<CharLiteral, sy::CompileError>(
        sy::CompileError::createUnsupportedChar());
  } break;
  case 'u':
  case 'U': {
    // TODO unicode
    return std::variant<CharLiteral, sy::CompileError>(
        sy::CompileError::createUnsupportedChar());
  } break;
  default: {
    return std::variant<CharLiteral, sy::CompileError>(
        sy::CompileError::createInvalidEscapeSequence());
  } break;
  }

  return std::variant<CharLiteral, sy::CompileError>(self);
}

std::variant<CharLiteral, sy::CompileError>
CharLiteral::create(const sy::StringSlice source, const uint32_t start,
                    const uint32_t end) {
  const uint32_t actualEnd = end - 1;

  sy_assert(source[start] == '\'', "Invalid source start for char literal");
  sy_assert(source[actualEnd] == '\'', "Invalid source end for char literal");

  uint32_t i = start + 1;
  const char firstChar = source[i];
  if (static_cast<int>(firstChar) >=
      0b01111111) { // for now only support ascii stuff
    return std::variant<CharLiteral, sy::CompileError>(
        sy::CompileError::createUnsupportedChar());
  }

  CharLiteral self;
  if (source[i] == '\\') {
    sy_assert((i + 1) != actualEnd, "Invalid source range for char literal");

    auto result = parseEscapeSequence(&source.data()[i]);
    if (std::holds_alternative<sy::CompileError>(result)) {
      return result;
    }

    self = std::get<CharLiteral>(result);
    i += 2; // after escape sequence
  } else {
    self.val = sy::Char(source[i]);
    i += 1;
  }

  if (i != actualEnd) {
    return std::variant<CharLiteral, sy::CompileError>(
        sy::CompileError::createTooManyCharsInCharLiteral());
  }

  return std::variant<CharLiteral, sy::CompileError>(self);
}

std::variant<StringLiteral, sy::CompileError>
StringLiteral::create(const sy::StringSlice source, const uint32_t start,
                      const uint32_t end, sy::Allocator alloc) {
  const uint32_t actualEnd = end - 1;

  sy_assert(source[start] == '\"', "Invalid source start for string literal");
  sy_assert(source[actualEnd] == '\"', "Invalid source end for string literal");

  size_t capacity = (end - start);
  size_t length = 0;
  char *buf = nullptr;
  {
    auto result = sy::detail::mallocStringBuffer(capacity, alloc);
    if (!result.hasValue()) {
      return std::variant<StringLiteral, sy::CompileError>(
          sy::CompileError::createOutOfMemory());
    }
    buf = result.value();
  }

  const char *strIter = &source.data()[start + 1];
  const char *const strEnd = &source.data()[actualEnd];

  while (strIter != strEnd) {
    if (static_cast<int>(*strIter) >=
        0b01111111) { // for now only support ascii stuff
      return std::variant<StringLiteral, sy::CompileError>(
          sy::CompileError::createUnsupportedChar());
    }

    if ((*strIter) == '\\') {
      sy_assert((strIter + 1) != strEnd,
                "Invalid source range for string literal");

      auto result = parseEscapeSequence(strIter);
      if (std::holds_alternative<sy::CompileError>(result)) {
        return std::variant<StringLiteral, sy::CompileError>(
            sy::CompileError::createInvalidEscapeSequence());
      }

      buf[length] = std::get<CharLiteral>(result).val.cchar();
      length += 1;
      strIter = &strIter[2];
    } else {
      buf[length] = *strIter;
      length += 1;
      strIter = &strIter[1];
    }
  }

  StringLiteral self;
  (void)new (&self.str) sy::StringUnmanaged(
      sy::detail::StringUtils::makeRaw(buf, length, capacity, alloc));
  self.alloc = alloc;
  return std::variant<StringLiteral, sy::CompileError>(std::move(self));
}

StringLiteral::StringLiteral(StringLiteral &&other) noexcept
    : str(std::move(other.str)), alloc(other.alloc) {}

StringLiteral &StringLiteral::operator=(StringLiteral &&other) noexcept {
  this->str.moveAssign(std::move(other.str), other.alloc);
  this->alloc = other.alloc;
  return *this;
}

StringLiteral::~StringLiteral() noexcept { this->str.destroy(this->alloc); }

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"
#include <string>

using sy::CompileError;

TEST_SUITE("number literal") {
  TEST_CASE("positive single digit") {
    for (int i = 0; i < 10; i++) {
      const char asChar = static_cast<char>(i) + '0';
      const char buf[2] = {asChar, '\0'};
      auto result = NumberLiteral::create(sy::StringSlice(buf, 1), 0, 1);
      CHECK(std::holds_alternative<NumberLiteral>(result));
      NumberLiteral num = std::get<NumberLiteral>(result);
      CHECK_EQ(num.asFloat64(), static_cast<double>(i));
      CHECK_EQ(std::get<int64_t>(num.asSigned64()), static_cast<int64_t>(i));
      CHECK_EQ(std::get<uint64_t>(num.asUnsigned64()),
               static_cast<uint64_t>(i));
    }
  }

  TEST_CASE("two digits") {
    for (int i = 10; i < 100; i++) {
      const char asCharTens = static_cast<char>(i / 10) + '0';
      const char asCharOnes = static_cast<char>(i % 10) + '0';
      const char buf[3] = {asCharTens, asCharOnes, '\0'};
      auto result = NumberLiteral::create(sy::StringSlice(buf, 2), 0, 2);
      CHECK(std::holds_alternative<NumberLiteral>(result));
      NumberLiteral num = std::get<NumberLiteral>(result);
      CHECK_EQ(num.asFloat64(), static_cast<double>(i));
      CHECK_EQ(std::get<int64_t>(num.asSigned64()), static_cast<int64_t>(i));
      CHECK_EQ(std::get<uint64_t>(num.asUnsigned64()),
               static_cast<uint64_t>(i));
    }
  }

  TEST_CASE("many digits") {
    // The powers of 2 have a good mix of characters.
    // We go up to 2^62 because 2^63 is outside of signed 64 bit range
    constexpr uint64_t max = (1ULL << 62);
    for (uint64_t i = 128; i <= max; i <<= 1) {
      std::string numAsString = std::to_string(i);
      auto result = NumberLiteral::create(
          sy::StringSlice(numAsString.c_str(), numAsString.size()), 0,
          static_cast<uint32_t>(numAsString.size()));
      CHECK(std::holds_alternative<NumberLiteral>(result));
      NumberLiteral num = std::get<NumberLiteral>(result);
      CHECK_EQ(num.asFloat64(), static_cast<double>(i));
      CHECK_EQ(std::get<int64_t>(num.asSigned64()), static_cast<int64_t>(i));
      CHECK_EQ(std::get<uint64_t>(num.asUnsigned64()),
               static_cast<uint64_t>(i));
    }
  }

  TEST_CASE("negative single digit") {
    for (int i = -1; i > -10; i--) {
      std::string numAsString = std::to_string(i);
      auto result = NumberLiteral::create(
          sy::StringSlice(numAsString.c_str(), numAsString.size()), 0,
          static_cast<uint32_t>(numAsString.size()));
      CHECK(std::holds_alternative<NumberLiteral>(result));
      NumberLiteral num = std::get<NumberLiteral>(result);
      CHECK_EQ(num.asFloat64(), static_cast<double>(i));
      CHECK_EQ(std::get<int64_t>(num.asSigned64()), static_cast<int64_t>(i));

      CHECK_EQ(std::get<CompileError>(num.asUnsigned64()).kind(),
               CompileError::Kind::NegativeToUnsignedIntConversion);
    }
  }

  TEST_CASE("negative two digits") {
    for (int i = -10; i > -100; i--) {
      std::string numAsString = std::to_string(i);
      auto result = NumberLiteral::create(
          sy::StringSlice(numAsString.c_str(), numAsString.size()), 0,
          static_cast<uint32_t>(numAsString.size()));
      CHECK(std::holds_alternative<NumberLiteral>(result));
      NumberLiteral num = std::get<NumberLiteral>(result);
      CHECK_EQ(num.asFloat64(), static_cast<double>(i));
      CHECK_EQ(std::get<int64_t>(num.asSigned64()), static_cast<int64_t>(i));

      CHECK_EQ(std::get<CompileError>(num.asUnsigned64()).kind(),
               CompileError::Kind::NegativeToUnsignedIntConversion);
    }
  }

  TEST_CASE("negative many digits") {
    constexpr int64_t min = (1LL << 62) * -1LL;
    for (int64_t i = -128; i > min; i *= 2) {
      std::string numAsString = std::to_string(i);
      auto result = NumberLiteral::create(
          sy::StringSlice(numAsString.c_str(), numAsString.size()), 0,
          static_cast<uint32_t>(numAsString.size()));
      CHECK(std::holds_alternative<NumberLiteral>(result));
      NumberLiteral num = std::get<NumberLiteral>(result);
      CHECK_EQ(num.asFloat64(), static_cast<double>(i));
      CHECK_EQ(std::get<int64_t>(num.asSigned64()), static_cast<int64_t>(i));

      CHECK_EQ(std::get<CompileError>(num.asUnsigned64()).kind(),
               CompileError::Kind::NegativeToUnsignedIntConversion);
    }
  }

  TEST_CASE("limits") {
    { // max 64 bit unsigned int
      std::string numAsString = "18446744073709551615";
      auto result = NumberLiteral::create(
          sy::StringSlice(numAsString.c_str(), numAsString.size()), 0,
          static_cast<uint32_t>(numAsString.size()));
      CHECK(std::holds_alternative<NumberLiteral>(result));
      NumberLiteral num = std::get<NumberLiteral>(result);
      CHECK_EQ(num.asFloat64(), static_cast<double>(18446744073709551615ULL));

      CHECK_EQ(std::get<CompileError>(num.asSigned64()).kind(),
               CompileError::Kind::UnsignedOutsideIntRangeConversion);

      CHECK_EQ(std::get<uint64_t>(num.asUnsigned64()), 18446744073709551615ULL);
    }
    { // max 64 bit signed int
      std::string numAsString = "9223372036854775807";
      auto result = NumberLiteral::create(
          sy::StringSlice(numAsString.c_str(), numAsString.size()), 0,
          static_cast<uint32_t>(numAsString.size()));
      CHECK(std::holds_alternative<NumberLiteral>(result));
      NumberLiteral num = std::get<NumberLiteral>(result);
      CHECK_EQ(num.asFloat64(), static_cast<double>(9223372036854775807));
      CHECK_EQ(std::get<int64_t>(num.asSigned64()), 9223372036854775807);
      CHECK_EQ(std::get<uint64_t>(num.asUnsigned64()), 9223372036854775807);
    }
    { // min 64 bit signed int
      std::string numAsString = "-9223372036854775808";
      auto result = NumberLiteral::create(
          sy::StringSlice(numAsString.c_str(), numAsString.size()), 0,
          static_cast<uint32_t>(numAsString.size()));
      CHECK(std::holds_alternative<NumberLiteral>(result));
      NumberLiteral num = std::get<NumberLiteral>(result);
      CHECK_EQ(num.asFloat64(), static_cast<double>(INT64_MIN));
      CHECK_EQ(std::get<int64_t>(num.asSigned64()), INT64_MIN);

      CHECK_EQ(std::get<CompileError>(num.asUnsigned64()).kind(),
               CompileError::Kind::NegativeToUnsignedIntConversion);
    }

    { // 1 above max 64 bit unsigned int
      std::string numAsString = "18446744073709551616";
      auto result = NumberLiteral::create(
          sy::StringSlice(numAsString.c_str(), numAsString.size()), 0,
          static_cast<uint32_t>(numAsString.size()));
      CHECK(std::holds_alternative<NumberLiteral>(result));
      NumberLiteral num = std::get<NumberLiteral>(result);
      CHECK_EQ(num.asFloat64(), 18446744073709551616.0);

      CHECK_EQ(std::get<CompileError>(num.asSigned64()).kind(),
               CompileError::Kind::FloatOutsideIntRangeConversion);
      CHECK_EQ(std::get<CompileError>(num.asUnsigned64()).kind(),
               CompileError::Kind::FloatOutsideIntRangeConversion);
    }
    { // 1 above max 64 bit signed int
      std::string numAsString = "9223372036854775808";
      auto result = NumberLiteral::create(
          sy::StringSlice(numAsString.c_str(), numAsString.size()), 0,
          static_cast<uint32_t>(numAsString.size()));
      CHECK(std::holds_alternative<NumberLiteral>(result));
      NumberLiteral num = std::get<NumberLiteral>(result);
      CHECK_EQ(num.asFloat64(), 9223372036854775808.0);

      CHECK_EQ(std::get<CompileError>(num.asSigned64()).kind(),
               CompileError::Kind::UnsignedOutsideIntRangeConversion);

      CHECK_EQ(std::get<uint64_t>(num.asUnsigned64()), 9223372036854775808ULL);
    }
    { // 1 below min 64 bit signed int
      std::string numAsString = "-9223372036854775809";
      auto result = NumberLiteral::create(
          sy::StringSlice(numAsString.c_str(), numAsString.size()), 0,
          static_cast<uint32_t>(numAsString.size()));
      CHECK(std::holds_alternative<NumberLiteral>(result));
      NumberLiteral num = std::get<NumberLiteral>(result);
      CHECK_EQ(num.asFloat64(), -9223372036854775809.0);

      CHECK_EQ(std::get<CompileError>(num.asSigned64()).kind(),
               CompileError::Kind::FloatOutsideIntRangeConversion);

      CHECK_EQ(std::get<CompileError>(num.asUnsigned64()).kind(),
               CompileError::Kind::NegativeToUnsignedIntConversion);
    }
  }
}

TEST_SUITE("char literal") {
  TEST_CASE("ascii printable characters non-escape") {
    // https://www.ascii-code.com/
    // Character codes 32 to 127
    auto asciiTest = [](char c) {
      const std::string charStr = std::string("\'") + c + "\'";
      auto result =
          CharLiteral::create(sy::StringSlice(charStr.c_str(), charStr.size()),
                              0, static_cast<uint32_t>(charStr.size()));

      CHECK(std::holds_alternative<CharLiteral>(result));
      CharLiteral parsedChar = std::get<CharLiteral>(result);
      CHECK_EQ(parsedChar.val, sy::Char(c));
    };

    asciiTest(' ');
    asciiTest('!');
    asciiTest('#');
    asciiTest('$');
    asciiTest('%');
    asciiTest('&');
    asciiTest('(');
    asciiTest(')');
    asciiTest('*');
    asciiTest('+');
    asciiTest(',');
    asciiTest('-');
    asciiTest('.');
    asciiTest('/');
    for (char c = '0'; c <= '9'; c++) {
      asciiTest(c);
    }
    asciiTest(':');
    asciiTest(';');
    asciiTest('<');
    asciiTest('=');
    asciiTest('>');
    asciiTest('?');
    asciiTest('@');
    for (char c = 'A'; c <= 'Z'; c++) {
      asciiTest(c);
    }
    asciiTest('[');
    asciiTest(']');
    asciiTest('^');
    asciiTest('_');
    asciiTest('`');
    for (char c = 'a'; c <= 'z'; c++) {
      asciiTest(c);
    }
    asciiTest('{');
    asciiTest('|');
    asciiTest('}');
    asciiTest('~');
    // delete character?
  }

  TEST_CASE("escape sequence") {
    auto escapeTest = [](char c, char expect) {
      const std::string charStr = std::string("\'\\") + c + "\'";
      auto result =
          CharLiteral::create(sy::StringSlice(charStr.c_str(), charStr.size()),
                              0, static_cast<uint32_t>(charStr.size()));

      CHECK(std::holds_alternative<CharLiteral>(result));
      CharLiteral parsedChar = std::get<CharLiteral>(result);
      CHECK_EQ(parsedChar.val, sy::Char(expect));
    };

    escapeTest('a', '\a');
    escapeTest('b', '\b');
    escapeTest('f', '\f');
    escapeTest('n', '\n');
    escapeTest('r', '\r');
    escapeTest('t', '\t');
    escapeTest('v', '\v');
    escapeTest('\\', '\\');
    escapeTest('\'', '\'');
    escapeTest('\"', '\"');
    escapeTest('?', '\?');
  }
}

TEST_SUITE("string literal") {
  TEST_CASE("ascii printable characters non-escape") {
    auto asciiTest = [](const char *str) {
      const std::string testStr = std::string("\"") + str + '\"';
      auto result = StringLiteral::create(
          sy::StringSlice(testStr.c_str(), testStr.size()), 0,
          static_cast<uint32_t>(testStr.size()), sy::Allocator());

      CHECK(std::holds_alternative<StringLiteral>(result));
      StringLiteral &parsedStr = std::get<StringLiteral>(result);
      CHECK_EQ(std::strcmp(parsedStr.str.cstr(), str), 0);
      CHECK_EQ(parsedStr.str.len(), std::strlen(str));
    };

    asciiTest("a");
    asciiTest("hello world!");
    asciiTest("this string goes past the SSO length");
    asciiTest("string goes past a multiple of the SSO length so we know it can "
              "store whatever literal is in compiled source code");
  }

  TEST_CASE("escape sequence") {
    auto escapeTest = [](const char *str, char escapeChar, char expect) {
      { // at the beginning
        const std::string expectStr = std::string("") + expect + str;
        const std::string testStr =
            std::string("\"") + '\\' + escapeChar + str + "\"";
        auto result = StringLiteral::create(
            sy::StringSlice(testStr.c_str(), testStr.size()), 0,
            static_cast<uint32_t>(testStr.size()), sy::Allocator());

        CHECK(std::holds_alternative<StringLiteral>(result));
        StringLiteral &parsedStr = std::get<StringLiteral>(result);
        CHECK_EQ(std::strcmp(parsedStr.str.cstr(), expectStr.c_str()), 0);
        CHECK_EQ(parsedStr.str.len(), expectStr.size());
      }
      { // at the end
        const std::string expectStr = std::string(str) + expect;
        const std::string testStr =
            std::string("\"") + str + '\\' + escapeChar + "\"";
        auto result = StringLiteral::create(
            sy::StringSlice(testStr.c_str(), testStr.size()), 0,
            static_cast<uint32_t>(testStr.size()), sy::Allocator());

        CHECK(std::holds_alternative<StringLiteral>(result));
        StringLiteral &parsedStr = std::get<StringLiteral>(result);
        CHECK_EQ(std::strcmp(parsedStr.str.cstr(), expectStr.c_str()), 0);
        CHECK_EQ(parsedStr.str.len(), expectStr.size());
      }
      { // in the middle
        const std::string expectStr = std::string(str) + expect + str;
        const std::string testStr =
            std::string("\"") + str + '\\' + escapeChar + str + "\"";
        auto result = StringLiteral::create(
            sy::StringSlice(testStr.c_str(), testStr.size()), 0,
            static_cast<uint32_t>(testStr.size()), sy::Allocator());

        CHECK(std::holds_alternative<StringLiteral>(result));
        StringLiteral &parsedStr = std::get<StringLiteral>(result);
        CHECK_EQ(std::strcmp(parsedStr.str.cstr(), expectStr.c_str()), 0);
        CHECK_EQ(parsedStr.str.len(), expectStr.size());
      }
    };

    escapeTest("hi", 'a', '\a');
    escapeTest("pwuifhpaiuwhgpaiwuhfpaiuwhdpiuhaw", 'a', '\a');
    escapeTest("hi", 'b', '\b');
    escapeTest("pwuifhpaiuwhgpaiwuhfpaiuwhdpiuhaw", 'b', '\b');
    escapeTest("hi", 'f', '\f');
    escapeTest("pwuifhpaiuwhgpaiwuhfpaiuwhdpiuhaw", 'f', '\f');
    escapeTest("hi", 'n', '\n');
    escapeTest("pwuifhpaiuwhgpaiwuhfpaiuwhdpiuhaw", 'n', '\n');
    escapeTest("hi", 'r', '\r');
    escapeTest("pwuifhpaiuwhgpaiwuhfpaiuwhdpiuhaw", 'r', '\r');
    escapeTest("hi", 't', '\t');
    escapeTest("pwuifhpaiuwhgpaiwuhfpaiuwhdpiuhaw", 't', '\t');
    escapeTest("hi", 'v', '\v');
    escapeTest("pwuifhpaiuwhgpaiwuhfpaiuwhdpiuhaw", 'v', '\v');
    escapeTest("hi", '\\', '\\');
    escapeTest("pwuifhpaiuwhgpaiwuhfpaiuwhdpiuhaw", '\\', '\\');
    escapeTest("hi", '\'', '\'');
    escapeTest("pwuifhpaiuwhgpaiwuhfpaiuwhdpiuhaw", '\'', '\'');
    escapeTest("hi", '\"', '\"');
    escapeTest("pwuifhpaiuwhgpaiwuhfpaiuwhdpiuhaw", '\"', '\"');
    escapeTest("hi", '?', '\?');
    escapeTest("pwuifhpaiuwhgpaiwuhfpaiuwhdpiuhaw", '?', '\?');
  }
}

#endif // SYNC_LIB_NO_TESTS
