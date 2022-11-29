#include "Lexer.h"
#include "Token.h"
#include "Util.h"
#include <Core/File.h>
#include <Mem/Locality.h>
#include <Ty/Error.h>
#include <Ty/StringBuffer.h>
#include <Ty/Try.h>

namespace He {

ErrorOr<void> LexError::show(SourceFile source) const
{
    auto maybe_line_and_column
        = Util::line_and_column_for(source.text, source_index);
    if (!maybe_line_and_column)
        return Error::from_string_literal("could not fetch line");

    auto line_number = maybe_line_and_column->line;
    auto column_number = maybe_line_and_column->column;
    auto line = Util::fetch_line(source.text, line_number);

    auto& out = Core::File::stderr();
    TRY(out.writeln("Lex error: "sv, message, " ["sv,
        source.file_name, ":"sv, line_number + 1, ":"sv,
        column_number + 1, "]\n"sv, line));
    for (u32 column = 0; column < column_number; column++)
        TRY(out.write(" "sv));
    TRY(out.writeln("^"sv));

    return {};
}

namespace {

struct [[gnu::packed]] FatToken {
    constexpr FatToken(TokenType type, u32 start, u32 end)
        : start_index(start)
        , end_index(end)
        , type(type)
    {
    }

    constexpr FatToken() = default;

    void dump(StringView source) const;

    StringView text(StringView source) const
    {
        return source.sub_view(start_index,
            end_index - start_index);
    }

    constexpr Token thin_token() const
    {
        return Token { type, start_index };
    }

    constexpr u32 size() const { return end_index - start_index; }

