#pragma once
#include "Expression.h"
#include "SourceFile.h"
#include "Token.h"
#include "Ty/SmallVector.h"
#include <Ty/ErrorOr.h>

namespace He {

struct ParseError {
    constexpr ParseError(c_string message, c_string hint,
        Token offending_token,
        c_string parser_function = __builtin_FUNCTION(),
        c_string parser_file = __builtin_FILE(),
        u32 line_in_parser_file = __builtin_LINE())
        : message(message)
        , hint(hint)
        , parser_function(parser_function)
        , parser_file(parser_file)
        , offending_token(offending_token)
        , line_in_parser_file(line_in_parser_file)
    {
    }

    constexpr ParseError(Error error)
        : message(error.m_message)
        , parser_function(error.m_function)
        , parser_file(error.m_file)
        , offending_token(TokenType::Invalid, 0, 0)
        , line_in_parser_file(error.m_line_in_file)
    {
    }

    c_string message { nullptr };
    c_string hint { nullptr };
    c_string parser_function { nullptr };
    c_string parser_file { nullptr };
    Token offending_token;
    u32 line_in_parser_file { 0 };

    ErrorOr<void> show(SourceFile source) const;
};

struct ParseErrors {

    constexpr ParseErrors() = default;

    constexpr ParseErrors(Error error)
        : basic_error(error)
    {
    }

    constexpr ParseErrors(SmallVector<ParseError>&& errors)
        : parse_errors(std::move(errors))
    {
    }

    constexpr ParseErrors(ParseErrors&& other)
        : basic_error(other.basic_error)
        , parse_errors(std::move(other.parse_errors))
    {
    }

    constexpr bool has_error() const
    {
        return !parse_errors.is_empty() || !basic_error.is_empty();
    }

    ErrorOr<void, ParseErrors> append_or_short(ParseError error)
    {
        if (parse_errors.append(error).is_error())
            return std::move(*this);
        return {};
    }

    ErrorOr<void> show(SourceFile source) const;

    Error basic_error {};
    SmallVector<ParseError> parse_errors {};
};

using ParseResult = ErrorOr<ParsedExpressions, ParseErrors>;
ParseResult parse(Tokens const& tokens);

}
