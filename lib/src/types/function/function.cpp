#include "function.hpp"
#include "../type_info.hpp"
#include "../../util/assert.hpp"
#include "../../interpreter/stack.hpp"
#include "../../interpreter/interpreter.hpp"
#include "../../program/program_internal.hpp"
#include "../../mem/allocator.hpp"
#include <utility>
#include <cstring>
#include <new>

using sy::Function;
using sy::Type;
using sy::c::SyFunction;
using sy::c::SyFunctionCallArgs;
using sy::c::SyProgramRuntimeError;
using sy::c::SyType;

#if _MSC_VER
    static constexpr size_t ALLOC_ALIGNMENT = std::hardware_destructive_interference_size;
#else
    static constexpr size_t ALLOC_ALIGNMENT = 64;
#endif

/// Stores arguments for a C function call.
class alignas(64) ArgBuf {
public:
    ArgBuf() = default;

    ~ArgBuf();

    struct Arg {
        const void *mem;
        const Type *type;
    };

    void push(const Arg &arg);

    Arg at(size_t index) const;

    void take(void* outValue, size_t index);

    /// Clears out the arguments. Any that were taken are ignored. The rest are destroyed.
    void clear();

private:
    static constexpr size_t INVALID_OFFSET = -1;

    /// May return INVALID_OFFSET, in which case reallocation is required.
    size_t nextOffset(const size_t sizeType, const size_t alignType) const;

    void reallocate(const size_t sizeNewType);

private:
    uint8_t *values = nullptr;
    const Type **types = nullptr;
    size_t *offsets;
    size_t count = 0;
    size_t valuesCapacity = 0;
    size_t typesAndOffsetsCapacity = 0;

    // False sharing must be avoided, since arg buffers are intended to be threadlocal.
};

/// Stores multiple argument buffers. Useful for having many "active" C calls.
class alignas(64) ArgBufArray {
public:
    ArgBufArray() = default;

    ~ArgBufArray();

    ArgBuf& bufAt(uint32_t index);

    void pushNewBuf();
private:
    ArgBuf* bufs = nullptr;
    uint32_t len = 0;
    uint32_t capacity = 0;

    // Use uint32_t not for memory savings within this class (is 64 byte aligned anyways),
    // but for memory savings in SyCFunctionHandler
};

ArgBuf::~ArgBuf()
{
    sy::Allocator alloc;
    if (this->valuesCapacity > 0) {
        alloc.freeAlignedArray(this->values, this->valuesCapacity, ALLOC_ALIGNMENT);
    }
    if (this->typesAndOffsetsCapacity > 0) {
        alloc.freeAlignedArray(this->types, this->typesAndOffsetsCapacity, ALLOC_ALIGNMENT);
        alloc.freeAlignedArray(this->offsets, this->typesAndOffsetsCapacity, ALLOC_ALIGNMENT);
    }
    this->count = 0;
    this->valuesCapacity = 0;
    this->typesAndOffsetsCapacity = 0;
}

void ArgBuf::push(const Arg &arg)
{
    sy_assert(arg.type->sizeType > 0, "Cannot push zero sized arguments");
    sy_assert(arg.type->alignType > 0, "Cannot push zero aligned arguments");

    this->reallocate(arg.type->sizeType);
    size_t offset = this->nextOffset(arg.type->sizeType, arg.type->alignType);

    // Lambda because this is unlikely to execute. In real world scenarios, may never execute at any point.
    auto loopingReallocate = [&]() {
        size_t i = 1;
        while (offset == INVALID_OFFSET)
        {
            this->reallocate(arg.type->sizeType << i);
            offset = this->nextOffset(arg.type->sizeType, arg.type->alignType);
        }
    };

    if (offset == INVALID_OFFSET) {
        loopingReallocate();
    }

    memcpy(&this->values[offset], arg.mem, arg.type->sizeType);
    this->types[this->count] = arg.type;
    this->offsets[this->count] = offset;
    this->count += 1;
}

