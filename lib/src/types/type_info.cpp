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

/// Supports atomic load/store for GenPool
template <typename T>
static sy::Type makePrimitiveType(StringSlice inName, sy::Type::Tag inTag,
                                  sy::Type::ExtraInfo inExtra) {
    const sy::Type type{
        sizeof(T),
        static_cast<decltype(sy::Type::alignType)>(alignof(T)),
        inName,
    };
}

static constexpr sy::Function<void(void*)> EMPTY_DESTRUCTOR = +[](void*) {};

constexpr static decltype(sy::BuiltInCoherentTraits::clone) BOOL_BUILTIN_COHERENT_CLONE =
    sy::BuiltInCoherentTraits::makeClone<bool>();
constexpr static decltype(sy::BuiltInCoherentTraits::equal) BOOL_BUILTIN_COHERENT_EQUALITY =
    sy::BuiltInCoherentTraits::makeEqualityFunction<bool>();
constexpr static decltype(sy::BuiltInCoherentTraits::hash) BOOL_BUILTIN_COHERENT_HASH =
    sy::BuiltInCoherentTraits::makeHashFunction<bool>();
constexpr static decltype(sy::BuiltInCoherentTraits::compare) BOOL_BUILTIN_COHERENT_COMPARE =
    sy::BuiltInCoherentTraits::makeCompareFunction<bool>();
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicDestroy)
    BOOL_ELEMENT_WISE_ATOMIC_DESTROY = &EMPTY_DESTRUCTOR;
constexpr static Function<void(void* dst, const void* src)> BOOL_ELEMENT_WISE_ATOMIC_LOAD_IMPL =
    doAtomicCloneStd<bool>;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicLoad)
    BOOL_ELEMENT_WISE_ATOMIC_LOAD = &BOOL_ELEMENT_WISE_ATOMIC_LOAD_IMPL;
constexpr static Function<void(void* dst, const void* src)> BOOL_ELEMENT_WISE_ATOMIC_STORE_IMPL =
    doAtomicCloneStd<bool>;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicStore)
    BOOL_ELEMENT_WISE_ATOMIC_STORE = &BOOL_ELEMENT_WISE_ATOMIC_STORE_IMPL;
constexpr static sy::BuiltInCoherentTraits BOOL_BUILTIN_TRAITS = {
    .clone = BOOL_BUILTIN_COHERENT_CLONE,
    .equal = BOOL_BUILTIN_COHERENT_EQUALITY,
    .hash = BOOL_BUILTIN_COHERENT_HASH,
    .compare = BOOL_BUILTIN_COHERENT_COMPARE,
    .elementWiseAtomicDestroy = BOOL_ELEMENT_WISE_ATOMIC_DESTROY,
    .elementWiseAtomicLoad = BOOL_ELEMENT_WISE_ATOMIC_LOAD,
    .elementWiseAtomicStore = BOOL_ELEMENT_WISE_ATOMIC_STORE};

namespace sy {
namespace internal {
constinit extern sy::Type TYPE_BOOL;
} // namespace internal
} // namespace sy

constinit static sy::Type BOOL_CONST_REF_TYPE = {
    sizeof(const bool*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const bool*)),
    sy::StringSlice("*bool"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_BOOL}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit static sy::Type BOOL_MUT_REF_TYPE = {
    sizeof(bool*),
    static_cast<decltype(sy::Type::alignType)>(alignof(bool*)),
    sy::StringSlice("*mut bool"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{true, &sy::internal::TYPE_BOOL}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

namespace sy {
namespace internal {
constinit sy::Type TYPE_BOOL = {
    .sizeType = sizeof(bool),
    .alignType = static_cast<uint16_t>(alignof(bool)),
    .name = "bool",
    .tag = Type::Tag::Bool,
    .extra = Type::ExtraInfo(),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &BOOL_BUILTIN_TRAITS,
    .constRef = &BOOL_CONST_REF_TYPE,
    .mutRef = &BOOL_MUT_REF_TYPE,
};
}
} // namespace sy

static constexpr sy::Function<void(void*)> TYPE_OPAQUE_PTR_DESTRUCTOR = +[](void*) {};
namespace sy {
namespace internal {
constinit sy::Type TYPE_OPAQUE_PTR = Type{sizeof(void*),
                                          static_cast<uint16_t>(alignof(void*)),
                                          "ptr",
                                          Type::Tag::OpaquePointer,
                                          {},
                                          &EMPTY_DESTRUCTOR,
                                          nullptr,
                                          nullptr,
                                          nullptr};
}
} // namespace sy

const Type* const sy::Type::TYPE_I8 =
    Type::makeType<int8_t>("i8", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{true, 8}));
const Type* const sy::Type::TYPE_I16 =
    Type::makeType<int16_t>("i16", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{true, 16}));
