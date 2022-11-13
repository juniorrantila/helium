#pragma once
#include "SourceFile.h"
#include "Token.h"
#include <Ty/ErrorOr.h>
#include <Ty/StringView.h>

namespace He {

struct LexError {
    StringView message {};
    u32 source_index { 0 };

    constexpr LexError(Error error)
        : message(error.message())
    {
    }

    constexpr LexError(StringView message, u32 source_index)
        : message(message)
        , source_index(source_index)
    {
    }

    ErrorOr<void> show(SourceFile source) const;
};

using LexResult = ErrorOr<Tokens, LexError>;
LexResult lex(StringView source);
u32 relex_size(StringView source, Token token);

}
