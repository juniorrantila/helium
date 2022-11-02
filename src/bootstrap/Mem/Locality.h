#pragma once
#include <Ty/Base.h>
#include <Ty/View.h>

namespace Mem {

template <typename T>
constexpr void mark_read_once(T const* value)
{
    constexpr auto const read_only = 0;
    constexpr auto const low_locality = 0;
    __builtin_prefetch(value, read_only, low_locality);
}
template <>
constexpr void mark_read_once(void const*) = delete;

template <typename T>
constexpr void mark_values_read_once(T const* values, u32 size)
{
    for (u32 i = 0; i < size; i++)
        mark_read_once(values + i);
}
template <>
constexpr void mark_values_read_once(void const*, u32) = delete;

}
