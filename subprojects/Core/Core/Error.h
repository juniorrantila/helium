#pragma once
#include <Types.h>

namespace Core {

typedef struct Error {
#if __cplusplus
    static Error from_string_literal(
        c_string message,
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        u32 line_in_file = __builtin_LINE())
    {
        return { message, function, file, line_in_file };
    }

    StringView message() const { return m_message; }

    StringView function() const
    {
        return m_function;
    }

    StringView file() const { return m_file; }

    u32 line_in_file() const { return m_line_in_file; }

    void show() const;

private:
    constexpr Error(
        c_string message,
        c_string function,
        c_string file,
        u32 line_in_file)
        : m_message(message)
        , m_function(function)
        , m_file(file)
        , m_line_in_file(line_in_file)
    {
    }
#endif

    c_string m_message { nullptr };
    c_string m_function { nullptr };
    c_string m_file { nullptr };
    u32 m_line_in_file { 0 };
} Error;

}
