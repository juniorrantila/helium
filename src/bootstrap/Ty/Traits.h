#pragma once

namespace Ty {

template <typename T>
inline constexpr bool is_trivial = __is_trivial(T);

template <typename T>
inline constexpr bool is_trivially_copyable
    = __is_trivially_copyable(T);

template <typename T>
inline constexpr bool is_trivially_destructible
    = __is_trivially_destructible(T);

template <typename T, typename U>
inline constexpr bool is_same = __is_same(T, U);

template <typename T>
inline constexpr bool is_const = __is_const(T);

template <typename T, typename... Args>
inline constexpr bool is_constructible
    = __is_constructible(T, Args...);

inline constexpr bool is_constant_evaluated()
{
    return __builtin_is_constant_evaluated();
}

}

using namespace Ty;
