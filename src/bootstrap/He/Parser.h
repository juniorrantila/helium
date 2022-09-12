#pragma once
#include <Core/ErrorOr.h>
#include <He/Expression.h>
#include <He/Token.h>
#include <SourceFile.h>

namespace He {

struct ParseError {
    constexpr ParseError(c_string message, c_string hint,
        Token offending_token,
        c_string parser_function = __builtin_FUNCTION())
        : message(message)
        , hint(hint)
        , parser_function(parser_function)
        , offending_token(offending_token)
    {
    }

    c_string message { nullptr };
    c_string hint { nullptr };
    c_string parser_function { nullptr };
    Token offending_token;

    Core::ErrorOr<void> show(SourceFile source) const;
};

using ParseResult = Core::ErrorOr<ParsedExpressions, ParseError>;
ParseResult parse(Tokens const& tokens);

}
