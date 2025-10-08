//! API
#ifndef SY_PROGRAM_PROGRAM_ERROR_HPP_
#define SY_PROGRAM_PROGRAM_ERROR_HPP_

#include "../core.h"
#include "../types/option/option.hpp"
#include "../types/string/string_slice.hpp"

namespace sy {
struct SourceLocation {
    uint32_t line = 1;
    uint32_t column = 1;

    SourceLocation() noexcept = default;
    SourceLocation(const StringSlice source, const uint32_t bytePos) noexcept;
};

struct SourceFileLocation {
    StringSlice fileName;
    StringSlice source;
    uint32_t bytePos;
    SourceLocation location;

    SourceFileLocation() noexcept : fileName({}), source({}), bytePos(0), location({}) {}

    SourceFileLocation(const StringSlice inSource, const uint32_t inBytePos) noexcept
        : fileName({}), source(inSource), bytePos(inBytePos), location(inSource, inBytePos) {}
};

/// @brief This class handles all fatal errors within Sync, for both runtime
/// and compile time. Runtime and compile time errors are treated the same due
/// to Sync supporting comptime code execution.
class ProgramError {
  public:
    enum class Kind : int {
        Unknown = 0,
        OutOfMemory = 1,
        CompileSourceFileTooBig,
        CompileNegativeToUnsignedIntConversion,
        CompileUnsignedOutsideIntRangeConversion,
        CompileFloatOutsideIntRangeConversion,
        CompileDecimalNumberLiteral,
        CompileCharNumberLiteral,
        CompileTooManyCharsInCharLiteral,
        CompileUnsupportedChar,
        CompileEscapeSequence,
        CompileFunctionSignature,
        CompileFunctionStatement,
        CompileExpression,
        CompileStatement,
        CompileSymbol,
    };

    ProgramError(Option<SourceFileLocation> inWhere, Kind inKind) noexcept;

    [[nodiscard]] Option<SourceFileLocation> where() const { return where_; }

    [[nodiscard]] Kind kind() const { return kind_; }

  private:
    Option<SourceFileLocation> where_;
    Kind kind_;
};
} // namespace sy

#endif // SY_PROGRAM_PROGRAM_ERROR_HPP_