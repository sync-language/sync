#include "type_info.h"
#include "../core/builtin_traits/builtin_traits.hpp"
#include "../core/core_internal.h"
#include "../program/program.hpp"
#include "function/function.h"
#include "function/function.hpp"
#include "string/string.hpp"
#include "string/string_internal.hpp"
#include "string/string_slice.hpp"
#include "type_info.hpp"
#include <atomic>

using namespace sy;

static_assert(sizeof(float) == 4);  // f32
static_assert(sizeof(double) == 8); // f64

static_assert(sizeof(Type::Tag) == sizeof(SyTypeTag));
static_assert(alignof(Type::Tag) == alignof(SyTypeTag));
static_assert(sizeof(Type::Tag) == sizeof(int));
static_assert(alignof(Type::Tag) == alignof(int));
static_assert(static_cast<int>(Type::Tag::Bool) == static_cast<int>(SyTypeTagBool));
static_assert(static_cast<int>(Type::Tag::Int) == static_cast<int>(SyTypeTagInt));
static_assert(static_cast<int>(Type::Tag::Float) == static_cast<int>(SyTypeTagFloat));
static_assert(static_cast<int>(Type::Tag::OpaquePointer) ==
              static_cast<int>(SyTypeTagOpaquePointer));
static_assert(static_cast<int>(Type::Tag::StringSlice) == static_cast<int>(SyTypeTagStringSlice));
static_assert(static_cast<int>(Type::Tag::String) == static_cast<int>(SyTypeTagString));
static_assert(static_cast<int>(Type::Tag::Reference) == static_cast<int>(SyTypeTagReference));
static_assert(static_cast<int>(Type::Tag::Function) == static_cast<int>(SyTypeTagFunction));

static_assert(sizeof(Type::ExtraInfo) == sizeof(SyTypeExtraInfo));
static_assert(alignof(Type::ExtraInfo) == alignof(SyTypeExtraInfo));

static_assert(sizeof(Type::ExtraInfo::Int) == sizeof(SyTypeInfoInt));
static_assert(alignof(Type::ExtraInfo::Int) == alignof(SyTypeInfoInt));
static_assert(offsetof(Type::ExtraInfo::Int, isSigned) == offsetof(SyTypeInfoInt, isSigned));
static_assert(offsetof(Type::ExtraInfo::Int, bits) == offsetof(SyTypeInfoInt, bits));

static_assert(sizeof(Type::ExtraInfo::Float) == sizeof(SyTypeInfoFloat));
static_assert(alignof(Type::ExtraInfo::Float) == alignof(SyTypeInfoFloat));
static_assert(offsetof(Type::ExtraInfo::Float, bits) == offsetof(SyTypeInfoFloat, bits));

static_assert(sizeof(Type::ExtraInfo::Reference) == sizeof(SyTypeInfoReference));
static_assert(alignof(Type::ExtraInfo::Reference) == alignof(SyTypeInfoReference));
static_assert(offsetof(Type::ExtraInfo::Reference, isMutable) ==
              offsetof(SyTypeInfoReference, isMutable));
static_assert(offsetof(Type::ExtraInfo::Reference, childType) ==
              offsetof(SyTypeInfoReference, childType));

static_assert(sizeof(Type::ExtraInfo::Function) == sizeof(SyTypeInfoFunction));
static_assert(alignof(Type::ExtraInfo::Function) == alignof(SyTypeInfoFunction));
static_assert(offsetof(Type::ExtraInfo::Function, retType) ==
              offsetof(SyTypeInfoFunction, retType));
static_assert(offsetof(Type::ExtraInfo::Function, argTypes) ==
              offsetof(SyTypeInfoFunction, argTypes));
static_assert(offsetof(Type::ExtraInfo::Function, argLen) == offsetof(SyTypeInfoFunction, argLen));

static_assert(sizeof(Type) == sizeof(SyType));
static_assert(alignof(Type) == alignof(SyType));
static_assert(offsetof(Type, sizeType) == offsetof(SyType, sizeType));
static_assert(offsetof(Type, alignType) == offsetof(SyType, alignType));
static_assert(offsetof(Type, name) == offsetof(SyType, name));
static_assert(offsetof(Type, destructor) == offsetof(SyType, destructor));
static_assert(offsetof(Type, builtinTraits) == offsetof(SyType, builtinTraits));
static_assert(offsetof(Type, tag) == offsetof(SyType, tag));
static_assert(offsetof(Type, extra) == offsetof(SyType, extra));
static_assert(offsetof(Type, constRef) == offsetof(SyType, constRef));
static_assert(offsetof(Type, mutRef) == offsetof(SyType, mutRef));