const Type* const sy::Type::TYPE_I32 =
    Type::makeType<int32_t>("i32", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{true, 32}));
const Type* const sy::Type::TYPE_I64 =
    Type::makeType<int64_t>("i64", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{true, 64}));
const Type* const sy::Type::TYPE_U8 =
    Type::makeType<uint8_t>("u8", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{false, 8}));
const Type* const sy::Type::TYPE_U16 = Type::makeType<uint16_t>(
    "u16", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{false, 16}));
const Type* const sy::Type::TYPE_U32 = Type::makeType<uint32_t>(
    "u32", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{false, 32}));
const Type* const sy::Type::TYPE_U64 = Type::makeType<uint64_t>(
    "u64", Type::Tag::Int, Type::ExtraInfo(Type::ExtraInfo::Int{false, 64}));

constexpr static decltype(sy::BuiltInCoherentTraits::clone) USIZE_BUILTIN_COHERENT_CLONE =
    sy::BuiltInCoherentTraits::makeClone<size_t>();
constexpr static decltype(sy::BuiltInCoherentTraits::equal) USIZE_BUILTIN_COHERENT_EQUALITY =
    sy::BuiltInCoherentTraits::makeEqualityFunction<size_t>();
constexpr static decltype(sy::BuiltInCoherentTraits::hash) USIZE_BUILTIN_COHERENT_HASH =
    sy::BuiltInCoherentTraits::makeHashFunction<size_t>();
constexpr static decltype(sy::BuiltInCoherentTraits::compare) USIZE_BUILTIN_COHERENT_COMPARE =
    sy::BuiltInCoherentTraits::makeCompareFunction<size_t>();
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicDestroy)
    USIZE_ELEMENT_WISE_ATOMIC_DESTROY = &EMPTY_DESTRUCTOR;
constexpr static Function<void(void* dst, const void* src)> USIZE_ELEMENT_WISE_ATOMIC_LOAD_IMPL =
    doAtomicCloneStd<size_t>;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicLoad)
    USIZE_ELEMENT_WISE_ATOMIC_LOAD = &USIZE_ELEMENT_WISE_ATOMIC_LOAD_IMPL;
constexpr static Function<void(void* dst, const void* src)> USIZE_ELEMENT_WISE_ATOMIC_STORE_IMPL =
    doAtomicCloneStd<size_t>;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicStore)
    USIZE_ELEMENT_WISE_ATOMIC_STORE = &USIZE_ELEMENT_WISE_ATOMIC_STORE_IMPL;
constexpr static sy::BuiltInCoherentTraits USIZE_BUILTIN_TRAITS = {
    .clone = USIZE_BUILTIN_COHERENT_CLONE,
    .equal = USIZE_BUILTIN_COHERENT_EQUALITY,
    .hash = USIZE_BUILTIN_COHERENT_HASH,
    .compare = USIZE_BUILTIN_COHERENT_COMPARE,
    .elementWiseAtomicDestroy = USIZE_ELEMENT_WISE_ATOMIC_DESTROY,
    .elementWiseAtomicLoad = USIZE_ELEMENT_WISE_ATOMIC_LOAD,
    .elementWiseAtomicStore = USIZE_ELEMENT_WISE_ATOMIC_STORE};

namespace sy {
namespace internal {
constinit extern sy::Type TYPE_USIZE;
} // namespace internal
} // namespace sy

