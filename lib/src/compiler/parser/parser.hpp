#ifndef SY_COMPILER_PARSER_PARSER_HPP_
#define SY_COMPILER_PARSER_PARSER_HPP_

#include "../../mem/allocator.hpp"
#include "../../program/program_error.hpp"
#include "../../types/array/dynamic_array.hpp"
#include "../../types/hash/map.hpp"
#include "../../types/option/option.hpp"
#include "../../types/result/result.hpp"
#include "../../types/string/string_slice.hpp"
#include "../graph/scope.hpp"
#include "../tokenizer/tokenizer.hpp"
#include "stack_variables.hpp"

namespace sy {
struct SourceTreeNode;

struct ParseInfo {
    TokenIter tokenIter;
    Allocator alloc;
    /// Will always be of type `SourceFileKind::SyncSourceFile`
    const SourceTreeNode* fileSource;
    StringSlice moduleName;
    MapUnmanaged<StringSlice, bool> imports;
    ProgramErrorReporter errReporter;
    void* errReporterArg;

    ParseInfo(TokenIter inIter, Allocator inAlloc, const SourceTreeNode* inSource, StringSlice inModName,
              ProgramErrorReporter inErrReporter, void* inErrReporterArg) noexcept
        : tokenIter(inIter), alloc(inAlloc), fileSource(inSource), moduleName(inModName), imports({}),
          errReporter(inErrReporter), errReporterArg(inErrReporterArg) {}

    void reportErr(ProgramError errKind, uint32_t inBytePos, StringSlice msg) const noexcept;
};

class IFunctionDefinition;
class ITypeDefNode;
class IFunctionStatement;

struct FileAst {
    Allocator alloc;
    DynArrayUnmanaged<IFunctionDefinition*> nonGenericFunctions;
    DynArrayUnmanaged<ITypeDefNode*> nonGenericStructs;
    Scope scope;
    MapUnmanaged<StringSlice, bool> imports;

    FileAst() = default;

    FileAst(FileAst&&) = default;

    ~FileAst() noexcept;
};

Result<Option<IFunctionStatement*>, ProgramError>
parseStatement(ParseInfo* parseInfo, DynArray<StackVariable>* localVariables, Scope* currentScope) noexcept;

Result<FileAst, ProgramError> parseFile(Allocator alloc, const SourceTreeNode* fileSource,
                                        ProgramErrorReporter errReporter, void* errReporterArg) noexcept;

} // namespace sy

#endif // SY_COMPILER_PARSER_PARSER_HPP_