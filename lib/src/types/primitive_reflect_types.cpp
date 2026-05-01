// separate translation unit to avoid the compiler type mismatch redefinition stuff,
// plus organization

#include "../core/builtin_traits/builtin_traits.hpp"
#include "function/function.hpp"
#include "option/option.hpp"
#include "reflect_fwd.hpp"
#include "string/string.hpp"
#include "string/string_internal.hpp"
#include "string/string_slice.hpp"
#include "type_info.hpp"

using sy::BuiltInCoherentTraits;
using sy::Function;
using sy::Type;

template <typename T> static void doAtomicCloneStd(void* dst, const void* src) {
    std::atomic<T>* atomicDst = reinterpret_cast<std::atomic<T>*>(dst);
    const std::atomic<T>* atomicSrc = reinterpret_cast<const std::atomic<T>*>(src);
    T temp = atomicSrc->load(std::memory_order_relaxed);
    atomicDst->store(temp, std::memory_order_relaxed);
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
SY_API constinit const sy::Type* SY_TYPE_BOOL = &sy::internal::TYPE_BOOL;

SY_API const sy::Type* SY_TYPE_I8 = Type::TYPE_I8;
SY_API const sy::Type* SY_TYPE_I16 = Type::TYPE_I16;
SY_API const sy::Type* SY_TYPE_I32 = Type::TYPE_I32;
SY_API const sy::Type* SY_TYPE_I64 = Type::TYPE_I64;
SY_API const sy::Type* SY_TYPE_U8 = Type::TYPE_U8;
SY_API const sy::Type* SY_TYPE_U16 = Type::TYPE_U16;
SY_API const sy::Type* SY_TYPE_U32 = Type::TYPE_U32;
SY_API const sy::Type* SY_TYPE_U64 = Type::TYPE_U64;
SY_API constinit const sy::Type* SY_TYPE_USIZE = &sy::internal::TYPE_USIZE;

SY_API const sy::Type* SY_TYPE_F32 = Type::TYPE_F32;
SY_API const sy::Type* SY_TYPE_F64 = Type::TYPE_F64;

// SY_API const SyType* SY_TYPE_CHAR           = reinterpret_cast<const SyType*>(Type::TYPE_CHAR);
SY_API constinit const sy::Type* SY_TYPE_STRING = &sy::internal::TYPE_STRING;
SY_API const sy::Type* SY_TYPE_STRING_SLICE = Type::TYPE_STRING_SLICE;

SY_API constinit const sy::Type* SY_TYPE_ORDERING = &sy::internal::TYPE_ORDERING;
}
