#include "tokenizer.hpp"
#include "../../util/assert.hpp"
#include "../../threading/alloc_cache_align.hpp"

using namespace sy;

static_assert(sizeof(Token) == sizeof(uint32_t));

AllocExpect<Tokenizer> Tokenizer::create(Allocator allocator, StringSlice source)
{
    Tokenizer self(allocator);

    sy_assert(source.len() <= MAX_SOURCE_LEN, "Source file input too big");

    // The total amount of tokens will always be less than or equal 
    // to the number of bytes in the source
    // As a result, we can over-allocate upfront.
    self.capacity_ = source.len();
    auto res = self.alloc_.allocAlignedArray<Token>(self.capacity_, ALLOC_CACHE_ALIGN);
    if(res.hasValue() == false) {
        return AllocExpect<Tokenizer>();
    }
    self.tokens_ = res.value();
}
