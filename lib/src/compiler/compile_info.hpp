//! API
#pragma once
#ifndef SY_COMPILER_COMPILE_INFO_HPP_
#define SY_COMPILER_COMPILE_INFO_HPP_

#include "../core.h"
#include "../types/string/string_slice.hpp"

namespace sy {
struct SourceLocation {
    uint32_t line;
    uint32_t column;
};

namespace detail {
SourceLocation sourceLocationFromFileLocation(const sy::StringSlice source, const uint32_t location);
}

class SY_API CompileError {
  public:
    enum class Kind : int {
        None = 0,
        OutOfMemory,
        FileTooBig,
        NegativeToUnsignedIntConversion,
        UnsignedOutsideIntRangeConversion,
        FloatOutsideIntRangeConversion,
        InvalidDecimalNumberLiteral,
        InvalidCharNumberLiteral,
        TooManyCharsInCharLiteral,
        UnsupportedChar,
        InvalidEscapeSequence,
        InvalidFunctionSignature,
        InvalidFunctionStatement,
    };

    struct FileTooBig {
        size_t fileSize;
        size_t maxFileSize;
    };

    static CompileError createOutOfMemory();

    static CompileError createFileTooBig(FileTooBig inFileTooBig);

    static CompileError createNegativeToUnsignedIntConversion();

    static CompileError createUnsignedOutsideIntRangeConversion();

    static CompileError createFloatOutsideIntRangeConversion();

    static CompileError createInvalidDecimalNumberLiteral();

    static CompileError createInvalidCharNumberLiteral();

    static CompileError createTooManyCharsInCharLiteral();

    static CompileError createUnsupportedChar();

    static CompileError createInvalidEscapeSequence();

    static CompileError createInvalidFunctionSignature(SourceLocation loc);

    static CompileError createInvalidFunctionStatement(SourceLocation loc);

    [[nodiscard]] Kind kind() const { return this->kind_; }

    [[nodiscard]] FileTooBig errFileTooBig() const;

    [[nodiscard]] SourceLocation location() const;

  private:
    CompileError() = default;

    union SY_API ErrorUnion {
        bool _unused;
        FileTooBig fileTooBig;

        ErrorUnion() : _unused(false) {}
    };

    Kind kind_ = Kind::None;
    ErrorUnion err_{};
    SourceLocation location_;
};
} // namespace sy

#endif // SY_COMPILER_COMPILE_INFO_HPP_