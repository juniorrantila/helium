#include <Core/Error.h>
#include <Core/Try.h>
#include <He/Lexer.h>
#include <He/Token.h>
#include <Util.h>
#include <iostream>

namespace He {

Core::ErrorOr<void> LexError::show(SourceFile source) const
{
    auto maybe_line_and_column
        = Util::line_and_column_for(source.text, source_index);
    if (!maybe_line_and_column)
        return Core::Error::from_string_literal(
            "could not fetch line");

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

static constexpr bool is_whitespace(char character);
static constexpr bool is_number(char character);
static constexpr bool is_letter(char character);
static constexpr bool is_delimiter(char character);
static constexpr bool is_identifier_character(char character);
static constexpr Token lex_string(std::string_view source,
    u32 start);
static constexpr Token lex_quoted(std::string_view source,
    u32 start);
static constexpr Token lex_identifier(std::string_view source,
    u32 start);
static constexpr Token lex_number(std::string_view source,
    u32 start);
static constexpr Token lex_minus_or_arrow(std::string_view source,
    u32 start);
static constexpr Token lex_less_or_less_than_equal(
    std::string_view source, u32 start);
static constexpr Token lex_assign_or_equals(std::string_view source,
    u32 start);
static constexpr Token lex_ampersand_or_ref_mut(
    std::string_view source, u32 start);

using LexItemResult = Core::ErrorOr<Token, LexError>;
static LexItemResult lex_single_item(std::string_view source,
    u32 start);

LexResult lex(std::string_view source)
{
    Tokens tokens;

    for (u32 start = 0; start < source.size();) {
        auto character = source[start];
        if (is_whitespace(character)) {
            start++;
            continue;
        }
        if (source.substr(start, 2) == "//") {
            for (; start < source.size(); start++)
                if (source[start] == '\n')
                    break;
            continue;
        }

        auto token = TRY(lex_single_item(source, start));
        tokens.push_back(token);
        start = token.end_index();
    }

    return tokens;
}

static LexItemResult lex_single_item(std::string_view source,
    u32 start)
{
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

    if (character == '>')
        return Token { TokenType::GreaterThan, start, start + 1 };

    if (character == '?')
        return Token { TokenType::QuestionMark, start, start + 1 };

    if (character == '=')
        return lex_assign_or_equals(source, start);

    if (character == '-')
        return lex_minus_or_arrow(source, start);

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

        if (value == "embed") {
            token.type = TokenType::Embed;
            return token;
        }

        if (value == "uninitialized") {
            token.type = TokenType::Uninitialized;
            return token;
        }

        if (value == "import_c") {
            token.type = TokenType::ImportC;
            return token;
        }

        if (value == "size_of") {
            token.type = TokenType::SizeOf;
            return token;
        }

        return LexError { "invalid builtin function", start + 1 };
    }

    if (character == '&')
        return lex_ampersand_or_ref_mut(source, start);

    if (is_letter(character)) {
        auto token = lex_string(source, start);
        auto value = token.text(source);
        if (value == "fn") {
            token.type = TokenType::Fn;
            return token;
        }
        if (value == "pub") {
            token.type = TokenType::Pub;
            return token;
        }
        if (value == "return") {
            token.type = TokenType::Return;
            return token;
        }
        if (value == "let") {
            token.type = TokenType::Let;
            return token;
        }
        if (value == "var") {
            token.type = TokenType::Var;
            return token;
        }
        if (value == "if") {
            token.type = TokenType::If;
            return token;
        }
        if (value == "inline_c") {
            token.type = TokenType::InlineC;
            return token;
        }
        if (value == "while") {
            token.type = TokenType::While;
            return token;
        }
        if (value == "struct") {
            token.type = TokenType::Struct;
            return token;
        }
        return token;
    }

    if (is_identifier_character(character))
        return lex_identifier(source, start);

    return LexError { "unknown token", start };
}

static constexpr Token lex_minus_or_arrow(std::string_view source, u32 start)
{
    if (start + 1 > source.size())
        return { TokenType::Minus, start, start + 1 };
    char character = source[start + 1];
    if (character == '>')
        return { TokenType::Arrow, start, start + 2 };
    return { TokenType::Minus, start, start + 1 };
}

static constexpr Token lex_ampersand_or_ref_mut(std::string_view source,
    u32 start)
{
    if (start + 1 < source.size()) {
        if (source.substr(start + 1, 3) == "mut")
            return { TokenType::RefMut, start, start + 4 };
    }
    return { TokenType::Ampersand, start, start + 1 };
}

static constexpr Token lex_less_or_less_than_equal(std::string_view source,
    u32 start)
{
    (void)source;
    return { TokenType::LessThanOrEqual, start, start + 2 };
}

static constexpr Token lex_assign_or_equals(std::string_view source,
    u32 start)
{
    if (start + 1 > source.size())
        return { TokenType::Assign, start, start + 1 };
    char character = source[start + 1];
    if (character == '=')
        return { TokenType::Equals, start, start + 2 };
    return { TokenType::Assign, start, start + 1 };
}

static constexpr Token lex_string(std::string_view source, u32 start)
{
    u32 end = start;
    for (; end < source.size(); end++) {
        if (!is_delimiter(source[end]))
            continue;
        break;
    }
    return { TokenType::Identifier, start, end };
}

static constexpr Token lex_quoted(std::string_view source, u32 start)
{
    auto quote = source[start];
    u32 end = start + 1;
    for (; end < source.size(); end++) {
        char character = source[end];
        if (character != quote)
            continue;
        break;
    }
    return { TokenType::Quoted, start, end + 1 };
}

static constexpr Token lex_identifier(std::string_view source, u32 start)
{
    u32 end = start + 1;
    for (; end < source.size(); end++) {
        char character = source[end];
        if (is_letter(character))
            continue;
        if (is_number(character))
            continue;
        break;
    }
    return { TokenType::Identifier, start, end };
}

static constexpr Token lex_number(std::string_view source, u32 start)
{
    u32 end = start;
    for (; end < source.size(); end++) {
        char character = source[end];
        if (character == '.')
            continue;
        if (!is_number(source[end]))
            break;
    }
    return { TokenType::Number, start, end };
}

static constexpr bool is_delimiter(char character)
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
    }
    return false;
}

static constexpr bool is_identifier_character(char character)
{
    return is_letter(character) || character == '$';
}

static constexpr bool is_whitespace(char character)
{
    switch (character) {
    case ' ': return true;
    case '\t': return true;
    case '\r': return true;
    case '\n': return true;
    }
    return false;
}

static constexpr bool is_number(char character)
{
    switch (character) {
    case '0' ... '9': return true;
    default: return false;
    }
}

static constexpr bool is_letter(char character)
{
    switch (character) {
    case 'A' ... 'Z': return true;
    case 'a' ... 'z': return true;
    default: return false;
    }
}

}
