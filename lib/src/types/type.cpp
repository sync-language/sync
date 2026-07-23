#include "type.hpp"
#include "../core/core_internal.h"
#include "ordering/ordering.hpp"
#include "string/string.hpp"
#include "string/string_internal.hpp"
#include "string/string_slice.hpp"

using namespace sy;

bool Type::isIndirectionLevelMutable(uint32_t level) const noexcept {
    sy_assert(this->indirection <= 32, "`indirection` value out of range");
    sy_assert(level < this->indirection,
              "Expected `level` to be less than or equal to `indirection`");

    return (this->mutableBits & (1u << level)) != 0u;
}

Type Type::reference(bool isMutable) const noexcept {
    // Check from 31 cause the new one would have 32.
    sy_assert(this->indirection <= 31, "`indirection` value out of range");
    Type out = *this;
    out.indirection += 1;
    out.mutableBits |= (static_cast<uint32_t>(isMutable) << this->indirection);
    return out;
}

Type sy::Type::dereference() const noexcept {
    sy_assert(this->indirection <= 32, "`indirection` value out of range");
    sy_assert(this->indirection > 0, "Cannot dereference non-reference type");
    Type out = *this;
    out.indirection -= 1;
    out.mutableBits &= (~(1u << out.indirection));
    return out;
}

bool sy::Type::operator==(const Type& other) const noexcept {
    if (this->indirection != other.indirection) {
        return false;
    }
    if (this->mutableBits != other.mutableBits) {
        return false;
    }
    if (this->base == other.base) {
        return true;
    }

    if (this->base->typeSize != other.base->typeSize) {
        return false;
    }
    if (this->base->typeAlign != other.base->typeAlign) {
        return false;
    }

    // don't check name, it can differ

    if (this->base->extra.tag != other.base->extra.tag) {
        return false;
    }

    using Tag = sy::TypeExtra::Tag;

    const TypeExtra::Info& thisInfo = this->base->extra.info;
    const TypeExtra::Info& otherInfo = other.base->extra.info;
    switch (this->base->extra.tag) {
    case Tag::Bool:
        return true;
    case Tag::Int:
        return (thisInfo.intInfo.isSigned == otherInfo.intInfo.isSigned) &&
               (thisInfo.intInfo.bits == otherInfo.intInfo.bits);
    case Tag::Float:
        return (thisInfo.floatInfo.bits == otherInfo.floatInfo.bits);
    case Tag::Reference:
        return (thisInfo.referenceInfo.isMutable == otherInfo.referenceInfo.isMutable) &&
               // this one recursive... hmm maybe ok
               (thisInfo.referenceInfo.childType == otherInfo.referenceInfo.childType);
    case Tag::Function: {
        // recursive...
        const bool retTypeSame = thisInfo.functionInfo.retType == otherInfo.functionInfo.retType;
        const bool argTypesSame = [&thisInfo, &otherInfo]() -> bool {
            if (thisInfo.functionInfo.argLen != otherInfo.functionInfo.argLen)
                return false;
            for (uint16_t i = 0; i < thisInfo.functionInfo.argLen; i++) {
                // very recursive...
                if (thisInfo.functionInfo.argTypes[i] != otherInfo.functionInfo.argTypes[i]) {
                    return false;
                }
            }
            return true;
        }();
        return retTypeSame && argTypesSame;
    }
    default:
        break;
    }

    return true;
}

// ==========
// PRIMITIVES
// ==========

namespace {
constexpr static BuiltInDestructorFn EMPTY_DESTRUCTOR = +[](void*) {};

template <typename T> static void doAtomicCloneStd(void* dst, const void* src) {
    std::atomic<T>* atomicDst = reinterpret_cast<std::atomic<T>*>(dst);
    const std::atomic<T>* atomicSrc = reinterpret_cast<const std::atomic<T>*>(src);
    T temp = atomicSrc->load(std::memory_order_relaxed);
    atomicDst->store(temp, std::memory_order_relaxed);
}

constexpr static BuiltInElementWiseAtomicLoadFn BUILTIN_COHERENT_1_BYTE_ATOMIC_CLONE_IMPL =
    doAtomicCloneStd<uint8_t>;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicLoad)
    BUILTIN_COHERENT_1_BYTE_ATOMIC_CLONE = &BUILTIN_COHERENT_1_BYTE_ATOMIC_CLONE_IMPL;
