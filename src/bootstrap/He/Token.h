#pragma once
#include <Ty/StringView.h>
#include <Ty/Vector.h>
#include <string_view>

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

    Equals,
    GreaterThan,
    GreaterThanOrEqual,
    LessThan,
    LessThanOrEqual,

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
    Var,
    While,

    Enum,
    Struct,
    Union,
    Variant,

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

    void dump(StringView source) const;

    StringView text(StringView source) const
    {
        return { &source.data[start_index], size };
    }

    constexpr void set_end_index(u32 index)
    {
        size = index - start_index;
    }
    constexpr u32 end_index() const { return start_index + size; }

    constexpr bool is(TokenType type) const
    {
        return this->type == type;
    }

    constexpr bool is_not(TokenType type) const
    {
        return this->type != type;
    }

    template <u32 size>
    constexpr bool is_any_of(TokenType const (&types)[size]) const
    {
        for (auto type : types) {
            if (is(type))
                return true;
        }
        return false;
    }

    u32 start_index { 0 };
    u16 size { 0 };
    TokenType type { TokenType::Invalid };
};
using Tokens = Vector<Token>;

}
