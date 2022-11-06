#pragma once

namespace Ty {

using i8 = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = signed long;

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long;

static_assert(sizeof(i8) == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(i64) == 8);

static_assert(sizeof(u8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);

using usize = u64;
using isize = i64;

template <u8 bytes>
struct SignedTypeFromSize;

template <>
struct SignedTypeFromSize<1> {
    using Type = i8;
};

template <>
struct SignedTypeFromSize<2> {
    using Type = u16;
};

template <>
struct SignedTypeFromSize<4> {
    using Type = i32;
};

template <>
struct SignedTypeFromSize<8> {
    using Type = i64;
};

template <u8 bytes>
struct TypeFromSize;

template <>
struct TypeFromSize<1> {
    using Type = u8;
};

template <>
struct TypeFromSize<2> {
    using Type = u16;
};

template <>
struct TypeFromSize<4> {
    using Type = u32;
};

template <>
struct TypeFromSize<8> {
    using Type = u64;
};

template <u8 bytes>
using type_from_size = typename TypeFromSize<bytes>::Type;

template <u8 bytes>
using signed_type_from_size =
    typename SignedTypeFromSize<bytes>::Type;

using iptr = signed_type_from_size<sizeof(void*)>;
using uptr = type_from_size<sizeof(void*)>;

using f32 = float;
using f64 = double;

using c_string = char const*;

using nullptr_t = decltype(nullptr);

}

using namespace Ty;