constexpr static BuiltInElementWiseAtomicLoadFn BUILTIN_COHERENT_2_BYTE_ATOMIC_CLONE_IMPL =
    doAtomicCloneStd<uint16_t>;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicLoad)
    BUILTIN_COHERENT_2_BYTE_ATOMIC_CLONE = &BUILTIN_COHERENT_2_BYTE_ATOMIC_CLONE_IMPL;
constexpr static BuiltInElementWiseAtomicLoadFn BUILTIN_COHERENT_4_BYTE_ATOMIC_CLONE_IMPL =
    doAtomicCloneStd<uint32_t>;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicLoad)
    BUILTIN_COHERENT_4_BYTE_ATOMIC_CLONE = &BUILTIN_COHERENT_4_BYTE_ATOMIC_CLONE_IMPL;
constexpr static BuiltInElementWiseAtomicLoadFn BUILTIN_COHERENT_8_BYTE_ATOMIC_CLONE_IMPL =
    doAtomicCloneStd<uint64_t>;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicLoad)
    BUILTIN_COHERENT_8_BYTE_ATOMIC_CLONE = &BUILTIN_COHERENT_8_BYTE_ATOMIC_CLONE_IMPL;

template <typename T> constexpr sy::BuiltInCoherentTraits makeBuiltinTraitsForPrimitive() {
    if constexpr (sizeof(T) == 1) {
        return sy::BuiltInCoherentTraits{
            .clone = sy::BuiltInCoherentTraits::makeClone<T>(),
            .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<T>(),
            .hash = sy::BuiltInCoherentTraits::makeHashFunction<T>(),
            .compare = sy::BuiltInCoherentTraits::makeCompareFunction<T>(),
            .elementWiseAtomicDestroy = &EMPTY_DESTRUCTOR,
            .elementWiseAtomicLoad = BUILTIN_COHERENT_1_BYTE_ATOMIC_CLONE,
            .elementWiseAtomicStore = BUILTIN_COHERENT_1_BYTE_ATOMIC_CLONE};
    } else if constexpr (sizeof(T) == 2) {
        return sy::BuiltInCoherentTraits{
            .clone = sy::BuiltInCoherentTraits::makeClone<T>(),
            .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<T>(),
            .hash = sy::BuiltInCoherentTraits::makeHashFunction<T>(),
            .compare = sy::BuiltInCoherentTraits::makeCompareFunction<T>(),
            .elementWiseAtomicDestroy = &EMPTY_DESTRUCTOR,
            .elementWiseAtomicLoad = BUILTIN_COHERENT_2_BYTE_ATOMIC_CLONE,
            .elementWiseAtomicStore = BUILTIN_COHERENT_2_BYTE_ATOMIC_CLONE};
    } else if constexpr (sizeof(T) == 4) {
        return sy::BuiltInCoherentTraits{
            .clone = sy::BuiltInCoherentTraits::makeClone<T>(),
            .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<T>(),
            .hash = sy::BuiltInCoherentTraits::makeHashFunction<T>(),
            .compare = sy::BuiltInCoherentTraits::makeCompareFunction<T>(),
            .elementWiseAtomicDestroy = &EMPTY_DESTRUCTOR,
            .elementWiseAtomicLoad = BUILTIN_COHERENT_4_BYTE_ATOMIC_CLONE,
            .elementWiseAtomicStore = BUILTIN_COHERENT_4_BYTE_ATOMIC_CLONE};
    } else if constexpr (sizeof(T) == 8) {
        return sy::BuiltInCoherentTraits{
            .clone = sy::BuiltInCoherentTraits::makeClone<T>(),
            .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<T>(),
            .hash = sy::BuiltInCoherentTraits::makeHashFunction<T>(),
            .compare = sy::BuiltInCoherentTraits::makeCompareFunction<T>(),
            .elementWiseAtomicDestroy = &EMPTY_DESTRUCTOR,
            .elementWiseAtomicLoad = BUILTIN_COHERENT_8_BYTE_ATOMIC_CLONE,
            .elementWiseAtomicStore = BUILTIN_COHERENT_8_BYTE_ATOMIC_CLONE};
    }
}

