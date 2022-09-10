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

struct ParsedExpressions {
#define SOA_MEMBER(T, name)                                      \
    constexpr T& operator[](Id<T> id) { return name[id.raw()]; } \
    constexpr T const& operator[](Id<T> id) const                \
    {                                                            \
        return name[id.raw()];                                   \
    }                                                            \
    constexpr Id<T> append(T&& value)                            \
    {                                                            \
        auto id = Id<T>(name.size());                            \
        name.push_back(value);                                   \
        return id;                                               \
    }                                                            \
    std::vector<T> name

    SOA_MEMBER(Literal, literals);

#undef SOA_MEMBER
    std::vector<Expression> expressions;
};

using ParseResult = Core::ErrorOr<ParsedExpressions, ParseError>;
ParseResult parse(Tokens const& tokens);

}
