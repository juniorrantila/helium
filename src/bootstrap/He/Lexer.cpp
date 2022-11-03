#include "Lexer.h"
#include "Token.h"
#include "Util.h"
#include <Mem/Locality.h>
#include <Ty/Error.h>
#include <Ty/Try.h>
#include <iostream>

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

    std::cerr << "Lex error: " << message << ' ' << '['
              << source.file_name << ':' << line_number + 1 << ':'
              << column_number + 1 << ']' << '\n';
    std::cerr << line << '\n';
    for (u32 column = 0; column < column_number; column++)
        std::cerr << ' ';
    std::cerr << '^' << '\n';

    return {};
}

namespace {

constexpr bool is_whitespace(char character);
constexpr bool is_number(char character);
constexpr bool is_letter(char character);
constexpr bool is_delimiter(char character);
constexpr bool is_identifier_character(char character);
constexpr Token lex_string(StringView source, u32 start);
constexpr Token lex_quoted(StringView source, u32 start);
constexpr Token lex_identifier(StringView source, u32 start);
constexpr Token lex_number(StringView source, u32 start);
constexpr Token lex_minus_or_arrow(StringView source, u32 start);
constexpr Token lex_less_or_less_than_equal(StringView source,
    u32 start);
constexpr Token lex_greater_or_greater_than_equal(StringView source,
    u32 start);
constexpr Token lex_assign_or_equals(StringView source, u32 start);
constexpr Token lex_ampersand_or_ref_mut(StringView source,
    u32 start);

using LexItemResult = ErrorOr<Token, LexError>;
LexItemResult lex_single_item(StringView source, u32 start);

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
        TRY(tokens.append(token));
        start = token.end_index();
    }

    return tokens;
}

namespace {

LexItemResult lex_single_item(StringView source, u32 start)
{
    Mem::mark_read_once(&source[start]);
    auto character = source[start];

    if (character == '[')
        return Token { TokenType::OpenBracket, start, start + 1 };

    if (character == ']')
        return Token { TokenType::CloseBracket, start, start + 1 };

    if (character == '#')
        return Token { TokenType::Hash, start, start + 1 };

    if (character == '_')
        return Token { TokenType::Underscore, start, start + 1 };

    if (character == '/')
        return Token { TokenType::Slash, start, start + 1 };

    if (character == ',')
        return Token { TokenType::Comma, start, start + 1 };

    if (character == '(')
        return Token { TokenType::OpenParen, start, start + 1 };

    if (character == ')')
        return Token { TokenType::CloseParen, start, start + 1 };

    if (character == '{')
        return Token { TokenType::OpenCurly, start, start + 1 };

    if (character == '}')
        return Token { TokenType::CloseCurly, start, start + 1 };

    if (character == '+')
        return Token { TokenType::Plus, start, start + 1 };

    if (character == '*')
        return Token { TokenType::Star, start, start + 1 };

    if (character == '.')
        return Token { TokenType::Dot, start, start + 1 };

    if (character == ':')
        return Token { TokenType::Colon, start, start + 1 };

    if (character == ';')
        return Token { TokenType::Semicolon, start, start + 1 };

    if (character == '?')
        return Token { TokenType::QuestionMark, start, start + 1 };

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
        if (value == "inline_c"sv) {
            token.type = TokenType::InlineC;
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
        return token;
    }

    if (is_identifier_character(character))
        return lex_identifier(source, start);

    return LexError { "unknown token"sv, start };
}

constexpr Token lex_minus_or_arrow(StringView source, u32 start)
{
    if (start + 1 > source.size)
        return { TokenType::Minus, start, start + 1 };
    Mem::mark_read_once(&source[start + 1]);
    char character = source[start + 1];
    if (character == '>')
        return { TokenType::Arrow, start, start + 2 };
    return { TokenType::Minus, start, start + 1 };
}

constexpr Token lex_ampersand_or_ref_mut(StringView source,
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

constexpr Token lex_less_or_less_than_equal(StringView source,
    u32 start)
{
    if (start + 1 < source.size) {
        Mem::mark_read_once(&source[start + 1]);
        if (source[start + 1] == '=')
            return { TokenType::LessThanOrEqual, start, start + 2 };
    }
    return { TokenType::LessThan, start, start + 1 };
}

constexpr Token lex_greater_or_greater_than_equal(StringView source,
    u32 start)
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

constexpr Token lex_assign_or_equals(StringView source, u32 start)
{
    if (start + 1 > source.size)
        return { TokenType::Assign, start, start + 1 };
    Mem::mark_read_once(&source[start + 1]);
    char character = source[start + 1];
    if (character == '=')
        return { TokenType::Equals, start, start + 2 };
    return { TokenType::Assign, start, start + 1 };
}

constexpr Token lex_string(StringView source, u32 start)
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

constexpr Token lex_quoted(StringView source, u32 start)
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

constexpr Token lex_identifier(StringView source, u32 start)
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

constexpr Token lex_number(StringView source, u32 start)
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
