#pragma once
#include <Ty/StringView.h>
#include <optional>

namespace Util {

struct LineAndColumn {
    u32 line;
    u32 column;
};

constexpr std::optional<LineAndColumn> line_and_column_for(
    StringView source, u32 index)
{
    if (index >= source.size)
        return {};

    u32 line = 0;
    u32 column = 0;
    for (u32 i = 0; i < index; i++) {
        if (source[i] == '\n') {
            line++;
            column = 0;
        } else {
            column++;
        }
    }

    return LineAndColumn { line, column };
}

constexpr StringView fetch_line(StringView source, u32 line_to_find)
{
    u32 line = 0;
    u32 start = 0;
    while (start < source.size && line != line_to_find)
        if (source[start++] == '\n')
            line++;
    u32 end = start;
    while (end < source.size)
        if (source[end++] == '\n')
            break;
    return source.sub_view(start, end - 1 - start);
}

}
