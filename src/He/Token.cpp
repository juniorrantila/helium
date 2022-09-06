#include <He/Token.h>
#include <Util.h>
#include <iostream>

namespace He {

static std::string_view token_type_string(TokenType type);

void Token::dump(std::string_view source) const
{
    auto text = source.substr(start_index, size);
    auto start = *Util::line_and_column_for(source, start_index);
    start.line += 1;
    start.column += 1;
    auto end = *Util::line_and_column_for(source, end_index());
    end.line += 1;
    end.column += 1;
    std::cerr << "Token" << '[';
    auto old_width = std::cerr.width(12);
    std::cerr << token_type_string(type);
#if 0
    std::cout.width(1);
    std::cout << '(';
    std::cout.width(3);
    std::cout << start.line;
    std::cout.width(1);
    std::cout << ' ';
    std::cout.width(2);
    std::cout << start.column;
    std::cout.width(1);
    std::cout << ' ';
    std::cout.width(3);
    std::cout << end.line;
    std::cout.width(1);
    std::cout << ' ';
    std::cout.width(2);
    std::cout << end.column;
    std::cout.width(1);
    std::cout << ')';
#endif
    std::cerr << ' ';
    std::cerr << '\'' << (text == "\n" ? "\\n" : text) << '\'';

    std::cerr << ']' << '\n';
    std::cerr.width(old_width);
    // std::cout << value << value;
}

static std::string_view token_type_string(TokenType type)
{
    switch (type) {
#define CASE_RETURN(variant) \
    case TokenType::variant: return #variant
        CASE_RETURN(OpenBracket);
        CASE_RETURN(CloseBracket);

        CASE_RETURN(OpenParen);
        CASE_RETURN(CloseParen);

        CASE_RETURN(OpenCurly);
        CASE_RETURN(CloseCurly);

        CASE_RETURN(Ampersand);
        CASE_RETURN(Comma);
        CASE_RETURN(Assign);
        CASE_RETURN(Equals);
        CASE_RETURN(NewLine);
        CASE_RETURN(Number);
        CASE_RETURN(Colon);
        CASE_RETURN(Semicolon);
        CASE_RETURN(Space);
        CASE_RETURN(Hash);
        CASE_RETURN(Underscore);
        CASE_RETURN(QuestionMark);

        CASE_RETURN(Minus);
        CASE_RETURN(Plus);
        CASE_RETURN(Slash);
        CASE_RETURN(Star);

        CASE_RETURN(LessThanOrEqual);
        CASE_RETURN(GreaterThan);

        CASE_RETURN(Dot);
        CASE_RETURN(Arrow);

        CASE_RETURN(Quoted);
        CASE_RETURN(Identifier);

        CASE_RETURN(Fn);
        CASE_RETURN(If);
        CASE_RETURN(InlineC);
        CASE_RETURN(Let);
        CASE_RETURN(Pub);
        CASE_RETURN(RefMut);
        CASE_RETURN(Return);
        CASE_RETURN(Struct);
        CASE_RETURN(Var);
        CASE_RETURN(While);

        CASE_RETURN(Embed);
        CASE_RETURN(ImportC);
        CASE_RETURN(SizeOf);
        CASE_RETURN(Uninitialized);

        CASE_RETURN(Invalid);
#undef CASE_RETURN
    }
}

}
