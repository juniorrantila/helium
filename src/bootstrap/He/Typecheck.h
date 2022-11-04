#pragma once
#include "Context.h"
#include "Parser.h"
#include "TypecheckedExpression.h"
#include <Ty/ErrorOr.h>

namespace He {

struct TypecheckError {
    StringView message {};
    u32 offending_expression_index { 0 };

    TypecheckError(StringView message,
        u32 offending_expression_index)
        : message(message)
        , offending_expression_index(offending_expression_index)
    {
    }

    TypecheckError(Error error)
        : message(error.message())
    {
    }

    ErrorOr<void> show(Context const& context) const;
};

using TypecheckResult
    = ErrorOr<TypecheckedExpressions, TypecheckError>;
TypecheckResult typecheck(Context& context);

}
