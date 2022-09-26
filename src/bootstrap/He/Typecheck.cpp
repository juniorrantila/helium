#include <Core/ErrorOr.h>
#include <He/Context.h>
#include <He/Expression.h>
#include <He/Parser.h>
#include <He/Typecheck.h>
#include <He/TypecheckedExpression.h>
#include <SourceFile.h>

namespace He {

Core::ErrorOr<void> TypecheckError::show(Context const&) const
{
    return Core::Error::from_string_literal("unimplemented");
}

TypecheckResult typecheck(Context& context)
{
    auto output = TRY(TypecheckedExpressions::create());
    auto& expressions = context.expressions;
    for (auto& expression : expressions.expressions) {
        if (expression.type() == ExpressionType::ImportC) {
            auto import_c_id = expression.release_as_import_c();
            auto import_c = expressions[import_c_id];
            TRY(output.import_cs.append(import_c.filename));
        }

        if (expression.type() == ExpressionType::InlineC) {
            auto inline_c_id = expression.release_as_inline_c();
            auto inline_c = expressions[inline_c_id];
            TRY(output.inline_cs.append(inline_c.literal));
        }
    }

    TRY(output.struct_forwards.reserve(
        expressions.struct_declarations.size()));
    for (auto const& struct_ : expressions.struct_declarations) {
        auto name = struct_.name;
        output.struct_forwards.unchecked_append({ name });
    }

    TRY(output.private_function_forwards.reserve(
        expressions.private_functions.size()));
    for (auto const& function : expressions.private_functions) {
        output.private_function_forwards.unchecked_append({
            function.name,
            function.return_type,
            expressions[function.parameters],
        });
    }

    TRY(output.private_c_function_forwards.reserve(
        expressions.private_c_functions.size()));
    for (auto const& function : expressions.private_c_functions) {
        output.private_c_function_forwards.unchecked_append({
            function.name,
            function.return_type,
            expressions[function.parameters],
        });
    }

    TRY(output.public_function_forwards.reserve(
        expressions.public_functions.size()));
    for (auto const& function : expressions.public_functions) {
        output.public_function_forwards.unchecked_append({
            function.name,
            function.return_type,
            expressions[function.parameters],
        });
    }

    TRY(output.public_c_function_forwards.reserve(
        expressions.public_c_functions.size()));
    for (auto const& function : expressions.public_c_functions) {
        output.public_c_function_forwards.unchecked_append({
            function.name,
            function.return_type,
            expressions[function.parameters],
        });
    }
    return output;
}

}
