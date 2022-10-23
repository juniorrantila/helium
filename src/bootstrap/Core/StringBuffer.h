#pragma once
#include "ErrorOr.h"
#include "Try.h"
#include "Vector.h"
#include <Ty/Base.h>

namespace Core {

struct StringBuffer {
    static Core::ErrorOr<StringBuffer> create(u32 capacity)
    {
        auto* data = (char*)__builtin_malloc(capacity);
        if (!data)
            return Core::Error::from_errno();
        return StringBuffer {
            data,
            capacity,
        };
    }

    constexpr StringBuffer(StringBuffer&& other)
        : m_data(other.m_data)
        , m_size(other.m_size)
        , m_capacity(other.m_capacity)
    {
        other.invalidate();
    }

    ~StringBuffer()
    {
        if (is_valid()) {
            __builtin_free(m_data);
            invalidate();
        }
    }

    template <typename... Args>
    constexpr Core::ErrorOr<void> write(Args... args)
    {
        const StringView strings[] = {
            args...,
        };
        for (auto const string : strings)
            TRY(write(string));

        return {};
    }

    template <typename... Args>
    constexpr Core::ErrorOr<void> writeln(Args... args)
    {
        TRY(write(args..., "\n"sv));
        return {};
    }

    constexpr Core::ErrorOr<void> write(StringView string)
    {
        if (m_size + string.size >= m_capacity)
            return Core::Error::from_string_literal(
                "buffer filled");
        m_size += string.unchecked_copy_to(&m_data[m_size]);

        return {};
    }

    constexpr Core::ErrorOr<void> writeln(StringView string)
    {
        if (m_size + string.size >= m_capacity)
            return Core::Error::from_string_literal(
                "buffer filled");
        m_size += string.unchecked_copy_to(&m_data[m_size]);
        TRY(write("\n"sv));

        return {};
    }

    constexpr char const* data() const { return m_data; }
    constexpr u32 size() const { return m_size; }

private:
    constexpr StringBuffer(char* data, u32 capacity)
        : m_data(data)
        , m_size(0)
        , m_capacity(capacity)
    {
    }

    constexpr bool is_valid() const { return m_data != nullptr; }
    constexpr void invalidate() { m_data = nullptr; }

    char* m_data { nullptr };
    u32 m_size { 0 };
    u32 m_capacity { 0 };
};

}