constinit static sy::Type USIZE_CONST_REF_TYPE = {
    sizeof(const size_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const size_t*)),
    sy::StringSlice("*usize"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_USIZE}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit static sy::Type USIZE_MUT_REF_TYPE = {
    sizeof(size_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(size_t*)),
    sy::StringSlice("*mut usize"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{true, &sy::internal::TYPE_USIZE}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

namespace sy {
namespace internal {
constinit sy::Type TYPE_USIZE = {
    .sizeType = sizeof(size_t),
    .alignType = static_cast<uint16_t>(alignof(size_t)),
    .name = "usize",
    .tag = Type::Tag::Int,
    .extra = Type::ExtraInfo(Type::ExtraInfo::Int{false, sizeof(size_t) * 8}),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &USIZE_BUILTIN_TRAITS,
    .constRef = &USIZE_CONST_REF_TYPE,
    .mutRef = &USIZE_MUT_REF_TYPE,
};
}
} // namespace sy

#pragma region Float

const Type* const sy::Type::TYPE_F32 =
    Type::makeType<float>("f32", Type::Tag::Float, Type::ExtraInfo(Type::ExtraInfo::Float{32}));
const Type* const sy::Type::TYPE_F64 =
    Type::makeType<double>("f64", Type::Tag::Float, Type::ExtraInfo(Type::ExtraInfo::Float{64}));

#pragma endregion

#pragma region String

// const Type* const sy::Type::TYPE_CHAR = nullptr;
const Type* const sy::Type::TYPE_STRING_SLICE =
    Type::makeType<sy::StringSlice>("str", Type::Tag::StringSlice, Type::ExtraInfo());

constexpr static decltype(sy::BuiltInCoherentTraits::clone) STRING_BUILTIN_COHERENT_CLONE =
    sy::BuiltInCoherentTraits::makeClone<sy::String>();
constexpr static decltype(sy::BuiltInCoherentTraits::equal) STRING_BUILTIN_COHERENT_EQUALITY =
    sy::BuiltInCoherentTraits::makeEqualityFunction<sy::String>();
constexpr static decltype(sy::BuiltInCoherentTraits::hash) STRING_BUILTIN_COHERENT_HASH =
    sy::BuiltInCoherentTraits::makeHashFunction<sy::String>();

constexpr static sy::Function<void(void* self)> STRING_BUILTIN_COHERENT_ATOMIC_DESTROY_FN =
    +[](void* self) {
        sy::String* tSelf = reinterpret_cast<sy::String*>(self);
        sy::internal::AtomicStringHeader::atomicStringDestroy(tSelf);
    };
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicDestroy)
    STRING_BUILTIN_COHERENT_ATOMIC_DESTROY = &STRING_BUILTIN_COHERENT_ATOMIC_DESTROY_FN;

constexpr static sy::Function<void(void* dst, const void* src)>
    STRING_BUILTIN_COHERENT_ATOMIC_LOAD_FN = +[](void* out, const void* src) {
        sy::String* tDst = reinterpret_cast<sy::String*>(out);
        const sy::String* tSrc = reinterpret_cast<const sy::String*>(src);
        sy::internal::AtomicStringHeader::atomicStringClone(tDst, tSrc);
    };
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicLoad)
    STRING_BUILTIN_COHERENT_ATOMIC_LOAD = &STRING_BUILTIN_COHERENT_ATOMIC_LOAD_FN;

constexpr static sy::Function<void(void* dst, const void* src)>
    STRING_BUILTIN_COHERENT_ATOMIC_STORE_FN = +[](void* overwrite, const void* src) {
        sy::String* tDst = reinterpret_cast<sy::String*>(overwrite);
        const sy::String* tSrc = reinterpret_cast<const sy::String*>(src);
        sy::internal::AtomicStringHeader::atomicStringSet(tDst, tSrc);
    };
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicStore)
    STRING_BUILTIN_COHERENT_ATOMIC_STORE = &STRING_BUILTIN_COHERENT_ATOMIC_STORE_FN;

constexpr static sy::BuiltInCoherentTraits STRING_BUILTIN_COHERENT_TRAITS = {
    STRING_BUILTIN_COHERENT_CLONE,          STRING_BUILTIN_COHERENT_EQUALITY,
    STRING_BUILTIN_COHERENT_HASH,           {},
    STRING_BUILTIN_COHERENT_ATOMIC_DESTROY, STRING_BUILTIN_COHERENT_ATOMIC_LOAD,
    STRING_BUILTIN_COHERENT_ATOMIC_STORE,
};

constexpr static sy::Function<void(void* self)> STRING_DESTRUCTOR_FN = +[](void* self) {
    sy::String* tSelf = reinterpret_cast<sy::String*>(self);
    tSelf->~String();
};

constexpr static sy::Function<void(void* self)> STRING_REF_EMPTY_DESTRUCTOR =
    +[](void* self) { (void)self; };

