#pragma once
#include <Types.h>
#include <string_view>
#include <vector>

namespace He {

enum class TokenType {
    OpenBracket,
    CloseBracket,

    OpenParen,
    CloseParen,

    OpenCurly,
    CloseCurly,

    Ampersand,
    Comma,
    Assign,
    Equals,
    NewLine,
    Number,
    Colon,
    Semicolon,
    Space,
    Hash,
    Underscore,
    QuestionMark,

    Minus,
    Plus,
    Slash,
    Star,

    LessThanOrEqual,
    GreaterThan,

    Dot,
    Arrow,

    Quoted,
    Identifier,

    // Keywords

    Fn,
    If,
    InlineC,
    Let,
    Pub,
    RefMut,
    Return,
    Struct,
    Var,
    While,

    // Builtin functions
    Embed,
    ImportC,
    SizeOf,
    Uninitialized,

    // Garbage
    Invalid
};

struct Token {
    constexpr Token(TokenType type, u32 start, u32 end)
        : type(type)
        , start_index(start)
        , end_index(end)
    {
    }

    constexpr Token() = default;

    void dump(std::string_view source) const;
    std::string_view text(std::string_view source) const
    {
        return source.substr(start_index, end_index - start_index);
    }

    TokenType type { TokenType::Invalid };
    u32 start_index { 0 };
    u32 end_index { 0 };
};
using Tokens = std::vector<Token>;

}
