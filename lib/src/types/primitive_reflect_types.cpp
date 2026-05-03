// separate translation unit to avoid the compiler type mismatch redefinition stuff,
// plus organization

#include "../core/builtin_traits/builtin_traits.hpp"
#include "function/function.hpp"
#include "option/option.hpp"
#include "ordering/ordering.hpp"
#include "reflect_fwd.hpp"
#include "string/string.hpp"
#include "string/string_internal.hpp"
#include "string/string_slice.hpp"
#include "type_info.hpp"

using sy::BuiltInCoherentTraits;
using sy::Function;
using sy::Type;

namespace sy {
namespace internal {
template <typename T> static void doAtomicCloneStd(void* dst, const void* src) {
    std::atomic<T>* atomicDst = reinterpret_cast<std::atomic<T>*>(dst);
    const std::atomic<T>* atomicSrc = reinterpret_cast<const std::atomic<T>*>(src);
    T temp = atomicSrc->load(std::memory_order_relaxed);
    atomicDst->store(temp, std::memory_order_relaxed);
}

constexpr static Function<void(void* dst, const void* src)>
    BUILTIN_COHERENT_1_BYTE_ATOMIC_CLONE_IMPL = doAtomicCloneStd<uint8_t>;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicLoad)
    BUILTIN_COHERENT_1_BYTE_ATOMIC_CLONE = &BUILTIN_COHERENT_1_BYTE_ATOMIC_CLONE_IMPL;
constexpr static Function<void(void* dst, const void* src)>
    BUILTIN_COHERENT_2_BYTE_ATOMIC_CLONE_IMPL = doAtomicCloneStd<uint16_t>;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicLoad)
    BUILTIN_COHERENT_2_BYTE_ATOMIC_CLONE = &BUILTIN_COHERENT_2_BYTE_ATOMIC_CLONE_IMPL;
constexpr static Function<void(void* dst, const void* src)>
    BUILTIN_COHERENT_4_BYTE_ATOMIC_CLONE_IMPL = doAtomicCloneStd<uint32_t>;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicLoad)
    BUILTIN_COHERENT_4_BYTE_ATOMIC_CLONE = &BUILTIN_COHERENT_4_BYTE_ATOMIC_CLONE_IMPL;
constexpr static Function<void(void* dst, const void* src)>
    BUILTIN_COHERENT_8_BYTE_ATOMIC_CLONE_IMPL = doAtomicCloneStd<uint64_t>;
constexpr static decltype(sy::BuiltInCoherentTraits::elementWiseAtomicLoad)
    BUILTIN_COHERENT_8_BYTE_ATOMIC_CLONE = &BUILTIN_COHERENT_8_BYTE_ATOMIC_CLONE_IMPL;

#pragma region Bool

constexpr static sy::BuiltInCoherentTraits BOOL_BUILTIN_TRAITS = {
    // conserve generated template functions where possible
    .clone = sy::BuiltInCoherentTraits::makeClone<uint8_t>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<uint8_t>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<bool>(),
    .compare = sy::BuiltInCoherentTraits::makeCompareFunction<bool>(),
    .elementWiseAtomicDestroy = &sy::internal::EMPTY_DESTRUCTOR,
    .elementWiseAtomicLoad = sy::internal::BUILTIN_COHERENT_1_BYTE_ATOMIC_CLONE,
    .elementWiseAtomicStore = sy::internal::BUILTIN_COHERENT_1_BYTE_ATOMIC_CLONE};

