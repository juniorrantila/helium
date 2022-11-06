#pragma once

#define TRY(expr)                              \
    ({                                         \
        decltype(auto) _result = (expr);       \
        if (_result.is_error()) [[unlikely]] { \
            return _result.release_error();    \
        }                                      \
        _result.release_value();               \
    })

#define MUST(expr)                             \
    ({                                         \
        decltype(auto) _result = (expr);       \
        if (_result.is_error()) [[unlikely]] { \
            __builtin_abort();                 \
        }                                      \
        _result.release_value();               \
    })
