#include "base_nodes.hpp"
#include "ast/return.hpp"
#include "parser.hpp"

using namespace sy;

String sy::detail::IBaseParserNode::toString() const { return String(this->alloc_); }

// force 8 byte alignment
static constexpr size_t PARSER_NODE_ALIGN = alignof(uint64_t);

void* sy::detail::IBaseParserNode::operator new(size_t size, Allocator inAlloc) noexcept {
    auto res = inAlloc.allocAlignedArray<uint8_t>(size, PARSER_NODE_ALIGN);
    if (!res) {
        return nullptr;
    }
    IBaseParserNode* actualSelf = reinterpret_cast<IBaseParserNode*>(res.value());
    actualSelf->size_ = size;
    actualSelf->alloc_ = inAlloc;
    return reinterpret_cast<void*>(actualSelf);
}

void sy::detail::IBaseParserNode::operator delete(void* self) noexcept {
    if (self == nullptr)
        return;
    IBaseParserNode* actualSelf = reinterpret_cast<IBaseParserNode*>(self);
    actualSelf->alloc_.freeAlignedArray(reinterpret_cast<uint8_t*>(self), actualSelf->size_, PARSER_NODE_ALIGN);
}

void sy::detail::IBaseParserNode::operator delete(void* self, Allocator inAlloc) noexcept {
    if (self == nullptr)
        return;
    IBaseParserNode* actualSelf = reinterpret_cast<IBaseParserNode*>(self);
    inAlloc.freeAlignedArray(reinterpret_cast<uint8_t*>(self), actualSelf->size_, PARSER_NODE_ALIGN);
}

Result<Option<IFunctionStatement*>, ProgramError>
IFunctionStatement::parseStatement(ParseInfo* parseInfo, DynArray<StackVariable>* variables, Scope* currentScope) {
    const Token token = parseInfo->tokenIter.current();
    switch (token.tag()) {
    case TokenType::RightBraceSymbol: {
        return std::nullopt;
    } break;
    case TokenType::ReturnKeyword: {
        ReturnNode* ret = new (parseInfo->alloc) ReturnNode(parseInfo->alloc);
        auto res = ret->init(parseInfo, variables, currentScope);
        if (res.hasErr()) {
            delete ret;
            return Error(res.takeErr());
        }
        return ret;
    } break;
    default:
        return Error(ProgramError(SourceFileLocation(parseInfo->tokenIter.source(), token.location()),
                                  ProgramError::Kind::CompileStatement));
    }
}

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"

namespace {
class TestParserNodeThing : public detail::IBaseParserNode {
  public:
    TestParserNodeThing(Allocator inAlloc) : IBaseParserNode(inAlloc) {}
};
} // namespace

TEST_CASE("IBaseParserNode new and delete") {
    Allocator alloc;
    TestParserNodeThing* ouh = new (alloc) TestParserNodeThing(alloc);
    delete ouh;
}

#endif // SYNC_LIB_WITH_TESTS
