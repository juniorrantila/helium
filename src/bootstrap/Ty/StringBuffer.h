#pragma once
#include "Base.h"
#include "ErrorOr.h"
#include "Try.h"
#include "Vector.h"

void* he_malloc(size_t);
void he_free(void*);

namespace Ty {

struct StringBuffer {
    static ErrorOr<StringBuffer> create(
        u32 capacity = inline_capacity)
    {
        if (capacity > inline_capacity) {
            auto* data = (char*)he_malloc(capacity);
            if (!data)
                return Error::from_errno();
            return StringBuffer {
                data,
                capacity,
            };
        }

        return StringBuffer();
    }

    constexpr StringBuffer()
        : m_data(m_storage)
        , m_capacity(inline_capacity)
    {
    }

    constexpr StringBuffer(StringBuffer&& other)
        : m_data(other.m_data)
        , m_size(other.m_size)
        , m_capacity(other.m_capacity)
    {
        if (!other.is_saturated()) {
            __builtin_memcpy(m_storage, other.m_storage,
                inline_capacity);
            m_data = m_storage;
        }
        other.invalidate();
    }

    ~StringBuffer()
    {
        if (is_valid()) {
            if (is_saturated())
                he_free(m_data);
            invalidate();
        }
    }

    template <typename... Args>
    constexpr ErrorOr<u32> write(Args... args) requires(
        sizeof...(Args) > 1)
    {
        constexpr auto args_size = sizeof...(Args);
        ErrorOr<u32> results[args_size] = {
            write(args)...,
        };
        u32 written = 0;
        for (u32 i = 0; i < args_size; i++)
            written += TRY(results[i]);
        return written;
    }

    template <typename... Args>
    constexpr ErrorOr<u32> writeln(Args... args)
    {
        return TRY(write(args..., "\n"sv));
    }

    constexpr ErrorOr<u32> write(StringView string)
    {
        if (m_size + string.size >= m_capacity)
            return Error::from_string_literal("buffer filled");
        auto size = string.unchecked_copy_to(&m_data[m_size]);
        m_size += size;
        return size;
    }

    constexpr ErrorOr<u32> write(u64 number)
    {
        if (number == 0) {
            return TRY(write("0"sv));
        }

        char buffer[max_chars_in_u64];
        u32 buffer_start = max_chars_in_u64;

        while (number != 0) {
            buffer_start--;
            buffer[buffer_start] = number_to_character(number % 10);
            number /= 10;
        }

        u32 buffer_size = max_chars_in_u64 - buffer_start;

        return TRY(write({ &buffer[buffer_start], buffer_size }));
    }

    constexpr ErrorOr<u32> write(Error error)
    {
        auto size
            = TRY(write(error.function(), ": "sv, error.message()));
        size += TRY(writeln(" ["sv, error.file(), ":"sv,
            error.line_in_file(), "]"sv));

        return size;
    }

    constexpr void clear() { m_size = 0; }

    constexpr char const* data() const { return m_data; }
    constexpr u32 size() const { return m_size; }
    constexpr u32 capacity() const { return m_capacity; }
    constexpr u32 size_left() const { return m_capacity - m_size; }

    constexpr char* begin() { return m_data; }
    constexpr char* end() { return &m_data[m_size]; }

    constexpr c_string begin() const { return m_data; }
    constexpr c_string end() const { return &m_data[m_size]; }

    constexpr StringView view() const { return { m_data, m_size }; }

    constexpr void replace_all(char thing, char with)
    {
        for (auto& c : *this) {
            if (c == thing)
                c = with;
        }
    }

private:
    static constexpr auto max_chars_in_u64 = 20;
    static constexpr auto inline_capacity = 1024;

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

    constexpr bool is_saturated() const
    {
        return m_data != m_storage;
    }

    char m_storage[inline_capacity];
    char* m_data { nullptr };
    u32 m_size { 0 };
    u32 m_capacity { 0 };
};

}
