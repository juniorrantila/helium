#pragma once

#define TRY(expr)                           \
    ({                                      \
        decltype(auto) _result = (expr);    \
        if (_result.is_error())             \
            return _result.release_error(); \
        _result.release_value();            \
    })
