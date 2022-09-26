#pragma once
#include <Types.h>

#ifdef __cplusplus
namespace Core {
#endif

typedef struct Error {
    c_string M(message);
    c_string M(function);
    c_string M(file);
    u32 M(line_in_file);

#if __cplusplus
    static Error from_string_literal(c_string message,
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        u32 line_in_file = __builtin_LINE())
    {
        return { message, function, file, line_in_file };
    }

    static Error from_errno(int code = errno,
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        u32 line_in_file = __builtin_LINE())
    {
        return { errno_to_string(code), function, file, line_in_file };
    }

    static c_string errno_to_string(int);

    StringView message() const { return m_message; }

    StringView function() const { return m_function; }

    StringView file() const { return m_file; }

    u32 line_in_file() const { return m_line_in_file; }

    void show() const asm("Error$show");

private:
    constexpr Error(c_string message, c_string function,
        c_string file, u32 line_in_file)
        : m_message(message)
        , m_function(function)
        , m_file(file)
        , m_line_in_file(line_in_file)
    {
    }
#endif
} Error;

#ifndef __cplusplus
void Error$show(Error const*);
#endif

#ifdef __cplusplus
}
#endif
