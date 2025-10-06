#ifndef SY_COMPILER_PARSER_PARSER_HPP_
#define SY_COMPILER_PARSER_PARSER_HPP_

#include "../../mem/allocator.hpp"
#include "../../program/program_error.hpp"
#include "../../types/array/dynamic_array.hpp"
#include "../../types/hash/map.hpp"
#include "../../types/option/option.hpp"
#include "../../types/result/result.hpp"
#include "../../types/sync_obj/sync_obj.hpp"
#include "../graph/scope.hpp"
#include "../source_tree/source_tree.hpp"
#include "../tokenizer/tokenizer.hpp"
#include "stack_variables.hpp"

namespace sy {
struct ParseInfo {
    TokenIter tokenIter;
    Allocator alloc;
    /// Will always be of type `SourceFileKind::SyncSourceFile`
    const volatile SourceTreeNode* fileSource;
    MapUnmanaged<StringSlice, bool> imports;
};

class IFunctionDefinition;
class ITypeDefNode;
class IFunctionStatement;

struct FileAst {
    Allocator alloc_;
    DynArrayUnmanaged<IFunctionDefinition*> functions_;
    DynArrayUnmanaged<ITypeDefNode*> structs_;
    Scope scope_;
    // TODO imports
};

Result<Option<IFunctionStatement*>, ProgramError>
parseStatement(ParseInfo* parseInfo, DynArray<StackVariable>* localVariables, Scope* currentScope);

} // namespace sy

#endif // SY_COMPILER_PARSER_PARSER_HPP_