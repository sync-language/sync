#ifndef _SY_TYPES_TASK_TASK_HPP_
#define _SY_TYPES_TASK_TASK_HPP_

#include "../../core/core.h"
#include "../../mem/allocator.hpp"
#include "../../program/program_error.hpp"
#include "../result/result.hpp"

namespace sy {

class RawFunction;
class Type;

class SY_API RawTask {
  public:
    RawTask(RawTask&& other) noexcept;

    RawTask& operator=(RawTask&& other) = delete;

    ~RawTask() noexcept;

    RawTask() = delete;
    RawTask(const RawTask&) = delete;
    RawTask& operator=(const RawTask&) = delete;

    const Type* retType() const noexcept;

    Result<bool, ProgramError> isDone() const noexcept;

    Result<void, ProgramError> awaitDone(void* outReturn) noexcept;

    Result<bool, ProgramError> getIfDone(void* outReturn) noexcept;

  private:
    void* inner_;
};
} // namespace sy

#endif // _SY_TYPES_TASK_TASK_HPP_