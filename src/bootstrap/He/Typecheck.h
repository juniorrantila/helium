#pragma once
#include <Core/ErrorOr.h>
#include <He/TypecheckedExpression.h>
#include <He/Context.h>
#include <He/Parser.h>
#include <string_view>

namespace He {

struct TypecheckError {
    StringView message {};
    u32 offending_expression_index { 0 };

    TypecheckError(Core::Error error)
        : message(error.message())
    {
    }

    Core::ErrorOr<void> show(Context const& context) const;
};

using TypecheckResult = Core::ErrorOr<TypecheckedExpressions, TypecheckError>;
TypecheckResult typecheck(Context& context);

}
