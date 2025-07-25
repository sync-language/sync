#include "function.h"
#include "function.hpp"
#include "../type_info.h"
#include "../type_info.hpp"
#include "../../util/assert.hpp"
#include "../../interpreter/stack/stack.hpp"
#include "../../interpreter/interpreter.hpp"
#include "../../program/program.h"
#include "../../program/program_internal.hpp"
#include "../../mem/allocator.hpp"
#include "../../util/unreachable.hpp"
#include "../../threading/alloc_cache_align.hpp"
#include <utility>
#include <cstring>
#include <new>

using sy::Function;
using sy::Type;

static_assert(static_cast<int>(Function::CallType::C) == static_cast<int>(SyFunctionTypeC));
static_assert(static_cast<int>(Function::CallType::C) == static_cast<int>(SyFunctionTypeC));
static_assert(sizeof(Function::CallType) == sizeof(int));
static_assert(sizeof(SyFunctionType) == sizeof(int));

static_assert(sizeof(Function::CallArgs) == sizeof(SyFunctionCallArgs));
static_assert(alignof(Function::CallArgs) == alignof(SyFunctionCallArgs));
static_assert(offsetof(Function::CallArgs, func) == offsetof(SyFunctionCallArgs, func));
static_assert(offsetof(Function::CallArgs, pushedCount) == offsetof(SyFunctionCallArgs, pushedCount));
static_assert(offsetof(Function::CallArgs, _offset) == offsetof(SyFunctionCallArgs, _offset));

#if _MSC_VER
    static constexpr size_t ALLOC_ALIGNMENT = std::hardware_destructive_interference_size;
#else
    static constexpr size_t ALLOC_ALIGNMENT = 64;
#endif

#if defined(_MSC_VER)
// Supress warning for struct padding due to alignment specifier
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4324?view=msvc-170
#pragma warning(push)
#pragma warning(disable: 4324)
#endif
/// Stores arguments for a C function call.
class alignas(ALLOC_CACHE_ALIGN) ArgBuf {
public:
    ArgBuf() = default;

    ~ArgBuf();

    struct Arg {
        void *mem;
        const Type *type;
    };

    void push(const Arg &arg);

    Arg at(size_t index) const;

    void take(void* outValue, size_t index);

    /// Clears out the arguments. Any that were taken are ignored. The rest are destroyed.
    void clear();

    /// Sets the destination of the function return value.
    void setReturnDestination(void* dst);

    void* getReturnDestination();

    void setReturnValue(const void* value, const size_t sizeOfType);

private:
    static constexpr size_t INVALID_OFFSET = static_cast<size_t>(-1);

    /// May return INVALID_OFFSET, in which case reallocation is required.
    size_t nextOffset(const size_t sizeType, const size_t alignType) const;

    void reallocate(const size_t sizeNewType);

private:
    uint8_t *values = nullptr;
    const Type **types = nullptr;
    size_t *offsets = nullptr;
    size_t count = 0;
    size_t valuesCapacity = 0;
    size_t typesAndOffsetsCapacity = 0;
    void* retDst = nullptr;

    // False sharing must be avoided, since arg buffers are intended to be threadlocal.
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(_MSC_VER)
// Supress warning for struct padding due to alignment specifier
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4324?view=msvc-170
#pragma warning(push)
#pragma warning(disable: 4324)
#endif
/// Stores multiple argument buffers. Useful for having many "active" C calls.
class ArgBufArray {
public:
    ArgBufArray() = default;

    ~ArgBufArray();

    ArgBuf& bufAt(uint32_t index);

    uint32_t pushNewBuf();

    void popBuf() {
        sy_assert(len > 0, "Cannot pop arg buffer");
        len -= 1; 
    }
private:
    ArgBuf* bufs = nullptr;
    uint32_t len = 0;
    uint32_t capacity = 0;

