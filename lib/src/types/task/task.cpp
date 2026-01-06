#include "task.hpp"
#include "../../core/core_internal.h"
#include "../../interpreter/stack/stack.hpp"
#include "../../util/assert.hpp"
#include "../function/function.hpp"
#include "../option/option.hpp"
#include "../type_info.hpp"
#include <cstring>

using namespace sy;

namespace sy {
struct TaskHeader {
    Allocator alloc_;
    SyAtomicBool isDone_;
    SyRawRwLock lock_;
    Option<ProgramError> encounteredErr_;
    const RawFunction* function_;
    Stack stack_;

    const Type* valType() const { return this->function_->returnType; }

    uintptr_t valueMemLocation() const {
        const size_t memOffset = sizeof(TaskHeader) + paddingForType<TaskHeader>(this->valType()->alignType);
        const uint8_t* asBytes = reinterpret_cast<const uint8_t*>(this);
        return reinterpret_cast<uintptr_t>(&asBytes[memOffset]);
    }

    const void* valueMem() const { return reinterpret_cast<const void*>(this->valueMemLocation()); }

    void* valueMemMut() { return reinterpret_cast<void*>(this->valueMemLocation()); }

    void destroy() {
        const size_t allocAlign =
            this->valType()->alignType < SYNC_CACHE_LINE_SIZE ? SYNC_CACHE_LINE_SIZE : this->valType()->alignType;
        const size_t fullAllocSize =
            sizeof(TaskHeader) + paddingForType<TaskHeader>(this->valType()->alignType) + this->valType()->sizeType;

        sy::Allocator alloc = this->alloc_;
        uint8_t* mem = reinterpret_cast<uint8_t*>(this);

        this->~TaskHeader();
        alloc.freeAlignedArray(mem, fullAllocSize, allocAlign);
    }
};

} // namespace sy

const TaskHeader* asHeader(const void* inner) { return reinterpret_cast<const TaskHeader*>(inner); }

TaskHeader* asHeaderMut(void* inner) { return reinterpret_cast<TaskHeader*>(inner); }

RawTask::RawTask(RawTask&& other) noexcept {
    this->inner_ = other.inner_;
    other.inner_ = nullptr;
}

sy::RawTask::~RawTask() noexcept {
    if (this->inner_ == nullptr)
        return;

    auto res = this->awaitDone(nullptr);
    if (res.hasErr()) {
        syncFatalErrorHandlerFn("Failed to handle Sync program error in Task");
    }

    this->inner_ = nullptr;
}

const Type* sy::RawTask::retType() const noexcept { return asHeader(this->inner_)->valType(); }

Result<bool, ProgramError> RawTask::isDone() const noexcept {
    return sy_atomic_bool_load(&asHeader(this->inner_)->isDone_, SY_MEMORY_ORDER_SEQ_CST);
}

Result<void, ProgramError> RawTask::awaitDone(void* outReturn) noexcept {
    while (!this->isDone()) {
        sy_thread_yield();
    }

    auto res = this->getIfDone(outReturn);
    if (res.hasErr()) {
        return Error(res.takeErr());
    }

    sy_assert(res.value(), "Should have succeeded");
    return {};
}

Result<bool, ProgramError> RawTask::getIfDone(void* outReturn) noexcept {
    if (!this->isDone()) {
        return false;
    }

    TaskHeader* header = asHeaderMut(this->inner_);

    sy_raw_rwlock_acquire_exclusive(&header->lock_);

    if (header->encounteredErr_.hasValue()) {
        ProgramError err = header->encounteredErr_.value();
        sy_raw_rwlock_release_exclusive(&header->lock_);
        header->destroy();
        this->inner_ = nullptr;
        return Error(err);
    }

    if (header->valType() != nullptr) {
        if (outReturn != nullptr) {
            memcpy(outReturn, header->valueMem(), header->valType()->sizeType);
        } else {
            header->valType()->destroyObject(header->valueMemMut());
        }
    }

    sy_raw_rwlock_release_exclusive(&header->lock_);
    header->destroy();
    this->inner_ = nullptr;
    return true;
}