#pragma once
#include "Array.h"
#include "Base.h"
#include "Id.h"
#include "StringView.h"
#include "Traits.h"
#include <errno.h>

namespace Ty {

struct ErrorCodeData {
    StringView message;
    StringView file;
    StringView function;
    u32 line;

    constexpr bool operator==(ErrorCodeData const& other) const
    {
        return message == other.message && file == other.file
            && function == other.function && line == other.line;
    }
};
using ErrorCode = SmallId<ErrorCodeData>;
using ErrorCodes = Array<ErrorCodeData, 0xFFFF>;

struct [[gnu::packed]] Error {
    struct InvalidToken { };
    static constexpr auto Invalid = InvalidToken();
    constexpr Error(InvalidToken) { }
    constexpr bool operator==(InvalidToken) const
    {
        return !code.is_valid();
    }

    ErrorCode code {};

    constexpr Error() = default;

    static Error from_string_literal(c_string message,
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        u16 line_in_file = __builtin_LINE())
    {
        return { message, function, file, line_in_file };
    }

    static constexpr Error from_errno(int code = errno,
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        u16 line_in_file = __builtin_LINE())
    {
        return {
            errno_to_string(code),
            function,
            file,
            line_in_file,
        };
    }

    static constexpr Error from_syscall(iptr rv,
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        u16 line_in_file = __builtin_LINE())
    {
        return {
            errno_to_string((i32)-rv),
            function,
            file,
            line_in_file,
        };
    }

    static c_string errno_to_string(int);

    constexpr StringView message() const
    {
        return s_error_codes[code].message;
    }

    constexpr StringView function() const
    {
        return s_error_codes[code].function;
    }

    constexpr StringView file() const
    {
        return s_error_codes[code].file;
    }

    constexpr u32 line_in_file() const
    {
        return s_error_codes[code].line;
    }

    constexpr bool is_empty() const
    {
        return code == ErrorCode::invalid();
    }

private:
    constexpr Error(c_string message, c_string function,
        c_string file, u16 line_in_file)
    {
        auto message_view = StringView::from_c_string(message);
        auto function_view = StringView::from_c_string(function);
        auto file_view = StringView::from_c_string(file);
        auto data = ErrorCodeData {
            .message = message_view,
            .file = file_view,
            .function = function_view,
            .line = line_in_file,
        };
        // Assume no fail.
        code = *s_error_codes.find_or_append(data);
    }

    // FIXME: Technically racy.
    static ErrorCodes s_error_codes;
};

}

using namespace Ty;
