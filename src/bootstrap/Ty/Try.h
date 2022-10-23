#pragma once

#define TRY(expr)                          \
    ({                                     \
        auto result = (expr);              \
        if (result.is_error())             \
            return result.release_error(); \
        result.release_value();            \
    })
