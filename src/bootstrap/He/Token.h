#pragma once
#include <Ty/StringView.h>
#include <Ty/Vector.h>

namespace He {

#define TOKEN_TYPES                                \
    X(OpenBracket, open_bracket)                   \
    X(CloseBracket, close_bracket)                 \
                                                   \
    X(OpenParen, open_paren)                       \
    X(CloseParen, close_paren)                     \
                                                   \
    X(OpenCurly, open_curly)                       \
    X(CloseCurly, close_curly)                     \
                                                   \
    X(Ampersand, ampersand)                        \
    X(Comma, comma)                                \
    X(Assign, assign)                              \
    X(NewLine, new_line)                           \
    X(Number, number)                              \
    X(Colon, colon)                                \
    X(Semicolon, semicolon)                        \
    X(Space, space)                                \
    X(Hash, hash)                                  \
    X(Underscore, underscore)                      \
    X(QuestionMark, question_mark)                 \
                                                   \
    X(Minus, minus)                                \
    X(Plus, plus)                                  \
    X(Slash, slash)                                \
    X(Star, star)                                  \
                                                   \
    X(Equals, equals)                              \
    X(GreaterThan, greater_than)                   \
    X(GreaterThanOrEqual, greater_than_or_equal)   \
    X(LessThan, less_than)                         \
    X(LessThanOrEqual, less_than_or_equal)         \
                                                   \
    X(Dot, dot)                                    \
    X(Arrow, arrow)                                \
                                                   \
    X(Quoted, quoted)                              \
    X(Identifier, identifier)                      \
                                                   \
    /* Keywords */                                 \
                                                   \
    X(CFn, c_fn)                                   \
    X(Fn, fn)                                      \
    X(If, if_token)                                \
    X(InlineC, inline_c)                           \
    X(InvalidInlineC, invalid_inline_c)            \
    X(InlineCBlock, inline_c_block)                \
    X(InvalidInlineCBlock, invalid_inline_c_block) \
    X(Let, let_token)                              \
    X(Pub, pub)                                    \
    X(RefMut, ref_mut)                             \
    X(Return, return_token)                        \
    X(Var, var_token)                              \
    X(While, while_token)                          \
                                                   \
    X(Enum, enum_token)                            \
    X(Struct, struct_token)                        \
    X(Union, union_token)                          \
    X(Variant, variant)                            \
                                                   \
    /* Builtin functions */                        \
                                                   \
    X(Embed, embed)                                \
    X(Import, import_token)                        \
    X(ImportC, import_c)                           \
    X(SizeOf, size_of)                             \
    X(Uninitialized, uninitialized)                \
                                                   \
    /* Garbage */                                  \
    X(Invalid, invalid)

enum class TokenType : u8 {
#define X(name, ...) name,
    TOKEN_TYPES
#undef X
};

StringView token_type_string(TokenType type);

struct [[gnu::packed]] Token {
    constexpr Token(TokenType type, u32 start)
        : start_index(start)
        , type(type)
    {
    }

    constexpr Token() = default;

    void dump(StringView source) const;

    StringView text(StringView source) const
    {
        return { &source.data[start_index], size(source) };
    }

    u32 size(StringView source) const;
    u32 end_index(StringView source) const
    {
        return start_index + size(source);
    }

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

    u32 start_index : 24 { 0 };
    TokenType type { TokenType::Invalid };
};
using Tokens = Vector<Token>;

ErrorOr<void> dump_tokens(StringView source,
    View<Token const> tokens);

}
