#pragma once
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

typedef char const* c_string;

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