    // Use uint32_t not for memory savings within this class (is 64 byte aligned anyways),
    // but for memory savings in SyCFunctionHandler
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

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
    sy_assert(index < this->count, "C function argument Index out of bounds");
    Arg a{};
    a.mem = const_cast<void*>(reinterpret_cast<const void *>(&this->values[this->offsets[index]]));
    a.type = this->types[index];
    return a;
}

void ArgBuf::take(void *outValue, size_t index)
{
    sy_assert(outValue != nullptr, "Cannot store argument to null memory");
    sy_assert(index < this->count, "C function argument index out of bounds");

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

void ArgBuf::setReturnDestination(void *dst)
{
    sy_assert(dst != nullptr, "Cannot set return destination to null");
    this->retDst = dst;
}

void* ArgBuf::getReturnDestination() {
    sy_assert(this->retDst != nullptr, "Cannot get invalid return destination");
    return this->retDst;
}

void ArgBuf::setReturnValue(const void *value, const size_t sizeOfType)
{
    sy_assert(this->retDst != nullptr, "Function either doesn't return or return value was already set");
    memcpy(this->retDst, value, sizeOfType);
    this->retDst = nullptr;
}

size_t ArgBuf::nextOffset(const size_t sizeType, const size_t alignType) const
{
    sy_assert(this->values != nullptr, "Cannot get the next offset from an invalid memory address");

    const size_t valuesAddr = reinterpret_cast<size_t>(this->values);
    const size_t remainder = (valuesAddr % alignType);
    if(this->count == 0 && remainder == 0) {
        return 0;
    }
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

uint32_t ArgBufArray::pushNewBuf()
{
    if(this->len == this->capacity) {
        sy::Allocator alloc;

        // 4 seems like reasonable starting capacity
        const uint32_t newCapacity = this->capacity == 0 ? 4 : this->capacity * 2;
        ArgBuf* newBufs = alloc.allocArray<ArgBuf>(newCapacity).get();

        if(this->capacity != 0) {
            for(uint32_t i = 0; i < this->len; i++) {
                newBufs[i] = std::move(this->bufs[i]);
            }
            alloc.freeArray(this->bufs, this->capacity);
        }
        
        this->capacity = newCapacity;
        this->bufs = newBufs;
    }

    const uint32_t handler = this->len;
    ArgBuf* placed = new (&this->bufs[handler]) ArgBuf;
    (void)placed;
    this->len += 1;
    return handler;
}

static thread_local ArgBufArray cArgBufs;

extern "C"
{
    SY_API SyFunctionCallArgs sy_function_start_call(const SyFunction *self)
    {
        SyFunctionCallArgs args = {0, 0, 0};
        args.func = self;
        if(self->tag == SyFunctionTypeC) {
            args._offset = static_cast<uint16_t>(cArgBufs.pushNewBuf());
        }
        return args;
    }

    SY_API bool sy_function_call_args_push(SyFunctionCallArgs *self, void *argMem, const SyType *typeInfo)
    {
        Function::CallArgs* asCpp = reinterpret_cast<Function::CallArgs*>(self);
        return asCpp->push(argMem, reinterpret_cast<const sy::Type*>(typeInfo));
    }

    union ErrContain {
        sy::ProgramRuntimeError cppErr;
        SyProgramRuntimeError cErr;

        ErrContain() : cppErr() {}
    };

    SY_API SyProgramRuntimeError sy_function_call(SyFunctionCallArgs self, void *retDst)
    {
        Function::CallArgs callArgs = *reinterpret_cast<Function::CallArgs*>(&self);
        ErrContain err;
        err.cppErr = callArgs.call(retDst);
        return err.cErr;
    }

    SY_API void sy_c_function_handler_take_arg(SyCFunctionHandler *self, void *outValue, size_t argIndex)
    {
        ArgBuf& buf = cArgBufs.bufAt(self->_handle);
        buf.take(outValue, argIndex);
    }

    SY_API void sy_c_function_handler_set_return_value(SyCFunctionHandler *self, const void *retValue, const SyType *type)
    {
        ArgBuf& buf = cArgBufs.bufAt(self->_handle);
        buf.setReturnValue(retValue, type->sizeType);
    }
} // extern "C"

bool sy::Function::CallArgs::push(void *argMem, const Type *typeInfo)
{
    sy_assert(typeInfo != nullptr, "Cannot push null typed argument");
    sy_assert(this->pushedCount < this->func->argsLen, "Cannot push more arguments than the function takes");

    if(this->func->tag == Function::CallType::Script) {
        const sy::InterpreterFunctionScriptInfo *scriptInfo =
            reinterpret_cast<const sy::InterpreterFunctionScriptInfo *>(this->func->fptr);
        const bool result = Stack::getActiveStack().pushScriptFunctionArg(
            argMem, reinterpret_cast<const Type *>(typeInfo), this->_offset, scriptInfo->stackSpaceRequired, func->alignment);

        if (result == false) {
            return false;
        }

        const size_t roundedToMultipleOf8 = typeInfo->sizeType % 8 == 0 ? typeInfo->sizeType : typeInfo->sizeType + (8 - (typeInfo->sizeType % 8));
        const size_t slotsOccupied = roundedToMultipleOf8 / 8;
        sy_assert(slotsOccupied <= UINT16_MAX, "Cannot push argument. Too big");

        const size_t newOffset = this->_offset + slotsOccupied;
        sy_assert(newOffset <= UINT16_MAX, "Cannot push argument. Would overflow script stack");
        
        sy_assert(newOffset <= scriptInfo->stackSpaceRequired, "Pushing argument would overflow this function's script stack");

        this->_offset = static_cast<uint16_t>(slotsOccupied);
    } else if(this->func->tag == Function::CallType::C) {
        const ArgBuf::Arg arg = {argMem, reinterpret_cast<const sy::Type*>(typeInfo)};
        cArgBufs.bufAt(this->_offset).push(arg);
    } else {
        sy_assert(false, "Unknown function call type");
    }
    this->pushedCount += 1;

    return true;
}

sy::ProgramRuntimeError sy::Function::CallArgs::call(void *retDst)
{
    sy_assert(this->pushedCount == this->func->argsLen, "Did not push enough arguments for function");

    if(this->func->tag == Function::CallType::Script) {
        return interpreterExecuteScriptFunction(this->func, retDst);
    }
    else if(this->func->tag == Function::CallType::C) {
        const uint32_t handlerIndex = this->_offset;
        Function::CHandler handler{handlerIndex};
        const auto cfunc = (c_function_t)(this->func->fptr);
        const ProgramRuntimeError err = cfunc(handler);
        cArgBufs.bufAt(handlerIndex).clear();
        cArgBufs.popBuf();
        return err;
    }
    else {
        unreachable();
    }
}

sy::Function::CallArgs sy::Function::startCall() const
{
    CallArgs callArgs = {0, 0, 0};
    callArgs.func = this;
    if(this->tag == Function::CallType::C) {
        callArgs._offset = static_cast<uint16_t>(cArgBufs.pushNewBuf());
    }
    return callArgs;
}

void* sy::Function::CHandler::getArgMem(size_t argIndex)
{
    ArgBuf& buf = cArgBufs.bufAt(this->handle);
    ArgBuf::Arg arg = buf.at(argIndex);
    return arg.mem;
}

const Type *sy::Function::CHandler::getArgType(size_t argIndex)
{
    ArgBuf& buf = cArgBufs.bufAt(this->handle);
    ArgBuf::Arg arg = buf.at(argIndex);
    return arg.type;
}

void sy::Function::CHandler::validateArgTypeMatches(void *arg, const Type *storedType, size_t sizeType, size_t alignType)
{
    sy_assert((reinterpret_cast<uintptr_t>(arg) % alignType) == 0, "Function argument misaligned");
    sy_assert(storedType->sizeType == sizeType, "Function argument size mismatch");
    sy_assert(storedType->alignType == alignType, "Function argument alignment mismatch");
}

void *sy::Function::CHandler::getRetDst()
{
    ArgBuf& buf = cArgBufs.bufAt(this->handle);
    return buf.getReturnDestination();
}

void sy::Function::CHandler::validateReturnDstAligned(void *retDst, size_t alignType)
{
    sy_assert((reinterpret_cast<uintptr_t>(retDst) % alignType) == 0, "Function return value destination misaligned");
}

#if SYNC_LIB_TEST

#include "../../doctest.h"

using sy::Function;
using sy::Type;
using sy::StringSlice;

TEST_CASE("push and get arg") {
    (void)cArgBufs.pushNewBuf();
    ArgBuf& buf = cArgBufs.bufAt(0);
    int32_t val = 45;
    const ArgBuf::Arg arg = {&val, Type::TYPE_I32};
    buf.push(arg);
    int32_t outVal = 99;
    buf.take(&outVal, 0);
    CHECK_EQ(outVal, 45);

    cArgBufs.popBuf();
}

template<typename T, T expected>
sy::ProgramRuntimeError simpleFunc1Arg(Function::CHandler handler) {
    const T arg = handler.takeArg<T>(0);
    CHECK_EQ(arg, expected);
    return sy::ProgramRuntimeError();
}

TEST_SUITE("C 1 arg no return") {
    TEST_CASE("int32_t") {
        const Type* argTypes[1] = {Type::TYPE_I32};
        const Function func = {
            StringSlice(""), StringSlice(""),
            nullptr,
            argTypes, 1,
            SY_FUNCTION_MIN_ALIGN,
            Function::CallType::C,
            reinterpret_cast<const void*>(&simpleFunc1Arg<int32_t, 56>)
        };

        Function::CallArgs callArgs = func.startCall();
        int32_t arg = 56;
        callArgs.push(&arg, Type::TYPE_I32);
        CHECK(callArgs.call(nullptr).ok());
    }
}

#endif // SYNC_LIB_TEST