ArgBuf::Arg ArgBuf::at(size_t index) const
{
    sy_assert(index < this->count, "Index out of bounds");
    Arg a{};
    a.mem = reinterpret_cast<const void *>(&this->values[this->offsets[index]]);
    a.type = this->types[index];
    return a;
}

void ArgBuf::take(void *outValue, size_t index)
{
    sy_assert(outValue != nullptr, "Cannot store argument to null memory");
    sy_assert(index < this->count, "Index out of bounds taking C function argument");

    const Type* type = this->types[index];
    sy_assert(type != nullptr, "Cannot take argument twice");

    const size_t offset = this->offsets[index];
    const void* mem = &this->values[offset];
    sy_assert((reinterpret_cast<uintptr_t>(mem) % type->alignType) == 0, "Misaligned function argument");
    memcpy(outValue, mem, type->sizeType);
    this->types[index] = nullptr; // Set argument as taken
}

void ArgBuf::clear() 
{
    // todo destruct non-taken arguments
}

size_t ArgBuf::nextOffset(const size_t sizeType, const size_t alignType) const
{
    sy_assert(this->values != nullptr, "Cannot get the next offset from an invalid memory address");

    const size_t valuesAddr = reinterpret_cast<size_t>(this->values);
    const size_t requiredShift = alignType - (valuesAddr % alignType);

    if (this->count == 0) {
        if ((requiredShift + sizeType) > this->valuesCapacity) {
            return INVALID_OFFSET;
        }

        return requiredShift;
    }
    else {
        if ((this->offsets[this->count] + requiredShift + sizeType) > this->valuesCapacity) {
            return INVALID_OFFSET;
        }

        return this->offsets[this->count] + requiredShift;
    }
}

void ArgBuf::reallocate(const size_t sizeNewType)
{
    size_t newValuesCapacity = 0;
    size_t newTypesAndOffsetCapacity = 0;

    if (this->count == 0) {
        // Overallocate. Guaranteed to get a valid offset.
        newValuesCapacity = sizeNewType * 2;
        if (newValuesCapacity < ALLOC_ALIGNMENT) {
            newValuesCapacity = ALLOC_ALIGNMENT;
        }
        newTypesAndOffsetCapacity = 2;
    }
    else {
        newValuesCapacity = (this->valuesCapacity * 2) + (sizeNewType * 2);
        newTypesAndOffsetCapacity = this->typesAndOffsetsCapacity * 2;
    }

    sy::Allocator alloc;
    uint8_t *newValues = alloc.allocAlignedArray<uint8_t>(newValuesCapacity, ALLOC_ALIGNMENT).get();
    const Type **newTypes = alloc.allocAlignedArray<const Type *>(newTypesAndOffsetCapacity, ALLOC_ALIGNMENT).get();
    size_t *newOffsets = alloc.allocAlignedArray<size_t>(newTypesAndOffsetCapacity, ALLOC_ALIGNMENT).get();

    if (this->count > 0) {
        memcpy(newValues, this->values, this->valuesCapacity);
        for (size_t i = 0; i < this->count; i++)
        {
            newTypes[i] = this->types[i];
            newOffsets[i] = this->offsets[i];
        }
        alloc.freeAlignedArray(this->values, this->valuesCapacity, ALLOC_ALIGNMENT);
        alloc.freeAlignedArray(this->types, this->typesAndOffsetsCapacity, ALLOC_ALIGNMENT);
        alloc.freeAlignedArray(this->offsets, this->typesAndOffsetsCapacity, ALLOC_ALIGNMENT);
    }

    this->values = newValues;
    this->types = newTypes;
    this->offsets = newOffsets;
    this->valuesCapacity = newValuesCapacity;
    this->typesAndOffsetsCapacity = newTypesAndOffsetCapacity;
}

ArgBufArray::~ArgBufArray()
{
    if(this->bufs == nullptr) return;

    for(uint32_t i = 0; i < this->len; i++) {
        ArgBuf& buf = this->bufs[i];
        buf.~ArgBuf();
    }

    sy::Allocator alloc;
    alloc.freeArray(this->bufs, this->capacity);
}

