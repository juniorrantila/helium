#include "Token.h"
#include "Util.h"
#include <iostream>

namespace He {

static StringView token_type_string(TokenType type);

void Token::dump(StringView source_view) const
{
    auto source = std::string_view(source_view.data, source_view.size);
    auto text = source.substr(start_index, size);
    auto start = *Util::line_and_column_for(source, start_index);
    start.line += 1;
    start.column += 1;
    auto end = *Util::line_and_column_for(source, end_index());
    end.line += 1;
    end.column += 1;
    std::cerr << "Token"sv << '[';
    auto old_width = std::cerr.width(12);
    std::cerr << token_type_string(type);
    std::cerr << ' ';
    std::cerr << '\'' << (text == "\n"sv ? "\\n"sv : text) << '\'';

    std::cerr << ']' << '\n';
    std::cerr.width(old_width);
}

static StringView token_type_string(TokenType type)
{
    switch (type) {
#define CASE_RETURN(variant) \
    case TokenType::variant: return #variant##sv
        CASE_RETURN(OpenBracket);
        CASE_RETURN(CloseBracket);

        CASE_RETURN(OpenParen);
        CASE_RETURN(CloseParen);

        CASE_RETURN(OpenCurly);
        CASE_RETURN(CloseCurly);

        CASE_RETURN(Ampersand);
        CASE_RETURN(Comma);
        CASE_RETURN(Assign);
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

        CASE_RETURN(Equals);
        CASE_RETURN(GreaterThan);
        CASE_RETURN(GreaterThanOrEqual);
        CASE_RETURN(LessThan);
        CASE_RETURN(LessThanOrEqual);

        CASE_RETURN(Dot);
        CASE_RETURN(Arrow);

        CASE_RETURN(Quoted);
        CASE_RETURN(Identifier);

        CASE_RETURN(CFn);
        CASE_RETURN(Fn);
        CASE_RETURN(If);
        CASE_RETURN(InlineC);
        CASE_RETURN(Let);
        CASE_RETURN(Pub);
        CASE_RETURN(RefMut);
        CASE_RETURN(Return);
        CASE_RETURN(Var);
        CASE_RETURN(While);

        CASE_RETURN(Enum);
        CASE_RETURN(Struct);
        CASE_RETURN(Union);
        CASE_RETURN(Variant);

        CASE_RETURN(Embed);
        CASE_RETURN(ImportC);
        CASE_RETURN(SizeOf);
        CASE_RETURN(Uninitialized);

        CASE_RETURN(Invalid);
#undef CASE_RETURN
    }
}

}
