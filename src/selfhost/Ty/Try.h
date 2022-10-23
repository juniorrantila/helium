#pragma once

#define TRY(expr)                \
    ({                           \
        auto result = (expr);    \
        if (result.is_error)     \
            return result.error; \
        result.value;            \
    })
