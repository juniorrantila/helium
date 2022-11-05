#pragma once
#include "Base.h"

typedef struct StringView {
    char const* data;
    u32 size;
} StringView;

static inline StringView StringView$from_c_string(c_string data)
{
    return (StringView) {
        .data = data,
        .size = (u32)__builtin_strlen(data),
    };
}
