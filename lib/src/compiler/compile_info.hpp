//! API
#pragma once
#ifndef SY_COMPILER_COMPILE_INFO_HPP_
#define SY_COMPILER_COMPILE_INFO_HPP_

#include "../core.h"
#include "../types/string/string_slice.hpp"

namespace sy {
    class SourceLocation {
    public:
        uint32_t line;
        uint32_t column;
    };

    namespace detail {
        SourceLocation sourceLocationFromFileLocation(
            const sy::StringSlice source, const uint32_t location);
    }
    

    class SY_API CompileError {
    public:
        enum class Kind : int {
            None = 0,
            OutOfMemory,
            FileTooBig,
        };

        struct FileTooBig {
            uint32_t fileSize;
            uint32_t maxFileSize;
        };

        CompileError() = default;

        static CompileError createOutOfMemory();

        static CompileError createFileTooBig(FileTooBig inFileTooBig);

        [[nodiscard]] Kind kind() const { return this->kind_; }

    private:
        
        union SY_API ErrorUnion {
            bool        _unused;
            FileTooBig  fileTooBig;

            ErrorUnion() : _unused(false) {}
        };

        Kind        kind_ = Kind::None;
        ErrorUnion  err_{};
    };
}

#endif // SY_COMPILER_COMPILE_INFO_HPP_