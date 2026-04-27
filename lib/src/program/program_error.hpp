//! API
#ifndef SY_PROGRAM_PROGRAM_ERROR_HPP_
#define SY_PROGRAM_PROGRAM_ERROR_HPP_

#include "../core/core.h"
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
    StringSlice moduleName{};
    StringSlice fileName{};
    StringSlice source{};
    uint32_t bytePos = 0;
    SourceLocation location{};

    SourceFileLocation() = default;

    SourceFileLocation(const StringSlice inSource, const uint32_t inBytePos) noexcept
        : source(inSource), bytePos(inBytePos), location(inSource, inBytePos) {}
};

enum class ProgramError : int {
    Unknown = 1,
    OutOfMemory = 2,
    CompileSourceFileTooBig = 3,
    CompileNegativeToUnsignedIntConversion = 4,
    CompileUnsignedOutsideIntRangeConversion = 5,
    CompileFloatOutsideIntRangeConversion = 6,
    CompileDecimalNumberLiteral = 7,
    CompileCharNumberLiteral = 8,
    CompileTooManyCharsInCharLiteral = 9,
    CompileUnsupportedChar = 10,
    CompileEscapeSequence = 11,
    CompileFunctionSignature = 12,
    CompileFunctionStatement = 13,
    CompileExpression = 14,
    CompileStatement = 15,
    CompileSymbol = 16,
    CompileCircularModuleDependency = 17,
    CompileModuleDependencyGraph = 18,
    CompileSelfImport = 19,
    CompileUnknownType = 20,
    BufferTooSmall = 21,
};

using ProgramErrorReporter = void (*)(ProgramError errKind, const SourceFileLocation& where,
                                      StringSlice msg, void* arg);

/// @brief This class handles all fatal errors within Sync, for both runtime
/// and compile time. Runtime and compile time errors are treated the same due
/// to Sync supporting comptime code execution.
// class ProgramError {
//   public:
//     enum class Kind : int {
//         Unknown = 0,
//         OutOfMemory = 1,
//         CompileSourceFileTooBig,
//         CompileNegativeToUnsignedIntConversion,
//         CompileUnsignedOutsideIntRangeConversion,
//         CompileFloatOutsideIntRangeConversion,
//         CompileDecimalNumberLiteral,
//         CompileCharNumberLiteral,
//         CompileTooManyCharsInCharLiteral,
//         CompileUnsupportedChar,
//         CompileEscapeSequence,
//         CompileFunctionSignature,
//         CompileFunctionStatement,
//         CompileExpression,
//         CompileStatement,
//         CompileSymbol,
//         CompileCircularModuleDependency,
//         CompileModuleDependencyGraph,
//     };

//     ProgramError(Option<SourceFileLocation> inWhere, Kind inKind) noexcept;

//     [[nodiscard]] Option<SourceFileLocation> where() const { return where_; }

//     [[nodiscard]] Kind kind() const { return kind_; }

//   private:
//     Option<SourceFileLocation> where_;
//     Kind kind_;
// };
} // namespace sy

#endif // SY_PROGRAM_PROGRAM_ERROR_HPP_
