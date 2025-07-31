#include "compile_info.hpp"

using namespace sy;

CompileError sy::CompileError::createOutOfMemory()
{
    CompileError err;
    err.kind_ = Kind::OutOfMemory;
    return err;
}

CompileError sy::CompileError::createFileTooBig(FileTooBig inFileTooBig)
{
    CompileError err;
    err.kind_ = Kind::FileTooBig;
    err.err_.fileTooBig = inFileTooBig;
    return err;
}
