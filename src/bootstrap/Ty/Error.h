#pragma once
#include "Base.h"
#include "Hardware.h"
#include "Id.h"
#include "StaticVector.h"
#include "StringView.h"
#include "Traits.h"
#include "Try.h"
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

// More than 0x1000 errors on 0xFF cores seems a bit much.
using ErrorCodes
    = StaticVector<StaticVector<ErrorCodeData, 0x1000>, 0xFF>;

struct [[gnu::packed]] Error {
    struct InvalidToken { };
    static constexpr auto Invalid = InvalidToken();
    constexpr Error(InvalidToken) { }
    constexpr bool operator==(InvalidToken) const
    {
        return !code.is_valid();
    }

    ErrorCode code {};
    u8 thread_slot { 0 };

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
        return s_error_codes[thread_slot][code].message;
    }

    constexpr StringView function() const
    {
        return s_error_codes[thread_slot][code].function;
    }

    constexpr StringView file() const
    {
        return s_error_codes[thread_slot][code].file;
    }

    constexpr u32 line_in_file() const
    {
        return s_error_codes[thread_slot][code].line;
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
        thread_slot = Hardware::current_thread();
        code
            = MUST(s_error_codes[thread_slot].find_or_append(data));
    }

    static ErrorCodes s_error_codes;
};

}

using namespace Ty;