constinit sy::Type TYPE_BOOL_CONST_REF = {
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

constinit sy::Type TYPE_BOOL_MUT_REF = {
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

constinit sy::Type TYPE_BOOL = {
    .sizeType = sizeof(bool),
    .alignType = static_cast<uint16_t>(alignof(bool)),
    .name = "bool",
    .tag = Type::Tag::Bool,
    .extra = Type::ExtraInfo(),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &BOOL_BUILTIN_TRAITS,
    .constRef = &TYPE_BOOL_CONST_REF,
    .mutRef = &TYPE_BOOL_MUT_REF,
};

#pragma endregion

#pragma region Integers

constexpr static sy::BuiltInCoherentTraits U8_BUILTIN_TRAITS = {
    // conserve generated template functions where possible
    .clone = sy::BuiltInCoherentTraits::makeClone<uint8_t>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<uint8_t>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<uint8_t>(),
    .compare = sy::BuiltInCoherentTraits::makeCompareFunction<uint8_t>(),
    .elementWiseAtomicDestroy = &sy::internal::EMPTY_DESTRUCTOR,
    .elementWiseAtomicLoad = sy::internal::BUILTIN_COHERENT_1_BYTE_ATOMIC_CLONE,
    .elementWiseAtomicStore = sy::internal::BUILTIN_COHERENT_1_BYTE_ATOMIC_CLONE};

constinit sy::Type TYPE_U8_CONST_REF = {
    sizeof(const uint8_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const uint8_t*)),
    sy::StringSlice("*u8"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_U8}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_U8_MUT_REF = {
    sizeof(uint8_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(uint8_t*)),
    sy::StringSlice("*mut u8"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{true, &sy::internal::TYPE_U8}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_U8 = {
    .sizeType = sizeof(uint8_t),
    .alignType = static_cast<uint16_t>(alignof(uint8_t)),
    .name = "u8",
    .tag = Type::Tag::Int,
    .extra = Type::ExtraInfo(Type::ExtraInfo::Int{false, 8}),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &U8_BUILTIN_TRAITS,
    .constRef = &TYPE_U8_CONST_REF,
    .mutRef = &TYPE_U8_MUT_REF,
};

constexpr static sy::BuiltInCoherentTraits I8_BUILTIN_TRAITS = {
    // conserve generated template functions where possible
    .clone = sy::BuiltInCoherentTraits::makeClone<uint8_t>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<uint8_t>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<int8_t>(),
    .compare = sy::BuiltInCoherentTraits::makeCompareFunction<int8_t>(),
    .elementWiseAtomicDestroy = &sy::internal::EMPTY_DESTRUCTOR,
    .elementWiseAtomicLoad = sy::internal::BUILTIN_COHERENT_1_BYTE_ATOMIC_CLONE,
    .elementWiseAtomicStore = sy::internal::BUILTIN_COHERENT_1_BYTE_ATOMIC_CLONE};

constinit sy::Type TYPE_I8_CONST_REF = {
    sizeof(const int8_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const int8_t*)),
    sy::StringSlice("*i8"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_I8}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_I8_MUT_REF = {
    sizeof(int8_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(int8_t*)),
    sy::StringSlice("*mut i8"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{true, &sy::internal::TYPE_I8}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_I8 = {
    .sizeType = sizeof(int8_t),
    .alignType = static_cast<uint16_t>(alignof(int8_t)),
    .name = "i8",
    .tag = Type::Tag::Int,
    .extra = Type::ExtraInfo(Type::ExtraInfo::Int{true, 8}),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &I8_BUILTIN_TRAITS,
    .constRef = &TYPE_I8_CONST_REF,
    .mutRef = &TYPE_I8_MUT_REF,
};

constexpr static sy::BuiltInCoherentTraits U16_BUILTIN_TRAITS = {
    // conserve generated template functions where possible
    .clone = sy::BuiltInCoherentTraits::makeClone<uint16_t>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<uint16_t>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<uint16_t>(),
    .compare = sy::BuiltInCoherentTraits::makeCompareFunction<uint16_t>(),
    .elementWiseAtomicDestroy = &sy::internal::EMPTY_DESTRUCTOR,
    .elementWiseAtomicLoad = sy::internal::BUILTIN_COHERENT_2_BYTE_ATOMIC_CLONE,
    .elementWiseAtomicStore = sy::internal::BUILTIN_COHERENT_2_BYTE_ATOMIC_CLONE};

constinit sy::Type TYPE_U16_CONST_REF = {
    sizeof(const uint16_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const uint16_t*)),
    sy::StringSlice("*u16"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_U16}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_U16_MUT_REF = {
    sizeof(uint16_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(uint16_t*)),
    sy::StringSlice("*mut u16"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{true, &sy::internal::TYPE_U16}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_U16 = {
    .sizeType = sizeof(uint16_t),
    .alignType = static_cast<uint16_t>(alignof(uint16_t)),
    .name = "u16",
    .tag = Type::Tag::Int,
    .extra = Type::ExtraInfo(Type::ExtraInfo::Int{false, 16}),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &U16_BUILTIN_TRAITS,
    .constRef = &TYPE_U16_CONST_REF,
    .mutRef = &TYPE_U16_MUT_REF,
};

constexpr static sy::BuiltInCoherentTraits I16_BUILTIN_TRAITS = {
    // conserve generated template functions where possible
    .clone = sy::BuiltInCoherentTraits::makeClone<uint16_t>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<uint16_t>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<int16_t>(),
    .compare = sy::BuiltInCoherentTraits::makeCompareFunction<int16_t>(),
    .elementWiseAtomicDestroy = &sy::internal::EMPTY_DESTRUCTOR,
    .elementWiseAtomicLoad = sy::internal::BUILTIN_COHERENT_2_BYTE_ATOMIC_CLONE,
    .elementWiseAtomicStore = sy::internal::BUILTIN_COHERENT_2_BYTE_ATOMIC_CLONE};

constinit sy::Type TYPE_I16_CONST_REF = {
    sizeof(const int16_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const int16_t*)),
    sy::StringSlice("*i16"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_I16}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_I16_MUT_REF = {
    sizeof(int16_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(int16_t*)),
    sy::StringSlice("*mut i16"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{true, &sy::internal::TYPE_I16}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_I16 = {
    .sizeType = sizeof(int16_t),
    .alignType = static_cast<uint16_t>(alignof(int16_t)),
    .name = "i16",
    .tag = Type::Tag::Int,
    .extra = Type::ExtraInfo(Type::ExtraInfo::Int{true, 16}),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &I16_BUILTIN_TRAITS,
    .constRef = &TYPE_I16_CONST_REF,
    .mutRef = &TYPE_I16_MUT_REF,
};

constexpr static sy::BuiltInCoherentTraits U32_BUILTIN_TRAITS = {
    // conserve generated template functions where possible
    .clone = sy::BuiltInCoherentTraits::makeClone<uint32_t>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<uint32_t>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<uint32_t>(),
    .compare = sy::BuiltInCoherentTraits::makeCompareFunction<uint32_t>(),
    .elementWiseAtomicDestroy = &sy::internal::EMPTY_DESTRUCTOR,
    .elementWiseAtomicLoad = sy::internal::BUILTIN_COHERENT_4_BYTE_ATOMIC_CLONE,
    .elementWiseAtomicStore = sy::internal::BUILTIN_COHERENT_4_BYTE_ATOMIC_CLONE};

constinit sy::Type TYPE_U32_CONST_REF = {
    sizeof(const uint32_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const uint32_t*)),
    sy::StringSlice("*u32"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_U32}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_U32_MUT_REF = {
    sizeof(uint32_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(uint32_t*)),
    sy::StringSlice("*mut u32"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{true, &sy::internal::TYPE_U32}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_U32 = {
    .sizeType = sizeof(uint32_t),
    .alignType = static_cast<uint16_t>(alignof(uint32_t)),
    .name = "u32",
    .tag = Type::Tag::Int,
    .extra = Type::ExtraInfo(Type::ExtraInfo::Int{false, 32}),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &U32_BUILTIN_TRAITS,
    .constRef = &TYPE_U32_CONST_REF,
    .mutRef = &TYPE_U32_MUT_REF,
};

constexpr static sy::BuiltInCoherentTraits I32_BUILTIN_TRAITS = {
    // conserve generated template functions where possible
    .clone = sy::BuiltInCoherentTraits::makeClone<uint32_t>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<uint32_t>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<int32_t>(),
    .compare = sy::BuiltInCoherentTraits::makeCompareFunction<int32_t>(),
    .elementWiseAtomicDestroy = &sy::internal::EMPTY_DESTRUCTOR,
    .elementWiseAtomicLoad = sy::internal::BUILTIN_COHERENT_4_BYTE_ATOMIC_CLONE,
    .elementWiseAtomicStore = sy::internal::BUILTIN_COHERENT_4_BYTE_ATOMIC_CLONE};

constinit sy::Type TYPE_I32_CONST_REF = {
    sizeof(const int32_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const int32_t*)),
    sy::StringSlice("*i32"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_I32}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_I32_MUT_REF = {
    sizeof(int32_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(int32_t*)),
    sy::StringSlice("*mut i32"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{true, &sy::internal::TYPE_I32}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_I32 = {
    .sizeType = sizeof(int32_t),
    .alignType = static_cast<uint16_t>(alignof(int32_t)),
    .name = "i32",
    .tag = Type::Tag::Int,
    .extra = Type::ExtraInfo(Type::ExtraInfo::Int{true, 32}),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &I32_BUILTIN_TRAITS,
    .constRef = &TYPE_I32_CONST_REF,
    .mutRef = &TYPE_I32_MUT_REF,
};

constexpr static sy::BuiltInCoherentTraits U64_BUILTIN_TRAITS = {
    // conserve generated template functions where possible
    .clone = sy::BuiltInCoherentTraits::makeClone<uint64_t>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<uint64_t>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<uint64_t>(),
    .compare = sy::BuiltInCoherentTraits::makeCompareFunction<uint64_t>(),
    .elementWiseAtomicDestroy = &sy::internal::EMPTY_DESTRUCTOR,
    .elementWiseAtomicLoad = sy::internal::BUILTIN_COHERENT_8_BYTE_ATOMIC_CLONE,
    .elementWiseAtomicStore = sy::internal::BUILTIN_COHERENT_8_BYTE_ATOMIC_CLONE};

constinit sy::Type TYPE_U64_CONST_REF = {
    sizeof(const uint64_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const uint64_t*)),
    sy::StringSlice("*u64"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_U64}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_U64_MUT_REF = {
    sizeof(uint64_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(uint64_t*)),
    sy::StringSlice("*mut u64"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{true, &sy::internal::TYPE_U64}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_U64 = {
    .sizeType = sizeof(uint64_t),
    .alignType = static_cast<uint16_t>(alignof(uint64_t)),
    .name = "u64",
    .tag = Type::Tag::Int,
    .extra = Type::ExtraInfo(Type::ExtraInfo::Int{false, 64}),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &U64_BUILTIN_TRAITS,
    .constRef = &TYPE_U64_CONST_REF,
    .mutRef = &TYPE_U64_MUT_REF,
};

constexpr static sy::BuiltInCoherentTraits I64_BUILTIN_TRAITS = {
    // conserve generated template functions where possible
    .clone = sy::BuiltInCoherentTraits::makeClone<uint64_t>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<uint64_t>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<int64_t>(),
    .compare = sy::BuiltInCoherentTraits::makeCompareFunction<int64_t>(),
    .elementWiseAtomicDestroy = &sy::internal::EMPTY_DESTRUCTOR,
    .elementWiseAtomicLoad = sy::internal::BUILTIN_COHERENT_8_BYTE_ATOMIC_CLONE,
    .elementWiseAtomicStore = sy::internal::BUILTIN_COHERENT_8_BYTE_ATOMIC_CLONE};

constinit sy::Type TYPE_I64_CONST_REF = {
    sizeof(const int64_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const int64_t*)),
    sy::StringSlice("*i64"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_I64}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_I64_MUT_REF = {
    sizeof(int64_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(int64_t*)),
    sy::StringSlice("*mut i64"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{true, &sy::internal::TYPE_I64}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_I64 = {
    .sizeType = sizeof(int64_t),
    .alignType = static_cast<uint16_t>(alignof(int64_t)),
    .name = "i64",
    .tag = Type::Tag::Int,
    .extra = Type::ExtraInfo(Type::ExtraInfo::Int{true, 64}),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &I64_BUILTIN_TRAITS,
    .constRef = &TYPE_I64_CONST_REF,
    .mutRef = &TYPE_I64_MUT_REF,
};

constexpr static Function<void(void* dst, const void* src)> USIZE_ELEMENT_WISE_ATOMIC_LOAD_IMPL =
    doAtomicCloneStd<size_t>;
constexpr static Function<void(void* dst, const void* src)> USIZE_ELEMENT_WISE_ATOMIC_STORE_IMPL =
    doAtomicCloneStd<size_t>;

constexpr static sy::BuiltInCoherentTraits USIZE_BUILTIN_TRAITS = {
    .clone = sy::BuiltInCoherentTraits::makeClone<size_t>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<size_t>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<size_t>(),
    .compare = sy::BuiltInCoherentTraits::makeCompareFunction<size_t>(),
    .elementWiseAtomicDestroy = &sy::internal::EMPTY_DESTRUCTOR,
    .elementWiseAtomicLoad = &USIZE_ELEMENT_WISE_ATOMIC_LOAD_IMPL,
    .elementWiseAtomicStore = &USIZE_ELEMENT_WISE_ATOMIC_STORE_IMPL};

constinit sy::Type TYPE_USIZE_CONST_REF = {
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

constinit sy::Type TYPE_USIZE_MUT_REF = {
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

constinit sy::Type TYPE_USIZE = {
    .sizeType = sizeof(size_t),
    .alignType = static_cast<uint16_t>(alignof(size_t)),
    .name = "usize",
    .tag = Type::Tag::Int,
    .extra = Type::ExtraInfo(Type::ExtraInfo::Int{false, sizeof(size_t) * 8}),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &USIZE_BUILTIN_TRAITS,
    .constRef = &TYPE_USIZE_CONST_REF,
    .mutRef = &TYPE_USIZE_MUT_REF,
};

#pragma endregion

#pragma region Floats

constexpr static sy::BuiltInCoherentTraits F32_BUILTIN_TRAITS = {
    // conserve generated template functions where possible
    .clone = sy::BuiltInCoherentTraits::makeClone<float>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<float>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<float>(),
    .compare = sy::BuiltInCoherentTraits::makeCompareFunction<float>(),
    .elementWiseAtomicDestroy = &sy::internal::EMPTY_DESTRUCTOR,
    .elementWiseAtomicLoad = sy::internal::BUILTIN_COHERENT_4_BYTE_ATOMIC_CLONE,
    .elementWiseAtomicStore = sy::internal::BUILTIN_COHERENT_4_BYTE_ATOMIC_CLONE};

constinit sy::Type TYPE_F32_CONST_REF = {
    sizeof(const float*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const float*)),
    sy::StringSlice("*f32"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_F32}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_F32_MUT_REF = {
    sizeof(int64_t*),
    static_cast<decltype(sy::Type::alignType)>(alignof(int64_t*)),
    sy::StringSlice("*mut f32"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_F32}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_F32 = {
    .sizeType = sizeof(float),
    .alignType = static_cast<uint16_t>(alignof(float)),
    .name = "i64",
    .tag = Type::Tag::Float,
    .extra = sy::Type::ExtraInfo(sy::Type::ExtraInfo::Float{32}),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &F32_BUILTIN_TRAITS,
    .constRef = &TYPE_F32_CONST_REF,
    .mutRef = &TYPE_F32_MUT_REF,
};

constexpr static sy::BuiltInCoherentTraits F64_BUILTIN_TRAITS = {
    // conserve generated template functions where possible
    .clone = sy::BuiltInCoherentTraits::makeClone<double>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<double>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<double>(),
    .compare = sy::BuiltInCoherentTraits::makeCompareFunction<double>(),
    .elementWiseAtomicDestroy = &sy::internal::EMPTY_DESTRUCTOR,
    .elementWiseAtomicLoad = sy::internal::BUILTIN_COHERENT_8_BYTE_ATOMIC_CLONE,
    .elementWiseAtomicStore = sy::internal::BUILTIN_COHERENT_8_BYTE_ATOMIC_CLONE};

constinit sy::Type TYPE_F64_CONST_REF = {
    sizeof(const double*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const double*)),
    sy::StringSlice("*f64"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_F64}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_F64_MUT_REF = {
    sizeof(double*),
    static_cast<decltype(sy::Type::alignType)>(alignof(double*)),
    sy::StringSlice("*mut f64"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_F64}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_F64 = {
    .sizeType = sizeof(double),
    .alignType = static_cast<uint16_t>(alignof(double)),
    .name = "f64",
    .tag = Type::Tag::Float,
    .extra = sy::Type::ExtraInfo(sy::Type::ExtraInfo::Float{64}),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &F64_BUILTIN_TRAITS,
    .constRef = &TYPE_F64_CONST_REF,
    .mutRef = &TYPE_F64_MUT_REF,
};

#pragma endregion

#pragma region Ordering

constinit sy::Type TYPE_ORDERING_CONST_REF = {
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

constinit sy::Type TYPE_ORDERING_MUT_REF = {
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

constinit sy::Type TYPE_ORDERING = {
    .sizeType = sizeof(sy::Ordering),
    .alignType = static_cast<uint16_t>(alignof(sy::Ordering)),
    .name = "Ordering",
    .tag = Type::Tag::Ordering,
    .extra = Type::ExtraInfo(),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &I32_BUILTIN_TRAITS,
    .constRef = &TYPE_ORDERING_CONST_REF,
    .mutRef = &TYPE_ORDERING_MUT_REF,
};

#pragma endregion

#pragma region String

constexpr static sy::BuiltInCoherentTraits STRING_SLICE_BUILTIN_TRAITS = {
    // conserve generated template functions where possible
    .clone = sy::BuiltInCoherentTraits::makeClone<sy::StringSlice>(),
    .equal = sy::BuiltInCoherentTraits::makeEqualityFunction<sy::StringSlice>(),
    .hash = sy::BuiltInCoherentTraits::makeHashFunction<sy::StringSlice>(),
    .compare = {},
    .elementWiseAtomicDestroy = {},
    .elementWiseAtomicLoad = {},
    .elementWiseAtomicStore = {}};

constinit sy::Type TYPE_STRING_SLICE_CONST_REF = {
    sizeof(const sy::StringSlice*),
    static_cast<decltype(sy::Type::alignType)>(alignof(const sy::StringSlice*)),
    sy::StringSlice("*const str"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{false, &sy::internal::TYPE_STRING_SLICE}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_STRING_SLICE_MUT_REF = {
    sizeof(sy::StringSlice*),
    static_cast<decltype(sy::Type::alignType)>(alignof(sy::StringSlice*)),
    sy::StringSlice("*mut str"),
    sy::Type::Tag::Reference,
    sy::Type::ExtraInfo(sy::Type::ExtraInfo::Reference{true, &sy::internal::TYPE_STRING_SLICE}),
    &EMPTY_DESTRUCTOR,
    nullptr,
    nullptr,
    nullptr,
};

constinit sy::Type TYPE_STRING_SLICE = {
    .sizeType = sizeof(sy::StringSlice),
    .alignType = static_cast<uint16_t>(alignof(sy::StringSlice)),
    .name = "str",
    .tag = Type::Tag::StringSlice,
    .extra = Type::ExtraInfo(),
    .destructor = &EMPTY_DESTRUCTOR,
    .builtinTraits = &STRING_SLICE_BUILTIN_TRAITS,
    .constRef = &TYPE_STRING_SLICE_CONST_REF,
    .mutRef = &TYPE_STRING_SLICE_MUT_REF,
};

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
    .compare = {},
    .elementWiseAtomicDestroy = STRING_BUILTIN_COHERENT_ATOMIC_DESTROY,
    .elementWiseAtomicLoad = STRING_BUILTIN_COHERENT_ATOMIC_LOAD,
    .elementWiseAtomicStore = STRING_BUILTIN_COHERENT_ATOMIC_STORE,
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

constinit sy::Type TYPE_STRING_CONST_REF = {
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

constinit sy::Type TYPE_STRING_MUT_REF = {
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

constinit sy::Type TYPE_STRING = {
    sizeof(sy::String),
    static_cast<decltype(sy::Type::alignType)>(alignof(sy::String)),
    sy::StringSlice("String"),
    sy::Type::Tag::String,
    sy::Type::ExtraInfo(),
    &STRING_DESTRUCTOR_FN,
    &STRING_BUILTIN_COHERENT_TRAITS,
    &TYPE_STRING_CONST_REF,
    &TYPE_STRING_MUT_REF,
};

#pragma endregion

} // namespace internal
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

extern "C" {
SY_API constinit const sy::Type* SY_TYPE_BOOL = &sy::internal::TYPE_BOOL;

SY_API constinit const sy::Type* SY_TYPE_I8 = &sy::internal::TYPE_I8;
SY_API constinit const sy::Type* SY_TYPE_I16 = &sy::internal::TYPE_I16;
SY_API constinit const sy::Type* SY_TYPE_I32 = &sy::internal::TYPE_I32;
SY_API constinit const sy::Type* SY_TYPE_I64 = &sy::internal::TYPE_I64;
SY_API constinit const sy::Type* SY_TYPE_U8 = &sy::internal::TYPE_U8;
SY_API constinit const sy::Type* SY_TYPE_U16 = &sy::internal::TYPE_U16;
SY_API constinit const sy::Type* SY_TYPE_U32 = &sy::internal::TYPE_U32;
SY_API constinit const sy::Type* SY_TYPE_U64 = &sy::internal::TYPE_U64;
SY_API constinit const sy::Type* SY_TYPE_USIZE = &sy::internal::TYPE_USIZE;

SY_API constinit const sy::Type* SY_TYPE_F32 = &sy::internal::TYPE_F32;
SY_API constinit const sy::Type* SY_TYPE_F64 = &sy::internal::TYPE_F64;

// SY_API const SyType* SY_TYPE_CHAR           = reinterpret_cast<const SyType*>(Type::TYPE_CHAR);
SY_API constinit const sy::Type* SY_TYPE_STRING_SLICE = &sy::internal::TYPE_STRING_SLICE;
SY_API constinit const sy::Type* SY_TYPE_STRING = &sy::internal::TYPE_STRING;

SY_API constinit const sy::Type* SY_TYPE_ORDERING = &sy::internal::TYPE_ORDERING;
}
