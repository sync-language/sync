#include "program_error.hpp"
#include "../util/assert.hpp"

using namespace sy;

SourceLocation::SourceLocation(const StringSlice source, const uint32_t location) noexcept : SourceLocation() {
    for (uint32_t i = 0; i < location; i++) {
        if (source.data()[i] == '\n') {
            this->line += 1;
            this->column = 1;
        } else {
            this->column += 1;
        }
    }
}

ProgramError::ProgramError(Option<SourceFileLocation> inWhere, Kind inKind) noexcept : where_(inWhere), kind_(inKind) {}