    u32 start_index { 0 };
    u32 end_index { 0 };
    TokenType type { TokenType::Invalid };
};

constexpr bool is_whitespace(char character);
constexpr bool is_number(char character);
constexpr bool is_letter(char character);
constexpr bool is_delimiter(char character);
constexpr bool is_identifier_character(char character);
constexpr FatToken lex_string(StringView source, u32 start);
constexpr FatToken lex_quoted(StringView source, u32 start);
constexpr FatToken lex_identifier(StringView source, u32 start);
constexpr FatToken lex_number(StringView source, u32 start);
constexpr FatToken lex_minus_or_arrow(StringView source, u32 start);
constexpr FatToken lex_less_or_less_than_equal(StringView source,
    u32 start);
constexpr FatToken lex_greater_or_greater_than_equal(
    StringView source, u32 start);
constexpr FatToken lex_assign_or_equals(StringView source,
    u32 start);
constexpr FatToken lex_ampersand_or_ref_mut(StringView source,
    u32 start);
constexpr FatToken lex_inline_c(StringView source, u32 start);

constexpr FatToken relex_inline_c(StringView source, u32 start);
constexpr FatToken relex_inline_c_block(StringView source,
    u32 start);

using LexItemResult = ErrorOr<FatToken, LexError>;
LexItemResult lex_single_item(StringView source, u32 start);

}

u32 relex_size(StringView source, Token token)
{
    switch (token.type) {
    case TokenType::OpenBracket: return "["sv.size;
    case TokenType::CloseBracket: return "]"sv.size;
    case TokenType::OpenParen: return "("sv.size;
    case TokenType::CloseParen: return ")"sv.size;
    case TokenType::OpenCurly: return "{"sv.size;
    case TokenType::CloseCurly: return "}"sv.size;
    case TokenType::Ampersand: return "&"sv.size;
    case TokenType::Comma: return ","sv.size;
    case TokenType::Assign: return "="sv.size;
    case TokenType::NewLine: return "\n"sv.size;
    case TokenType::Number:
        return lex_number(source, token.start_index).size();
    case TokenType::Colon: return ":"sv.size;
    case TokenType::Semicolon: return ";"sv.size;
    case TokenType::Space: return " "sv.size;
    case TokenType::Hash: return "#"sv.size;
    case TokenType::Underscore: return "_"sv.size;
    case TokenType::QuestionMark: return "?"sv.size;
    case TokenType::Minus: return "-"sv.size;
    case TokenType::Plus: return "+"sv.size;
    case TokenType::Slash: return "/"sv.size;
    case TokenType::Star: return "*"sv.size;
    case TokenType::Equals: return "=="sv.size;
    case TokenType::GreaterThan: return ">"sv.size;
    case TokenType::GreaterThanOrEqual: return ">="sv.size;
    case TokenType::LessThan: return "<"sv.size;
    case TokenType::LessThanOrEqual: return "<="sv.size;

    case TokenType::Dot: return "."sv.size;
    case TokenType::Arrow: return "->"sv.size;

    case TokenType::Quoted:
        return lex_quoted(source, token.start_index).size();
    case TokenType::Identifier:
        return lex_string(source, token.start_index).size();

    case TokenType::CFn: return "c_fn"sv.size;
    case TokenType::Fn: return "fn"sv.size;
    case TokenType::If: return "if"sv.size;
    case TokenType::InlineC:
    case TokenType::InvalidInlineC:
        return relex_inline_c(source, token.start_index).size();
    case TokenType::InlineCBlock:
    case TokenType::InvalidInlineCBlock:
        return relex_inline_c_block(source, token.start_index)
            .size();
    case TokenType::Let: return "let"sv.size;
    case TokenType::Pub: return "pub"sv.size;
    case TokenType::RefMut: return "&mut"sv.size;
    case TokenType::Return: return "return"sv.size;
    case TokenType::Throw: return "throw"sv.size;
    case TokenType::Var: return "var"sv.size;
    case TokenType::While: return "while"sv.size;

    case TokenType::Enum: return "enum"sv.size;
    case TokenType::Struct: return "struct"sv.size;
    case TokenType::Union: return "union"sv.size;
    case TokenType::Variant: return "variant"sv.size;

    case TokenType::Embed: return "embed"sv.size;
    case TokenType::Import: return "import"sv.size;
    case TokenType::ImportC: return "import_c"sv.size;
    case TokenType::SizeOf: return "size_of"sv.size;
    case TokenType::Uninitialized: return "uninitialized"sv.size;
    case TokenType::Invalid: return ""sv.size;
    }
}

LexResult lex(StringView source)
{
    auto tokens = TRY(Tokens::create());

    auto const guesstimated_size = source.size / 20;
    TRY(tokens.reserve(guesstimated_size));

    for (u32 start = 0; start < source.size;) {
        Mem::mark_read_once(&source[start]);
        auto character = source[start];
        if (is_whitespace(character)) {
            start++;
            continue;
        }
        Mem::mark_read_once(&source[start + 1]);
        if (source.sub_view(start, 2) == "//"sv) {
            for (; start < source.size; start++) {
                Mem::mark_read_once(&source[start]);
                if (source[start] == '\n')
                    break;
            }
            continue;
        }

        auto token = TRY(lex_single_item(source, start));
        TRY(tokens.append(token.thin_token()));
        start = token.end_index;
    }

    return tokens;
}

namespace {

LexItemResult lex_single_item(StringView source, u32 start)
{
    Mem::mark_read_once(&source[start]);
    auto character = source[start];

    if (character == '[')
        return FatToken { TokenType::OpenBracket, start,
            start + 1 };

    if (character == ']')
        return FatToken { TokenType::CloseBracket, start,
            start + 1 };

    if (character == '#')
        return FatToken { TokenType::Hash, start, start + 1 };

    if (character == '_')
        return FatToken { TokenType::Underscore, start, start + 1 };

    if (character == '/')
        return FatToken { TokenType::Slash, start, start + 1 };

    if (character == ',')
        return FatToken { TokenType::Comma, start, start + 1 };

    if (character == '(')
        return FatToken { TokenType::OpenParen, start, start + 1 };

    if (character == ')')
        return FatToken { TokenType::CloseParen, start, start + 1 };

    if (character == '{')
        return FatToken { TokenType::OpenCurly, start, start + 1 };

    if (character == '}')
        return FatToken { TokenType::CloseCurly, start, start + 1 };

    if (character == '+')
        return FatToken { TokenType::Plus, start, start + 1 };

    if (character == '*')
        return FatToken { TokenType::Star, start, start + 1 };

    if (character == '.')
        return FatToken { TokenType::Dot, start, start + 1 };

    if (character == ':')
        return FatToken { TokenType::Colon, start, start + 1 };

    if (character == ';')
        return FatToken { TokenType::Semicolon, start, start + 1 };

    if (character == '?')
        return FatToken { TokenType::QuestionMark, start,
            start + 1 };

    if (character == '=')
        return lex_assign_or_equals(source, start);

    if (character == '-')
        return lex_minus_or_arrow(source, start);

    if (character == '>')
        return lex_greater_or_greater_than_equal(source, start);

    if (character == '<')
        return lex_less_or_less_than_equal(source, start);

    if (is_number(character))
        return lex_number(source, start);

    if (character == '"')
        return lex_quoted(source, start);

    if (character == '\'')
        return lex_quoted(source, start);

    if (character == '@') {
        auto token = lex_string(source, start + 1);
        auto value = token.text(source);

        if (value == "embed"sv) {
            token.type = TokenType::Embed;
            return token;
        }

        if (value == "uninitialized"sv) {
            token.type = TokenType::Uninitialized;
            return token;
        }

        if (value == "import"sv) {
            token.type = TokenType::Import;
            return token;
        }

        if (value == "import_c"sv) {
            token.type = TokenType::ImportC;
            return token;
        }

        if (value == "size_of"sv) {
            token.type = TokenType::SizeOf;
            return token;
        }

        [[unlikely]] return LexError { "invalid builtin function"sv,
            start + 1 };
    }

    if (character == '&')
        return lex_ampersand_or_ref_mut(source, start);

    if (is_letter(character)) {
        auto token = lex_string(source, start);
        auto value = token.text(source);
        if (value == "c_fn"sv) {
            token.type = TokenType::CFn;
            return token;
        }
        if (value == "fn"sv) {
            token.type = TokenType::Fn;
            return token;
        }
        if (value == "pub"sv) {
            token.type = TokenType::Pub;
            return token;
        }
        if (value == "return"sv) {
            token.type = TokenType::Return;
            return token;
        }
        if (value == "throw"sv) {
            token.type = TokenType::Throw;
            return token;
        }
        if (value == "let"sv) {
            token.type = TokenType::Let;
            return token;
        }
        if (value == "var"sv) {
            token.type = TokenType::Var;
            return token;
        }
        if (value == "if"sv) {
            token.type = TokenType::If;
            return token;
        }
        if (value == "while"sv) {
            token.type = TokenType::While;
            return token;
        }
        if (value == "struct"sv) {
            token.type = TokenType::Struct;
            return token;
        }
        if (value == "union"sv) {
            token.type = TokenType::Union;
            return token;
        }
        if (value == "variant"sv) {
            token.type = TokenType::Variant;
            return token;
        }
        if (value == "enum"sv) {
            token.type = TokenType::Enum;
            return token;
        }
        if (value == "inline_c"sv)
            return lex_inline_c(source, start + token.size());
        return token;
    }

    if (is_identifier_character(character))
        return lex_identifier(source, start);

    return LexError { "unknown token"sv, start };
}

constexpr FatToken relex_inline_c_block(StringView source,
    u32 start)
{
    u32 end = start;
    u32 last_brace_index = end;
    for (i32 brace_level = 1; end < source.size; end++) {
        auto character = source[end];
        if (character == '{')
            brace_level++;
        if (character == '}') {
            last_brace_index = end;
            brace_level--;
        }
        if (brace_level == 0 && character == ';')
            break;
        if (brace_level < 0) {
            return {
                TokenType::InvalidInlineCBlock,
                start,
                last_brace_index,
            };
        }
    }
    return { TokenType::InlineCBlock, start, end - 1 };
}

constexpr FatToken relex_inline_c(StringView source, u32 start)
{
    auto end = start;
    auto last_brace_index = end;
    for (i32 brace_level = 0; end < source.size; end++) {
        auto character = source[end];
        if (character == '{')
            brace_level++;
        if (character == '}') {
            last_brace_index = end;
            brace_level--;
        }
        if (brace_level == 0 && character == ';')
            break;
        if (brace_level < 0) {
            return {
                TokenType::InvalidInlineC,
                start,
                last_brace_index,
            };
        }
    }

    return FatToken { TokenType::InlineC, start, end };
}

constexpr FatToken lex_inline_c_block(StringView source, u32 start)
{
    i32 brace_level = 1;
    u32 end = start;
    u32 ending_brace_index = end;
    for (; end < source.size; end++) {
        auto character = source[end];
        if (character == '{')
            brace_level++;
        if (character == '}') {
            brace_level--;
            if (brace_level == 0) {
                ending_brace_index = end;
            }
        }
        if (brace_level == 0 && character == ';')
            break;
        if (brace_level < 0) {
            // FIXME: Throw error:
            // TRY(errors.append_or_short({
            //     "suspicious curly brace",
            //     "did you forget a semicolon after inline_c?",
            //     token,
            // }));
            return {
                TokenType::InvalidInlineCBlock,
                start,
                end,
            };
        }
    }

    if (source[end] != ';') {
        return { TokenType::InvalidInlineCBlock, start, end };
    }

    return {
        TokenType::InlineCBlock,
        start,
        ending_brace_index - 1,
    };
}

constexpr FatToken lex_inline_c(StringView source, u32 start)
{
    while (is_whitespace(source[start]))
        start++;
    if (source[start] == '{')
        return lex_inline_c_block(source, start + 1);

    auto end = start;
    for (i32 brace_level = 0; end < source.size; end++) {
        auto character = source[end];
        if (character == '{')
            brace_level++;
        if (character == '}')
            brace_level--;
        if (brace_level == 0 && character == ';')
            break;
        if (brace_level < 0) {
            // FIXME: Throw error:
            // TRY(errors.append_or_short({
            //     "suspicious curly brace",
            //     "did you forget a semicolon after inline_c?",
            //     token,
            // }));
            return { TokenType::InvalidInlineC, start, end + 1 };
        }
    }

    if (source[end] != ';') {
        return { TokenType::InvalidInlineC, start, end };
    }

    return FatToken { TokenType::InlineC, start, end };
}

constexpr FatToken lex_minus_or_arrow(StringView source, u32 start)
{
    if (start + 1 > source.size)
        return { TokenType::Minus, start, start + 1 };
    Mem::mark_read_once(&source[start + 1]);
    char character = source[start + 1];
    if (character == '>')
        return { TokenType::Arrow, start, start + 2 };
    return { TokenType::Minus, start, start + 1 };
}

constexpr FatToken lex_ampersand_or_ref_mut(StringView source,
    u32 start)
{
    if (start + 1 >= source.size) {
        return { TokenType::Ampersand, start, start + 1 };
    }

    Mem::mark_values_read_once(&source[start + 1], 4);
    if (is_whitespace(source[start + 4])
        && source.sub_view(start + 1, 3) == "mut"sv) {
        return {
            TokenType::RefMut,
            start,
            start + 4,
        };
    }

    return { TokenType::Ampersand, start, start + 1 };
}

constexpr FatToken lex_less_or_less_than_equal(StringView source,
    u32 start)
{
    if (start + 1 < source.size) {
        Mem::mark_read_once(&source[start + 1]);
        if (source[start + 1] == '=')
            return { TokenType::LessThanOrEqual, start, start + 2 };
    }
    return { TokenType::LessThan, start, start + 1 };
}

constexpr FatToken lex_greater_or_greater_than_equal(
    StringView source, u32 start)
{
    if (start + 1 < source.size) {
        Mem::mark_read_once(&source[start + 1]);
        if (source[start + 1] == '=') {
            return {
                TokenType::GreaterThanOrEqual,
                start,
                start + 2,
            };
        }
    }
    return { TokenType::GreaterThan, start, start + 1 };
}

constexpr FatToken lex_assign_or_equals(StringView source,
    u32 start)
{
    if (start + 1 > source.size)
        return { TokenType::Assign, start, start + 1 };
    Mem::mark_read_once(&source[start + 1]);
    char character = source[start + 1];
    if (character == '=')
        return { TokenType::Equals, start, start + 2 };
    return { TokenType::Assign, start, start + 1 };
}

constexpr FatToken lex_string(StringView source, u32 start)
{
    const u32 size = source.size;
    u32 end = start;
    // clang-format off
    for (; end < size; end++) [[likely]] {
        Mem::mark_read_once(&source[end]);
        if (is_delimiter(source[end])) [[likely]] {
            break;
        }
    }
    // clang-format on
    return { TokenType::Identifier, start, end };
}

constexpr FatToken lex_quoted(StringView source, u32 start)
{
    auto quote = source[start];
    u32 end = start + 1;
    // clang-format off
    for (; end < source.size; end++) [[likely]] {
        Mem::mark_read_once(&source[end]);
        char character = source[end];
        if (character != quote)
            continue;
        break;
    }
    // clang-format on
    return { TokenType::Quoted, start, end + 1 };
}

constexpr FatToken lex_identifier(StringView source, u32 start)
{
    u32 end = start + 1;
    // clang-format off
    for (; end < source.size; end++) [[likely]] {
        Mem::mark_read_once(&source[end]);
        char character = source[end];
        if (is_letter(character))
            continue;
        if (is_number(character))
            continue;
        break;
    }
    // clang-format on
    return { TokenType::Identifier, start, end };
}

constexpr FatToken lex_number(StringView source, u32 start)
{
    u32 end = start;
    // clang-format off
    for (; end < source.size; end++) [[likely]] {
        Mem::mark_read_once(&source[end]);
        char character = source[end];
        if (character == '.')
            continue;
        if (!is_number(character))
            break;
    }
    // clang-format on
    return { TokenType::Number, start, end };
}

[[gnu::flatten]] constexpr bool is_delimiter(char character)
{
    if (is_whitespace(character))
        return true;
    switch (character) {
    case ',': return true;
    case '.': return true;
    case ':': return true;
    case ';': return true;
    case '=': return true;
    case '+': return true;
    case '-': return true;
    case '*': return true;
    case '/': return true;
    case '(': return true;
    case ')': return true;
    case '[': return true;
    case ']': return true;
    case '{': return true;
    case '}': return true;
    }
    return false;
}

constexpr bool is_identifier_character(char character)
{
    return is_letter(character) || character == '$';
}

constexpr bool is_whitespace(char character)
{
    switch (character) {
    case ' ': return true;
    case '\t': return true;
    case '\r': return true;
    case '\n': return true;
    }
    return false;
}

constexpr bool is_number(char character)
{
    switch (character) {
    case '0' ... '9': return true;
    default: return false;
    }
}

constexpr bool is_letter(char character)
{
    switch (character) {
    case 'A' ... 'Z': return true;
    case 'a' ... 'z': return true;
    default: return false;
    }
}

}

}