constexpr static BuiltInCoherentTraits BOOL_BUILTIN_TRAITS = makeBuiltinTraitsForPrimitive<bool>();

constexpr static TypeMetadata BOOL_TYPE_METADATA = {
    .typeSize = sizeof(bool),
    .typeAlign = alignof(bool),
    .name = sy::StringSlice("bool"),
    .extra = {.tag = TypeExtra::Tag::Bool, .info = {}},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &BOOL_BUILTIN_TRAITS};

constexpr static BuiltInCoherentTraits INT8_BUILTIN_TRAITS =
    makeBuiltinTraitsForPrimitive<int8_t>();

constexpr static TypeMetadata INT8_TYPE_METADATA = {
    .typeSize = sizeof(int8_t),
    .typeAlign = alignof(int8_t),
    .name = sy::StringSlice("i8"),
    .extra = {.tag = TypeExtra::Tag::Int,
              .info = TypeExtra::Info(TypeExtra::Int{.bits = 8, .isSigned = true})},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &INT8_BUILTIN_TRAITS};

constexpr static BuiltInCoherentTraits UINT8_BUILTIN_TRAITS =
    makeBuiltinTraitsForPrimitive<uint8_t>();

constexpr static TypeMetadata UINT8_TYPE_METADATA = {
    .typeSize = sizeof(uint8_t),
    .typeAlign = alignof(uint8_t),
    .name = sy::StringSlice("u8"),
    .extra = {.tag = TypeExtra::Tag::Int,
              .info = TypeExtra::Info(TypeExtra::Int{.bits = 8, .isSigned = false})},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &UINT8_BUILTIN_TRAITS};

constexpr static BuiltInCoherentTraits INT16_BUILTIN_TRAITS =
    makeBuiltinTraitsForPrimitive<int16_t>();

constexpr static TypeMetadata INT16_TYPE_METADATA = {
    .typeSize = sizeof(int16_t),
    .typeAlign = alignof(int16_t),
    .name = sy::StringSlice("i16"),
    .extra = {.tag = TypeExtra::Tag::Int,
              .info = TypeExtra::Info(TypeExtra::Int{.bits = 16, .isSigned = true})},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &INT16_BUILTIN_TRAITS};

constexpr static BuiltInCoherentTraits UINT16_BUILTIN_TRAITS =
    makeBuiltinTraitsForPrimitive<uint16_t>();

constexpr static TypeMetadata UINT16_TYPE_METADATA = {
    .typeSize = sizeof(uint16_t),
    .typeAlign = alignof(uint16_t),
    .name = sy::StringSlice("u16"),
    .extra = {.tag = TypeExtra::Tag::Int,
              .info = TypeExtra::Info(TypeExtra::Int{.bits = 16, .isSigned = false})},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &UINT16_BUILTIN_TRAITS};

constexpr static BuiltInCoherentTraits INT32_BUILTIN_TRAITS =
    makeBuiltinTraitsForPrimitive<int32_t>();

constexpr static TypeMetadata INT32_TYPE_METADATA = {
    .typeSize = sizeof(int32_t),
    .typeAlign = alignof(int32_t),
    .name = sy::StringSlice("i32"),
    .extra = {.tag = TypeExtra::Tag::Int,
              .info = TypeExtra::Info(TypeExtra::Int{.bits = 32, .isSigned = true})},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &INT32_BUILTIN_TRAITS};

constexpr static BuiltInCoherentTraits UINT32_BUILTIN_TRAITS =
    makeBuiltinTraitsForPrimitive<uint32_t>();

constexpr static TypeMetadata UINT32_TYPE_METADATA = {
    .typeSize = sizeof(uint32_t),
    .typeAlign = alignof(uint32_t),
    .name = sy::StringSlice("u32"),
    .extra = {.tag = TypeExtra::Tag::Int,
              .info = TypeExtra::Info(TypeExtra::Int{.bits = 32, .isSigned = false})},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &UINT32_BUILTIN_TRAITS};

