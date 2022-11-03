#pragma once
#include "Base.h"
#include "ErrorOr.h"
#include "Try.h"
#include "Vector.h"

void* he_malloc(size_t);
void he_free(void*);

namespace Ty {

struct StringBuffer {
    static ErrorOr<StringBuffer> create(u32 capacity)
    {
        auto* data = (char*)he_malloc(capacity);
        if (!data)
            return Error::from_errno();
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
            he_free(m_data);
            invalidate();
        }
    }

    template <typename... Args>
    constexpr ErrorOr<void> write(Args... args)
    {
        constexpr auto args_size = sizeof...(Args);
        ErrorOr<void> results[args_size] = {
            xwrite(args)...,
        };
        for (u32 i = 0; i < args_size; i++)
            TRY(results[i]);
        return {};
    }

    template <typename... Args>
    constexpr ErrorOr<void> writeln(Args... args)
    {
        TRY(write(args..., "\n"sv));
        return {};
    }

    constexpr ErrorOr<void> xwrite(StringView string)
    {
        if (m_size + string.size >= m_capacity)
            return Error::from_string_literal("buffer filled");
        m_size += string.unchecked_copy_to(&m_data[m_size]);

        return {};
    }

    constexpr ErrorOr<void> xwriteln(StringView string)
    {
        if (m_size + string.size >= m_capacity)
            return Error::from_string_literal("buffer filled");
        m_size += string.unchecked_copy_to(&m_data[m_size]);
        TRY(xwrite("\n"sv));

        return {};
    }

    constexpr ErrorOr<void> xwrite(u64 number)
    {
        char buffer[20];
        u32 buffer_start = max_chars_in_u64;

        while (number != 0) {
            buffer_start--;
            buffer[buffer_start] = number_to_character(number % 10);
            number /= 10;
        }

        u32 buffer_size = max_chars_in_u64 - buffer_start;

        TRY(xwrite({ &buffer[buffer_start], buffer_size }));

        return {};
    }

    constexpr char const* data() const { return m_data; }
    constexpr u32 size() const { return m_size; }

    constexpr StringView view() const { return { m_data, m_size }; }

private:
    static constexpr auto max_chars_in_u64 = 20;

    static constexpr char number_to_character(u8 number)
    {
        return (char)('0' + number);
    }

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
