#pragma once
#include <Types.h>
#include <string_view>
#include <vector>

namespace He {

enum class TokenType : u8 {
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

    CFn,
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

struct [[gnu::packed]] Token {
    constexpr Token(TokenType type, u32 start, u32 end)
        : start_index(start)
        , size(end - start)
        , type(type)
    {
    }

    constexpr Token() = default;

    void dump(std::string_view source) const;
    std::string_view text(std::string_view source) const
    {
        return source.substr(start_index, size);
    }

    constexpr void set_end_index(u32 index)
    {
        size = index - start_index;
    }
    constexpr u32 end_index() const { return start_index + size; }

    u32 start_index { 0 };
    u16 size { 0 };
    TokenType type { TokenType::Invalid };
};
using Tokens = std::vector<Token>;

}