template <typename T> static void doAtomicCloneStd(void* dst, const void* src) {
    std::atomic<T>* atomicDst = reinterpret_cast<std::atomic<T>*>(dst);
    const std::atomic<T>* atomicSrc = reinterpret_cast<const std::atomic<T>*>(src);
    T temp = atomicSrc->load(std::memory_order_relaxed);
    atomicDst->store(temp, std::memory_order_relaxed);
}

void sy::Type::assertTypeSizeAlignMatch(size_t sizeOfType, size_t alignOfType) const {
    sy_assert(this->sizeType == sizeOfType, "Type size mismatch");
    sy_assert(this->alignType == alignOfType, "Type align mismatch");
    (void)sizeOfType;
    (void)alignOfType;
}

Result<void, ProgramError> sy::Type::destroyObjectImpl(void* obj) const {
    sy_assert(obj != nullptr, "Cannot destroy null object");
    sy_assert(this->destructor != nullptr, "All objects must have destructors");

    switch (this->tag) {
    case Tag::Bool:
    case Tag::Int:
    case Tag::Float:
    case Tag::OpaquePointer:
    // case Tag::Char:
    case Tag::StringSlice:
    case Tag::Reference:
    case Tag::Function:
        return {};
    case Tag::String: {
        String* objAsString = reinterpret_cast<String*>(obj);
        objAsString->~String();
        return {};
    }
    default:
        break;
    }

    sy_assert(this->mutRef != nullptr, "Destructors take mutable references");
    sy_assert(this->mutRef->sizeType == sizeof(void*),
              "Mutable reference type should have the same size as void*");
    sy_assert(this->mutRef->alignType == alignof(void*),
              "Mutable reference type should have the same align as void*");

    return this->destructor->call(obj);
}

Result<void, ProgramError> sy::Type::cloneObjectImpl(void* dst, const void* src) const {
    sy_assert(dst != nullptr, "Cannot copy to null object");
    sy_assert(src != nullptr, "Cannot copy from null object");
    sy_assert(this->builtinTraits->clone.hasValue(),
              "Cannot do equality comparison without an equality function");

    // TODO immediate copy construction for simple types

    sy_assert(this->mutRef != nullptr, "Copy constructor takes mutable reference");
    sy_assert(this->mutRef->sizeType == sizeof(void*),
              "Mutable reference types should be the same size as void*");
    sy_assert(this->mutRef->alignType == alignof(void*),
              "Mutable reference types should be the same align as void*");
    sy_assert(this->constRef != nullptr, "Copy constructor takes const reference");
    sy_assert(this->constRef->sizeType == sizeof(void*),
              "Const reference types should be the same size as void*");
    sy_assert(this->constRef->alignType == alignof(void*),
              "Const reference types should be the same align as void*");

    return this->builtinTraits->clone.value()->call(dst, src);
}

Result<bool, ProgramError> sy::Type::equalObjectsImpl(const void* self, const void* other) const {
    sy_assert(self != nullptr, "Cannot equality compare null object");
    sy_assert(other != nullptr, "Cannot equality compare null object");
    sy_assert(this->builtinTraits->equal.hasValue(),
              "Cannot do equality comparison without an equality function");

    // TODO immediate equality comparison for simple types

    sy_assert(this->constRef != nullptr, "Equality comparison takes const references");
    sy_assert(this->constRef->sizeType == sizeof(void*),
              "Const reference types should be the same size as void*");
    sy_assert(this->constRef->alignType == alignof(void*),
              "Const reference types should be the same align as void*");

    return this->builtinTraits->equal.value()->call(self, other);
}

Result<size_t, ProgramError> sy::Type::hashObjectImpl(const void* self) const {
    sy_assert(self != nullptr, "Cannot hash null object");
    sy_assert(this->builtinTraits->hash.hasValue(), "Cannot do hash without a hash function");

    // TODO immediate hash for simple types

    sy_assert(this->constRef != nullptr, "Equality comparison takes const references");
    sy_assert(this->constRef->sizeType == sizeof(void*),
              "Const reference types should be the same size as void*");
    sy_assert(this->constRef->alignType == alignof(void*),
              "Const reference types should be the same align as void*");

    return this->builtinTraits->hash.value()->call(self);
}

Result<Ordering, ProgramError> sy::Type::compareObjectImpl(const void* self,
                                                           const void* other) const {
    sy_assert(self != nullptr, "Cannot equality compare null object");
    sy_assert(other != nullptr, "Cannot equality compare null object");
    sy_assert(this->builtinTraits->compare.hasValue(),
              "Cannot do equality comparison without an equality function");

    // TODO immediate equality comparison for simple types

    sy_assert(this->constRef != nullptr, "Equality comparison takes const references");
    sy_assert(this->constRef->sizeType == sizeof(void*),
              "Const reference types should be the same size as void*");
    sy_assert(this->constRef->alignType == alignof(void*),
              "Const reference types should be the same align as void*");

    return this->builtinTraits->compare.value()->call(self, other);
}