constexpr static decltype(sy::BuiltInCoherentTraits::clone) STRING_REF_BUILTIN_COHERENT_CLONE =
    sy::BuiltInCoherentTraits::makeClone<sy::String*>();
constexpr static decltype(sy::BuiltInCoherentTraits::equal) STRING_REF_BUILTIN_COHERENT_EQUALITY =
    sy::BuiltInCoherentTraits::makeEqualityFunction<sy::String*>();

constexpr static sy::BuiltInCoherentTraits STRING_REF_BUILTIN_COHERENT_TRAITS = {
    STRING_REF_BUILTIN_COHERENT_CLONE, STRING_REF_BUILTIN_COHERENT_EQUALITY, {}, {}, {}, {}, {},
};

namespace sy {
namespace internal {
constinit extern sy::Type TYPE_STRING;
} // namespace internal
} // namespace sy

constinit static sy::Type STRING_CONST_REF_TYPE = {
    sizeof(const sy::String*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const sy::String*)),
    sy::StringSlice("*String"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_STRING}),
    &STRING_REF_EMPTY_DESTRUCTOR,
    &STRING_REF_BUILTIN_COHERENT_TRAITS,
    nullptr,
    nullptr,
};

constinit static sy::Type STRING_MUT_REF_TYPE = {
    sizeof(sy::String*),
    static_cast<decltype(sy::Type::alignType)>(alignof(sy::String*)),
    sy::StringSlice("*mut String"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{true, &sy::internal::TYPE_STRING}),
    &STRING_REF_EMPTY_DESTRUCTOR,
    &STRING_REF_BUILTIN_COHERENT_TRAITS,
    nullptr,
    nullptr,
};

namespace sy {
namespace internal {
constinit sy::Type TYPE_STRING = {
    sizeof(sy::String),
    static_cast<decltype(sy::Type::alignType)>(alignof(sy::String)),
    sy::StringSlice("String"),
    sy::Type::Tag::String,
    sy::Type::ExtraInfo(),
    &STRING_DESTRUCTOR_FN,
    &STRING_BUILTIN_COHERENT_TRAITS,
    &STRING_CONST_REF_TYPE,
    &STRING_MUT_REF_TYPE,
};
}
} // namespace sy

#pragma endregion

constexpr static decltype(sy::BuiltInCoherentTraits::clone) ORDERING_BUILTIN_COHERENT_CLONE =
    sy::BuiltInCoherentTraits::makeClone<sy::Ordering>();
constexpr static decltype(sy::BuiltInCoherentTraits::equal) ORDERING_BUILTIN_COHERENT_EQUALITY =
    sy::BuiltInCoherentTraits::makeEqualityFunction<sy::Ordering>();
constexpr static decltype(sy::BuiltInCoherentTraits::hash) ORDERING_BUILTIN_COHERENT_HASH =
    sy::BuiltInCoherentTraits::makeHashFunction<sy::Ordering>();
constexpr static decltype(sy::BuiltInCoherentTraits::compare) ORDERING_BUILTIN_COHERENT_COMPARE =
    sy::BuiltInCoherentTraits::makeCompareFunction<int32_t>();
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicDestroy)
    ORDERING_ELEMENT_WISE_ATOMIC_DESTROY = &EMPTY_DESTRUCTOR;
constexpr static Function<void(void* dst, const void* src)> ORDERING_ELEMENT_WISE_ATOMIC_LOAD_IMPL =
    doAtomicCloneStd<int32_t>;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicLoad)
    ORDERING_ELEMENT_WISE_ATOMIC_LOAD = &ORDERING_ELEMENT_WISE_ATOMIC_LOAD_IMPL;
constexpr static Function<void(void* dst, const void* src)>
    ORDERING_ELEMENT_WISE_ATOMIC_STORE_IMPL = doAtomicCloneStd<int32_t>;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicStore)
    ORDERING_ELEMENT_WISE_ATOMIC_STORE = &ORDERING_ELEMENT_WISE_ATOMIC_STORE_IMPL;
