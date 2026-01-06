#include "iterator.hpp"
#include "../../types/function/function.hpp"
#include "../../types/type_info.hpp"
#include "../../util/assert.hpp"

using namespace sy;

Result<void, ProgramError> sy::IteratorObj::next(void* outOptional) noexcept {
    sy_assert(outOptional != nullptr, "Store iterator next value in null");

    RawFunction::CallArgs callArgs = this->traitImpl->next->startCall();
    (void)callArgs.push(this->obj, this->traitImpl->self);
    return callArgs.call(outOptional);
}