constexpr static BuiltInCoherentTraits INT64_BUILTIN_TRAITS =
    makeBuiltinTraitsForPrimitive<int64_t>();

constexpr static TypeMetadata INT64_TYPE_METADATA = {
    .typeSize = sizeof(int64_t),
    .typeAlign = alignof(int64_t),
    .name = sy::StringSlice("i64"),
    .extra = {.tag = TypeExtra::Tag::Int,
              .info = TypeExtra::Info(TypeExtra::Int{.bits = 64, .isSigned = true})},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &INT64_BUILTIN_TRAITS};

constexpr static BuiltInCoherentTraits UINT64_BUILTIN_TRAITS =
    makeBuiltinTraitsForPrimitive<uint64_t>();

constexpr static TypeMetadata UINT64_TYPE_METADATA = {
    .typeSize = sizeof(uint64_t),
    .typeAlign = alignof(uint64_t),
    .name = sy::StringSlice("u64"),
    .extra = {.tag = TypeExtra::Tag::Int,
              .info = TypeExtra::Info(TypeExtra::Int{.bits = 64, .isSigned = false})},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &UINT64_BUILTIN_TRAITS};

constexpr static BuiltInCoherentTraits USIZE_BUILTIN_TRAITS =
    makeBuiltinTraitsForPrimitive<size_t>();

constexpr static TypeMetadata USIZE_TYPE_METADATA = {
    .typeSize = sizeof(size_t),
    .typeAlign = alignof(size_t),
    .name = sy::StringSlice("usize"),
    .extra = {.tag = TypeExtra::Tag::Int,
              .info =
                  TypeExtra::Info(TypeExtra::Int{.bits = sizeof(size_t) * 8, .isSigned = false})},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &USIZE_BUILTIN_TRAITS};

constexpr static BuiltInCoherentTraits F32_BUILTIN_TRAITS = makeBuiltinTraitsForPrimitive<float>();

constexpr static TypeMetadata F32_TYPE_METADATA = {
    .typeSize = sizeof(float),
    .typeAlign = alignof(float),
    .name = sy::StringSlice("f32"),
    .extra = {.tag = TypeExtra::Tag::Float, .info = TypeExtra::Info(TypeExtra::Float{.bits = 32})},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &F32_BUILTIN_TRAITS};

constexpr static BuiltInCoherentTraits F64_BUILTIN_TRAITS = makeBuiltinTraitsForPrimitive<double>();

constexpr static TypeMetadata F64_TYPE_METADATA = {
    .typeSize = sizeof(double),
    .typeAlign = alignof(double),
    .name = sy::StringSlice("f64"),
    .extra = {.tag = TypeExtra::Tag::Float, .info = TypeExtra::Info(TypeExtra::Float{.bits = 64})},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &F64_BUILTIN_TRAITS};

constexpr static BuiltInCoherentTraits ORDERING_BUILTIN_TRAITS =
    makeBuiltinTraitsForPrimitive<Ordering>();

constexpr static TypeMetadata ORDERING_TYPE_METADATA = {
    .typeSize = sizeof(Ordering),
    .typeAlign = alignof(Ordering),
    .name = sy::StringSlice("Ordering"),
    .extra = {.tag = TypeExtra::Tag::Ordering, .info = {}},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &ORDERING_BUILTIN_TRAITS};

constexpr static BuiltInCoherentTraits STRING_SLICE_BUILTIN_TRAITS = {
    .clone = sy::BuiltInCoherentTraits::makeClone<sy::StringSlice>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<sy::StringSlice>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<sy::StringSlice>(),
    .compare = {},
    .elementWiseAtomicDestroy = {},
    .elementWiseAtomicLoad = {},
    .elementWiseAtomicStore = {}};

constexpr static TypeMetadata STRING_SLICE_TYPE_METADATA = {
    .typeSize = sizeof(StringSlice),
    .typeAlign = alignof(StringSlice),
    .name = sy::StringSlice("str"),
    .extra = {.tag = TypeExtra::Tag::StringSlice, .info = {}},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &STRING_SLICE_BUILTIN_TRAITS};

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
    .clone = sy::BuiltInCoherentTraits::makeClone<sy::String>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<sy::String>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<sy::String>(),
    .compare = {}, // TODO
    .elementWiseAtomicDestroy = STRING_BUILTIN_COHERENT_ATOMIC_DESTROY,
    .elementWiseAtomicLoad = STRING_BUILTIN_COHERENT_ATOMIC_LOAD,
    .elementWiseAtomicStore = STRING_BUILTIN_COHERENT_ATOMIC_STORE,
};

