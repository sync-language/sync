#include "program_error.hpp"
#include "../util/assert.hpp"
#include <iostream>

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

namespace sy {
ProgramErrorReporter defaultErrReporter = [](ProgramError errKind, const SourceFileLocation& where, StringSlice msg,
                                             void* arg) {
    (void)arg;

    try {
        std::cerr << "Sync Program Error:\n";
        std::cerr << static_cast<int>(errKind) << " Module: ";
        if (where.moduleName.len() == 0) {
            std::cerr << "?";
        } else {
            std::cerr.write(where.moduleName.data(), where.moduleName.len());
        }

        if (where.fileName.len() == 0) {
            std::cerr << std::endl;
        } else {
            std::cerr << " ";
            std::cerr.write(where.fileName.data(), where.fileName.len())
                << ':' << where.location.line << ':' << where.location.column;
        }

        if (msg.len() != 0) {
            std::cerr.write(msg.data(), msg.len()) << std::endl;
        }
    } catch (const std::ios_base::failure& e) {
        (void)e;
    }
};
}

// ProgramError::ProgramError(Option<SourceFileLocation> inWhere, Kind inKind) noexcept : where_(inWhere), kind_(inKind)
// {}
