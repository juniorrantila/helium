#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#ifndef __cplusplus
#    include <stdbool.h>
#else
#    include <type_traits>
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

#ifdef __cplusplus
#    define M(name) m_##name
#else
#    define M(name) name
#endif

#ifdef __cplusplus
template <typename T>
struct View {
    constexpr View(T* data, size_t size)
        : m_data(data)
        , m_size(size)
    {
    }

    T& operator[](size_t index) { return m_data[index]; }

    T const& operator[](size_t index) const
    {
        return m_data[index];
    }

    constexpr T* begin() { return m_data; }
    constexpr T* end() { return &m_data[m_size]; }

    constexpr T const* begin() const { return m_data; }
    constexpr T const* end() const { return &m_data[m_size]; }

    constexpr size_t size() const { return m_size; }
    constexpr T* data() { return m_data; }
    constexpr T const* data() const { return m_data; }

private:
    T* m_data;
    size_t m_size;
};

template <typename T>
requires std::is_const_v<T>
struct View<T> {
    constexpr View(T const* data, size_t size)
        : m_data(data)
        , m_size(size)
    {
    }

    T const& operator[](size_t index) const
    {
        return m_data[index];
    }

    constexpr T const* begin() const { return m_data; }
    constexpr T const* end() const { return &m_data[m_size]; }

    constexpr size_t size() const { return m_size; }
    constexpr T const* data() const { return m_data; }

private:
    T* m_data;
    size_t m_size;
};
#endif

#ifdef __cplusplus
#    include <string_view>
#endif
typedef struct StringView {
    char const* data;
    u32 size;

#ifdef __cplusplus
    constexpr StringView(std::string_view other)
        : data(other.data())
        , size(other.size())
    {
    }

    constexpr StringView() = default;

    static constexpr StringView from_c_string(c_string data)
    {
        return StringView(data, __builtin_strlen(data));
    }

    constexpr StringView(char const* data, u32 size)
        : data(data)
        , size(size)
    {
    }

    constexpr bool operator == (StringView other) const
    {
        if (size != other.size)
            return false;
        if (data == other.data)
            return true;
        return __builtin_strncmp(data, other.data, size) == 0;
    }

    constexpr bool operator == (char const* other) const
    {
        return *this == StringView(other);
    }

    constexpr bool is_empty() const { return size == 0; }

    constexpr char const& operator[](u32 index) const { return data[index]; }
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

#ifdef __cplusplus
template <typename T>
struct Id;
#endif

typedef struct GenericId {
    u32 M(raw) : 24;
    u8 M(type_id);

#if __cplusplus
    template <typename T>
    constexpr GenericId(Id<T> id)
        : m_raw(id.raw())
        , m_type_id(sizeof(T))
    {
    }

    constexpr explicit GenericId(u32 raw_id, u8 type_id)
        : m_raw(raw_id)
        , m_type_id(type_id)
    {
    }

    constexpr u32 raw() const { return m_raw; }
    constexpr u8 type_id() const { return m_type_id; }

    static constexpr GenericId invalid()
    {
        return GenericId { 0xFFFFFF, 0 };
    }
    constexpr bool is_valid() const { return m_type_id != 0; }
#endif
} GenericId;
#ifndef __cplusplus
inline static bool GenericId$is_valid(GenericId id)
{
    return id.type_id != 0;
}
#endif

#ifdef __cplusplus
template <typename T>
struct Id {
    constexpr Id() = default;

    constexpr explicit Id(u32 raw_id)
        : m_raw(raw_id)
    {
    }

    constexpr Id(GenericId id)
        : m_raw(id.raw())
    {
        if (!id.is_valid()) {
            *this = invalid();
        } else if (id.type_id() != sizeof(T)) {
            __builtin_abort();
        }
    }

    constexpr u32 raw() const { return m_raw; }

    static constexpr Id<T> invalid() { return Id<T> { 0xFFFFFF }; }
    constexpr bool is_valid() const { return m_raw != 0xFFFFFF; }

private:
    u32 m_raw { 0 };
};
#endif
