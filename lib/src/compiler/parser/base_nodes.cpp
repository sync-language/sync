#include "base_nodes.hpp"

using sy::Allocator;
using sy::Result;
using sy::String;
using sy::detail::IBaseParserNode;

String IBaseParserNode::toString() const { return String(this->alloc_); }

// force 8 byte alignment
// static constexpr size_t PARSER_NODE_ALIGN = alignof(uint64_t);

// void* sy::detail::IBaseParserNode::operator new(size_t size, Allocator inAlloc) noexcept {
//     auto res = inAlloc.allocAlignedArray<uint8_t>(size, PARSER_NODE_ALIGN);
//     if (!res) {
//         return nullptr;
//     }
//     return res.value();
// }

// void sy::detail::IBaseParserNode::operator delete(void* self, size_t size) {
//     IBaseParserNode* actualSelf = reinterpret_cast<IBaseParserNode*>(self);
//     Allocator allocator = actualSelf->alloc();
//     allocator.freeAlignedArray(reinterpret_cast<uint8_t*>(self), size, PARSER_NODE_ALIGN);
// }

// #if SYNC_LIB_WITH_TESTS

// #include "../../doctest.h"

// namespace {
// class TestParserNodeThing : public IBaseParserNode {
//   public:
//     TestParserNodeThing(Allocator inAlloc) : IBaseParserNode(inAlloc) {}
//     virtual Result<void, int> init(sy::ParseInfo*) override { return {}; }
// };
// } // namespace

// TEST_CASE("IBaseParserNode new and delete") {
//     Allocator alloc;
//     TestParserNodeThing* ouh = new (alloc) TestParserNodeThing(alloc);
//     delete ouh;
// }

// #endif // SYNC_LIB_WITH_TESTS