constexpr static sy::BuiltInCoherentTraits ORDERING_BUILTIN_TRAITS = {
    .clone = ORDERING_BUILTIN_COHERENT_CLONE,
    .equal = ORDERING_BUILTIN_COHERENT_EQUALITY,
    .hash = ORDERING_BUILTIN_COHERENT_HASH,
    .compare = ORDERING_BUILTIN_COHERENT_COMPARE,
    .elementWiseAtomicDestroy = ORDERING_ELEMENT_WISE_ATOMIC_DESTROY,
    .elementWiseAtomicLoad = ORDERING_ELEMENT_WISE_ATOMIC_LOAD,
    .elementWiseAtomicStore = ORDERING_ELEMENT_WISE_ATOMIC_STORE};

namespace sy {
namespace internal {
constinit extern sy::Type TYPE_ORDERING;
} // namespace internal
} // namespace sy

constinit static sy::Type ORDERING_CONST_REF_TYPE = {
    sizeof(const sy::Ordering*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const sy::Ordering*)),
    sy::StringSlice("*Ordering"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_ORDERING}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit static sy::Type ORDERING_MUT_REF_TYPE = {
    sizeof(sy::Ordering*),
    static_cast<decltype(sy::Type::alignType)>(alignof(sy::Ordering*)),
    sy::StringSlice("*mut Ordering"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{true, &sy::internal::TYPE_ORDERING}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

namespace sy {
namespace internal {
constinit sy::Type TYPE_ORDERING = {
    .sizeType = sizeof(sy::Ordering),
    .alignType = static_cast<uint16_t>(alignof(sy::Ordering)),
    .name = "Ordering",
    .tag = Type::Tag::Ordering,
    .extra = Type::ExtraInfo(),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &ORDERING_BUILTIN_TRAITS,
    .constRef = &ORDERING_CONST_REF_TYPE,
    .mutRef = &ORDERING_MUT_REF_TYPE,
};
}
} // namespace sy

extern "C" {
SY_API constinit const SyType* SY_TYPE_BOOL =
    reinterpret_cast<const SyType*>(&sy::internal::TYPE_BOOL);

SY_API const SyType* SY_TYPE_I8 = reinterpret_cast<const SyType*>(Type::TYPE_I8);
SY_API const SyType* SY_TYPE_I16 = reinterpret_cast<const SyType*>(Type::TYPE_I16);
SY_API const SyType* SY_TYPE_I32 = reinterpret_cast<const SyType*>(Type::TYPE_I32);
SY_API const SyType* SY_TYPE_I64 = reinterpret_cast<const SyType*>(Type::TYPE_I64);
SY_API const SyType* SY_TYPE_U8 = reinterpret_cast<const SyType*>(Type::TYPE_U8);
SY_API const SyType* SY_TYPE_U16 = reinterpret_cast<const SyType*>(Type::TYPE_U16);
SY_API const SyType* SY_TYPE_U32 = reinterpret_cast<const SyType*>(Type::TYPE_U32);
SY_API const SyType* SY_TYPE_U64 = reinterpret_cast<const SyType*>(Type::TYPE_U64);
SY_API constinit const SyType* SY_TYPE_USIZE =
    reinterpret_cast<const SyType*>(&sy::internal::TYPE_USIZE);

SY_API const SyType* SY_TYPE_F32 = reinterpret_cast<const SyType*>(Type::TYPE_F32);
SY_API const SyType* SY_TYPE_F64 = reinterpret_cast<const SyType*>(Type::TYPE_F64);

// SY_API const SyType* SY_TYPE_CHAR           = reinterpret_cast<const SyType*>(Type::TYPE_CHAR);
SY_API constinit const SyType* SY_TYPE_STRING =
    reinterpret_cast<const SyType*>(&sy::internal::TYPE_STRING);
SY_API const SyType* SY_TYPE_STRING_SLICE =
    reinterpret_cast<const SyType*>(Type::TYPE_STRING_SLICE);

SY_API constinit const SyType* SY_TYPE_ORDERING =
    reinterpret_cast<const SyType*>(&sy::internal::TYPE_ORDERING);
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
            }
        }
        return {};
    case Tag::Float:
        if (this->extra.floatInfo.bits == 32) {
            doAtomicCloneStd<int32_t>(dst, src);
        } else {
            doAtomicCloneStd<int64_t>(dst, src);
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
            }
        }
        return {};
    case Tag::Float:
        if (this->extra.floatInfo.bits == 32) {
            doAtomicCloneStd<int32_t>(dst, src);
        } else {
            doAtomicCloneStd<int64_t>(dst, src);
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
