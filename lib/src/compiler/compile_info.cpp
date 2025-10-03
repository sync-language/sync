#include "compile_info.hpp"
#include "../util/assert.hpp"

using namespace sy;

CompileError sy::CompileError::createOutOfMemory() {
    CompileError err;
    err.kind_ = Kind::OutOfMemory;
    return err;
}

CompileError sy::CompileError::createFileTooBig(FileTooBig inFileTooBig) {
    CompileError err;
    err.kind_ = Kind::FileTooBig;
    err.err_.fileTooBig = inFileTooBig;
    return err;
}

CompileError sy::CompileError::createNegativeToUnsignedIntConversion() {
    CompileError err;
    err.kind_ = Kind::NegativeToUnsignedIntConversion;
    return err;
}

CompileError sy::CompileError::createUnsignedOutsideIntRangeConversion() {
    CompileError err;
    err.kind_ = Kind::UnsignedOutsideIntRangeConversion;
    return err;
}

CompileError sy::CompileError::createFloatOutsideIntRangeConversion() {
    CompileError err;
    err.kind_ = Kind::FloatOutsideIntRangeConversion;
    return err;
}

CompileError sy::CompileError::createInvalidDecimalNumberLiteral() {
    CompileError err;
    err.kind_ = Kind::InvalidDecimalNumberLiteral;
    return err;
}

CompileError sy::CompileError::createInvalidCharNumberLiteral() {
    CompileError err;
    err.kind_ = Kind::InvalidCharNumberLiteral;
    return err;
}

CompileError sy::CompileError::createTooManyCharsInCharLiteral() {
    CompileError err;
    err.kind_ = Kind::TooManyCharsInCharLiteral;
    return err;
}

CompileError sy::CompileError::createUnsupportedChar() {
    CompileError err;
    err.kind_ = Kind::UnsupportedChar;
    return err;
}

CompileError sy::CompileError::createInvalidEscapeSequence() {
    CompileError err;
    err.kind_ = Kind::InvalidEscapeSequence;
    return err;
}

CompileError sy::CompileError::createInvalidFunctionSignature(SourceLocation loc) {
    CompileError err;
    err.kind_ = Kind::InvalidFunctionSignature;
    err.location_ = loc;
    return err;
}

CompileError sy::CompileError::createInvalidFunctionStatement(SourceLocation loc) {
    CompileError err;
    err.kind_ = Kind::InvalidFunctionStatement;
    err.location_ = loc;
    return err;
}

CompileError sy::CompileError::createInvalidExpression(SourceLocation loc) {
    CompileError err;
    err.kind_ = Kind::InvalidExpression;
    err.location_ = loc;
    return err;
}

CompileError sy::CompileError::createInvalidStatement(SourceLocation loc) {
    CompileError err;
    err.kind_ = Kind::InvalidStatement;
    err.location_ = loc;
    return err;
}

CompileError::FileTooBig sy::CompileError::errFileTooBig() const {
    sy_assert(this->kind_ == Kind::FileTooBig, "Expected the compile error to be file too big");
    return this->err_.fileTooBig;
}

SourceLocation sy::detail::sourceLocationFromFileLocation(const sy::StringSlice source, const uint32_t location) {
    sy_assert(source.len() > location, "Index out of bounds");

    SourceLocation self{1, 1};

    for (uint32_t i = 0; i < location; i++) {
        if (source.data()[i] == '\n') {
            self.line += 1;
            self.column = 1;
        } else {
            self.column += 1;
        }
    }

    return self;
}
