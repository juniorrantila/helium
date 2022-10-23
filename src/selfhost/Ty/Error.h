#pragma once
#include "Base.h"
#include "StringView.h"

extern int errno;

typedef struct Error {
    c_string message;
    c_string function;
    c_string file;
    u32 line_in_file;
} Error;

[[maybe_unused]] static inline Error Error$_from_string_literal(
    c_string message, c_string function, c_string file,
    u32 line_in_file)
{
    return Error {
        .message = message,
        .function = function,
        .file = file,
        .line_in_file = line_in_file,
    };
}
#define Error$from_string_literal(message)                      \
    Error$_from_string_literal(message, __FUNCTION__, __FILE__, \
        __LINE__)

c_string errno_to_string(int);
[[maybe_unused]] static inline Error Error$_from_errno(
    int code = errno, c_string function = __builtin_FUNCTION(),
    c_string file = __builtin_FILE(),
    u32 line_in_file = __builtin_LINE())
{
    return {
        errno_to_string(code),
        function,
        file,
        line_in_file,
    };
}
#define Error$from_errno() \
    Error$_from_errno(errno, __FUNCTION__, __FILE__, __LINE__)

void Error$show(Error);
