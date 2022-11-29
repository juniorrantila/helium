#include "Token.h"
#include "Lexer.h"
#include "Util.h"
#include <Core/File.h>

namespace He {

void Token::dump(StringView source) const
{
    auto text = source.sub_view(start_index, size(source));
    auto start = *Util::line_and_column_for(source, start_index);
    start.line += 1;
    start.column += 1;
    auto end
        = *Util::line_and_column_for(source, end_index(source));
    end.line += 1;
    end.column += 1;
    auto& out = Core::File::stderr();
    out.write("Token ["sv).ignore();
    auto name = token_type_string(type);
    for (auto size = name.size; size < 12; size++)
        out.write(" "sv).ignore();
    out.write(token_type_string(type), " '"sv,
           text == "\n"sv ? "\\n"sv : text, "'"sv)
        .ignore();

    out.writeln("]"sv).ignore();
}

ErrorOr<void> dump_tokens(StringView source,
    View<Token const> tokens)
{
    for (u32 i = 0; i < tokens.size(); i++) {
        auto chars = TRY(Core::File::stderr().write(i, " "sv));
        for (; chars < 5; chars++)
            TRY(Core::File::stderr().write(" "sv));
        TRY(Core::File::stderr().flush());
        auto token = tokens[i];
        token.dump(source);
    }
    TRY(Core::File::stderr().flush());

    return {};
}

StringView token_type_string(TokenType type)
{
    switch (type) {
#define X(variant, ...) \
    case TokenType::variant: return #variant##sv;
        TOKEN_TYPES
#undef X
    }
}

u32 Token::size(StringView source) const
{
    return relex_size(source, *this);
}

}