Result<void, ProgramError> sy::Type::elementWiseAtomicDestroyObjImpl(void* obj) const {
    sy_assert(obj != nullptr, "Cannot atomically destroy null object");
    sy_assert(this->builtinTraits->elementWiseAtomicDestroy.hasValue(),
              "Cannot do element wise atomic destroy without a function");

    switch (this->tag) {
    case Tag::Bool:
    case Tag::Int:
    case Tag::Float:
        return {};
    case Tag::String: {
        internal::AtomicStringHeader::atomicStringDestroy(reinterpret_cast<String*>(obj));
    }
    default:
        break;
    }

    sy_assert(this->mutRef != nullptr, "Atomic destroy takes mutable reference");
    sy_assert(this->mutRef->sizeType == sizeof(void*),
              "Mutable reference types should be the same size as void*");
    sy_assert(this->mutRef->alignType == alignof(void*),
              "Mutable reference types should be the same align as void*");

    return this->builtinTraits->elementWiseAtomicDestroy.value()->call(obj);
}

Result<void, ProgramError> sy::Type::elementWiseAtomicLoadObjImpl(void* dst,
                                                                  const void* src) const {
    sy_assert(dst != nullptr, "Cannot copy to null object");
    sy_assert(src != nullptr, "Cannot copy from null object");
    sy_assert(this->builtinTraits->elementWiseAtomicLoad.hasValue(),
              "Cannot perform atomic clone without an atomic clone function");

    switch (this->tag) {
    case Tag::Bool:
        doAtomicCloneStd<bool>(dst, src);
        return {};
    case Tag::Int:
        if (this->extra.intInfo.isSigned) {
            switch (this->extra.intInfo.bits) {
            case 8: {
                doAtomicCloneStd<int8_t>(dst, src);
            }
            case 16: {
                doAtomicCloneStd<int16_t>(dst, src);
            }
            case 32: {
                doAtomicCloneStd<int32_t>(dst, src);
            }
            case 64: {
                doAtomicCloneStd<int64_t>(dst, src);
            }
            default: {
                sync_unreachable();
            }
            }
        } else {
            switch (this->extra.intInfo.bits) {
            case 8: {
                doAtomicCloneStd<uint8_t>(dst, src);
            }
            case 16: {
                doAtomicCloneStd<uint16_t>(dst, src);
            }
            case 32: {
                doAtomicCloneStd<uint32_t>(dst, src);
            }
            case 64: {
                doAtomicCloneStd<uint64_t>(dst, src);
            }
            default: {
                sync_unreachable();
            }
            }
        }
        return {};
    case Tag::Float:
        switch (this->extra.floatInfo.bits) {
        case 32: {
            doAtomicCloneStd<int32_t>(dst, src);
        } break;
        case 64: {
            doAtomicCloneStd<int64_t>(dst, src);
        } break;
        default: {
            sync_unreachable();
        }
        }
        return {};
    case Tag::String: {
        internal::AtomicStringHeader::atomicStringClone(reinterpret_cast<String*>(dst),
                                                        reinterpret_cast<const String*>(src));
    }
    default:
        break;
    }

    sy_assert(this->mutRef != nullptr, "Atomic clone takes mutable reference");
    sy_assert(this->mutRef->sizeType == sizeof(void*),
              "Mutable reference types should be the same size as void*");
    sy_assert(this->mutRef->alignType == alignof(void*),
              "Mutable reference types should be the same align as void*");
    sy_assert(this->constRef != nullptr, "Atomic clone takes const reference");
    sy_assert(this->constRef->sizeType == sizeof(void*),
              "Const reference types should be the same size as void*");
    sy_assert(this->constRef->alignType == alignof(void*),
              "Const reference types should be the same align as void*");

    return this->builtinTraits->elementWiseAtomicLoad.value()->call(dst, src);
}