constexpr static sy::Function<void(void* self)> STRING_DESTRUCTOR_FN = +[](void* self) {
    sy::String* tSelf = reinterpret_cast<sy::String*>(self);
    tSelf->~String();
};

constexpr static TypeMetadata STRING_TYPE_METADATA = {
    .typeSize = sizeof(String),
    .typeAlign = alignof(String),
    .name = sy::StringSlice("String"),
    .extra = {.tag = TypeExtra::Tag::String, .info = {}},
    .destructor = &STRING_DESTRUCTOR_FN,
    .builtinTraits = &STRING_BUILTIN_COHERENT_TRAITS};

constexpr static BuiltInCoherentTraits OPAQUE_PTR_BUILTIN_TRAITS =
    makeBuiltinTraitsForPrimitive<void*>();

constexpr static TypeMetadata OPAQUE_PTR_TYPE_METADATA = {
    .typeSize = sizeof(void*),
    .typeAlign = alignof(void*),
    .name = sy::StringSlice("opaque"), // TODO what name?
    .extra = {.tag = TypeExtra::Tag::OpaquePointer, .info = {}},
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &OPAQUE_PTR_BUILTIN_TRAITS};
} // namespace

// =======
// Exports
// =======

namespace sy {
namespace internal {
SY_API constexpr Type TYPE_BOOL = {.base = &BOOL_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_I8 = {.base = &INT8_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_U8 = {.base = &UINT8_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_I16 = {.base = &INT16_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_U16 = {
    .base = &UINT16_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_I32 = {.base = &INT32_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_U32 = {
    .base = &UINT32_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_I64 = {.base = &INT64_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_U64 = {
    .base = &UINT64_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_USIZE = {
    .base = &USIZE_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_F32 = {.base = &F32_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_F64 = {.base = &F64_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_ORDERING = {
    .base = &ORDERING_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_STRING_SLICE = {
    .base = &STRING_SLICE_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_STRING = {
    .base = &STRING_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
SY_API constexpr Type TYPE_OPAQUE_PTR = {
    .base = &OPAQUE_PTR_TYPE_METADATA, .indirection = 0, .mutableBits = 0};
} // namespace internal
} // namespace sy

extern "C" {
SY_API extern const Type SY_TYPE_BOOL = sy::internal::TYPE_BOOL;
SY_API extern const Type SY_TYPE_I8 = sy::internal::TYPE_I8;
SY_API extern const Type SY_TYPE_U8 = sy::internal::TYPE_U8;
SY_API extern const Type SY_TYPE_I16 = sy::internal::TYPE_I16;
SY_API extern const Type SY_TYPE_U16 = sy::internal::TYPE_U16;
SY_API extern const Type SY_TYPE_I32 = sy::internal::TYPE_I32;
SY_API extern const Type SY_TYPE_U32 = sy::internal::TYPE_U32;
SY_API extern const Type SY_TYPE_I64 = sy::internal::TYPE_I64;
SY_API extern const Type SY_TYPE_U64 = sy::internal::TYPE_U64;
SY_API extern const Type SY_TYPE_USIZE = sy::internal::TYPE_USIZE;
SY_API extern const Type SY_TYPE_F32 = sy::internal::TYPE_F32;
SY_API extern const Type SY_TYPE_F64 = sy::internal::TYPE_F64;
SY_API extern const Type SY_TYPE_ORDERING = sy::internal::TYPE_ORDERING;
SY_API extern const Type SY_TYPE_STRING_SLICE = sy::internal::TYPE_STRING_SLICE;
SY_API extern const Type SY_TYPE_STRING = sy::internal::TYPE_STRING;
SY_API extern const Type SY_TYPE_OPAQUE_PTR = sy::internal::TYPE_OPAQUE_PTR;
}
