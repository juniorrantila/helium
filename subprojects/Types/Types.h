#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t usize;
typedef ssize_t isize;

typedef float f32;
typedef double f64;

typedef char const* c_string;

#define var __auto_type
#define let __auto_type const

#if __cplusplus
#define M(name) m_##name
#else
#define M(name) name
#endif

#if __cplusplus
#    include <string_view>
#endif
typedef struct StringView {
    char const* data;
    u32 size;

#if __cplusplus
    constexpr StringView(std::string_view other)
        : data(other.data())
        , size(other.size())
    {
    }

    constexpr StringView() = default;

    constexpr StringView(c_string string)
        : data(string)
        , size(__builtin_strlen(string))
    {
    }

    constexpr StringView(c_string data, u32 size)
        : data(data)
        , size(size)
    {
    }
#endif
} StringView;

#if __cplusplus
#    include <iostream>
inline std::ostream& operator<<(std::ostream& os, StringView view)
{
    return std::operator<<(os,
        std::string_view(view.data, view.size));
}
#endif

#if __cplusplus
template <typename T>
struct Id {
    constexpr Id() = default;

    constexpr explicit Id(u32 raw_id)
        : m_raw(raw_id)
    {
    }

    constexpr u32 raw() const { return m_raw; }

private:
    u32 m_raw { 0 };
};
#endif
