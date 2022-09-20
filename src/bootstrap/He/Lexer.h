#pragma once
#include <Core/ErrorOr.h>
#include <He/Token.h>
#include <SourceFile.h>
#include <Types.h>
#include <string_view>
#include <vector>

namespace He {

struct LexError {
    StringView message {};
    u32 source_index { 0 };

    constexpr LexError(Core::Error error)
        : message(error.message())
    {
    }

    constexpr LexError(StringView message, u32 source_index)
        : message(message)
        , source_index(source_index)
    {
    }

    Core::ErrorOr<void> show(SourceFile source) const;
};

using LexResult = Core::ErrorOr<Tokens, LexError>;
LexResult lex(std::string_view source);

}