ArgBuf &ArgBufArray::bufAt(uint32_t index)
{
    sy_assert(index < this->len, "Index out of bounds");
    return this->bufs[index];
}

void ArgBufArray::pushNewBuf()
{
    if(this->len == this->capacity) {
        sy::Allocator alloc;

        // 4 seems like reasonable starting capacity
        const uint32_t newCapacity = this->capacity == 0 ? 4 : this->capacity * 2;
        ArgBuf* newBufs = alloc.allocArray<ArgBuf>(newCapacity).get();

        if(this->capacity == 0) {
            for(uint32_t i = 0; i < this->len; i++) {
                newBufs[i] = std::move(this->bufs[i]);
            }
            alloc.freeArray(this->bufs, this->capacity);
        }
        
        this->capacity = newCapacity;
    }

    ArgBuf* placed = new (&this->bufs[this->len]) ArgBuf;
    (void)placed;
    this->len += 1;
}

// static thread_local ArgBuf ffiArgBuf;

extern "C"
{
    SY_API SyFunctionCallArgs sy_function_start_call(const SyFunction *self)
    {
        SyFunctionCallArgs args = {0, 0, 0};
        args.func = self;
        return args;
    }

    SY_API bool sy_function_call_args_push(SyFunctionCallArgs *self, const void *argMem, const SyType *typeInfo)
    {
        const Function *function = reinterpret_cast<const Function *>(self->func);
        sy_assert(self->pushedCount < function->argsLen, "Cannot push more arguments than the function takes");
        sy_assert(function->tag == Function::CallType::Script, "Can only do script function calling currently");

        const bool result = Stack::getActiveStack().pushScriptFunctionArg(
            argMem, reinterpret_cast<const Type *>(typeInfo), self->_offset);

        if (result == false) {
            return false;
        }

        const size_t roundedToMultipleOf8 = typeInfo->sizeType % 8 == 0 ? typeInfo->sizeType : typeInfo->sizeType + (8 - (typeInfo->sizeType % 8));
        const size_t slotsOccupied = roundedToMultipleOf8 / 8;
        sy_assert(slotsOccupied <= UINT16_MAX, "Cannot push argument. Too big");

        const size_t newOffset = self->_offset + slotsOccupied;
        sy_assert(newOffset <= UINT16_MAX, "Cannot push argument. Would overflow script stack");
        const sy::InterpreterFunctionScriptInfo *scriptInfo =
            reinterpret_cast<const sy::InterpreterFunctionScriptInfo *>(function->fptr);
        sy_assert(newOffset <= scriptInfo->stackSpaceRequired, "Pushing argument would overflow this function's script stack");

        self->_offset = static_cast<uint16_t>(slotsOccupied);

        return true;
    }

    union ErrContain {
        sy::ProgramRuntimeError cppErr;
        SyProgramRuntimeError cErr;

        ErrContain() : cppErr() {}
    };

    SY_API SyProgramRuntimeError sy_function_call(SyFunctionCallArgs self, void *retDst)
    {
        Function::CallArgs callArgs = {self};
        ErrContain err;
        err.cppErr = callArgs.call(retDst);
        return err.cErr;
    }
}

bool sy::Function::CallArgs::push(const void *argMem, const Type *typeInfo)
{
    return c::sy_function_call_args_push(&this->info, argMem, reinterpret_cast<const SyType *>(typeInfo));
}

sy::ProgramRuntimeError sy::Function::CallArgs::call(void *retDst)
{
    const Function *function = reinterpret_cast<const Function *>(this->info.func);
    sy_assert(this->info.pushedCount == function->argsLen, "Did not push enough arguments for function");
    sy_assert(function->tag == Function::CallType::Script, "Can only do script function calling currently");

    return interpreterExecuteFunction(function, retDst);
}

sy::Function::CallArgs sy::Function::startCall() const
{
    CallArgs callArgs = {{0, 0, 0}};
    callArgs.info.func = reinterpret_cast<const SyFunction *>(this);
    return callArgs;
}


