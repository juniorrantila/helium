#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

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

typedef struct StringView {
    char const* data;
    u32 size;

#if __cplusplus
    constexpr StringView(c_string string)
        : data(string)
        , size(__builtin_strlen(string))
    {
    }
#endif
} StringView;

#if __cplusplus
#    include <iostream>
#    include <string_view>
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