Result<void, ProgramError> sy::Type::elementWiseAtomicStoreObjImpl(void* dst,
                                                                   const void* src) const {
    sy_assert(dst != nullptr, "Cannot move to null object");
    sy_assert(src != nullptr, "Cannot move from null object");
    sy_assert(this->builtinTraits->elementWiseAtomicStore.hasValue(),
              "Cannot perform atomic move without an atomic move function");

    switch (this->tag) {
    // the PoD types can just do clone directly.
    case Tag::Bool:
        doAtomicCloneStd<bool>(dst, src);
        return {};
    case Tag::Int:
        if (this->extra.intInfo.isSigned) {
            switch (this->extra.intInfo.bits) {
            case 8: {
                doAtomicCloneStd<int8_t>(dst, src);
            } break;
            case 16: {
                doAtomicCloneStd<int16_t>(dst, src);
            } break;
            case 32: {
                doAtomicCloneStd<int32_t>(dst, src);
            } break;
            case 64: {
                doAtomicCloneStd<int64_t>(dst, src);
            } break;
            default: {
                sync_unreachable();
            }
            }
        } else {
            switch (this->extra.intInfo.bits) {
            case 8: {
                doAtomicCloneStd<uint8_t>(dst, src);
            } break;
            case 16: {
                doAtomicCloneStd<uint16_t>(dst, src);
            } break;
            case 32: {
                doAtomicCloneStd<uint32_t>(dst, src);
            } break;
            case 64: {
                doAtomicCloneStd<uint64_t>(dst, src);
            } break;
            default: {
                sync_unreachable();
            }
            }
        }
        return {};
    case Tag::Float:
        switch (this->extra.floatInfo.bits) {
        case 32: {
            doAtomicCloneStd<int32_t>(dst, src);
        } break;
        case 64: {
            doAtomicCloneStd<int64_t>(dst, src);
        } break;
        default: {
            sync_unreachable();
        }
        }
        return {};
    case Tag::String: {
        internal::AtomicStringHeader::atomicStringSet(
            reinterpret_cast<String*>(dst),
            reinterpret_cast<const String*>(const_cast<const void*>(src)));
    }
    default:
        break;
    }

    sy_assert(this->mutRef != nullptr, "Atomic store takes mutable reference");
    sy_assert(this->mutRef->sizeType == sizeof(void*),
              "Mutable reference types should be the same size as void*");
    sy_assert(this->mutRef->alignType == alignof(void*),
              "Mutable reference types should be the same align as void*");
    sy_assert(this->constRef != nullptr, "Atomic store takes const reference");
    sy_assert(this->constRef->sizeType == sizeof(void*),
              "Const reference types should be the same size as void*");
    sy_assert(this->constRef->alignType == alignof(void*),
              "Const reference types should be the same align as void*");

    return this->builtinTraits->elementWiseAtomicStore.value()->call(dst, src);
}

#if SYNC_LIB_WITH_TESTS

#include "../doctest.h"

// TEST_CASE("same object") {
//     const size_t cppBoolPtr = reinterpret_cast<size_t>(sy::Type::TYPE_BOOL);
//     const size_t cBoolPtr = reinterpret_cast<size_t>(SY_TYPE_BOOL);
//     CHECK_EQ(cppBoolPtr, cBoolPtr);
// }

// TEST_CASE("destroy object") {
//     size_t n1 = 10;
//     size_t n2 = 40;

//     sy::Type::TYPE_USIZE->destroyObject(reinterpret_cast<void*>(&n1));
//     sy::Type::TYPE_USIZE->destroyObject(&n2);
// }

// TEST_CASE("string destructor") {
//     // create with new so that destructor doesn't automatically get called
//     String* s = new String("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
//     Type::TYPE_STRING->destroyObject(s);
//     delete s;
// }

// TEST_CASE("equality") {
//     CHECK(sy::Type::TYPE_BOOL->equality);

//     { // equal
//         bool lhs = true;
//         bool rhs = true;
//         bool ret;

//         RawFunction::CallArgs args = sy::Type::TYPE_BOOL->equality.value()->startCall();
//         const bool* lhsMem = &lhs;
//         args.push(&lhsMem, sy::Type::TYPE_BOOL->constRef);
//         const bool* rhsMem = &rhs;
//         args.push(&rhsMem, sy::Type::TYPE_BOOL->constRef);
//         auto err = args.call(&ret);
//         CHECK_FALSE(err.hasErr());
//         CHECK(ret);
//     }
//     { // not equal
//         bool lhs = false;
//         bool rhs = true;
//         bool ret;

//         RawFunction::CallArgs args = sy::Type::TYPE_BOOL->equality.value()->startCall();
//         const bool* lhsMem = &lhs;
//         args.push(&lhsMem, sy::Type::TYPE_BOOL->constRef);
//         const bool* rhsMem = &rhs;
//         args.push(&rhsMem, sy::Type::TYPE_BOOL->constRef);
//         auto err = args.call(&ret);
//         CHECK_FALSE(err.hasErr());
//         CHECK_FALSE(ret);
//     }
// }

// TEST_CASE("hash") {
//     CHECK(sy::Type::TYPE_U64->hash);

//     uint64_t obj = 123456789;
//     size_t ret = 0;

//     RawFunction::CallArgs args = sy::Type::TYPE_U64->hash.value()->startCall();
//     const uint64_t* objMem = &obj;
//     args.push(&objMem, sy::Type::TYPE_U64->constRef);
//     auto err = args.call(&ret);
//     CHECK_FALSE(err.hasErr());
//     if (ret == 0) {
//         std::cerr << "Possible test failure " << __FILE__ << ':' << __LINE__ << std::endl;
//     }
// }

#endif // SYNC_LIB_NO_TESTS
