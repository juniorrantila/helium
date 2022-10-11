#include <Core/ErrorOr.h>
#include <He/Context.h>
#include <He/Expression.h>
#include <He/Parser.h>
#include <He/Typecheck.h>
#include <He/TypecheckedExpression.h>
#include <SourceFile.h>

namespace He {

using TypecheckSingleItemResult
    = Core::ErrorOr<CheckedExpression, TypecheckError>;

#define FORWARD_DECLARE_TYPECHECKER(name)                \
    static TypecheckSingleItemResult typecheck_##name(   \
        TypecheckedExpressions& typechecked_expressions, \
        ParsedExpressions const& parsed_expressions, u32 start)

#define X(T, name, ...) FORWARD_DECLARE_TYPECHECKER(name);
CHECKED_EXPRESSIONS
#undef X

FORWARD_DECLARE_TYPECHECKER(expression);

#undef FORWARD_DECLARE_TYPECHECKER

TypecheckResult typecheck(Context& context)
{
    auto output = TRY(TypecheckedExpressions::create());
    auto& expressions = context.expressions;
    for (auto& expression : expressions.expressions) {
        if (expression.type() == ExpressionType::ImportC) {
            auto import_c_id = expression.release_as_import_c();
            auto import_c = expressions[import_c_id];
            TRY(output.import_c_quoted_filenames.append(
                import_c.filename));
        }

        if (expression.type() == ExpressionType::InlineC) {
            auto inline_c_id = expression.release_as_inline_c();
            auto inline_c = expressions[inline_c_id];
            TRY(output.inline_c_texts.append(inline_c.literal));
        }
    }

    TRY(output.enum_forwards.reserve(
        expressions.enum_declarations.size()));
    for (auto const& enum_ : expressions.enum_declarations) {
        auto name = enum_.name;
        output.enum_forwards.unchecked_append({ name });
    }

    TRY(output.struct_forwards.reserve(
        expressions.struct_declarations.size()));
    for (auto const& struct_ : expressions.struct_declarations) {
        auto name = struct_.name;
        output.struct_forwards.unchecked_append({ name });
    }

    TRY(output.union_forwards.reserve(
        expressions.union_declarations.size()));
    for (auto const& union_ : expressions.union_declarations) {
        auto name = union_.name;
        output.union_forwards.unchecked_append({ name });
    }

    TRY(output.variant_forwards.reserve(
        expressions.variant_declarations.size()));
    for (auto const& variant : expressions.variant_declarations) {
        auto name = variant.name;
        output.variant_forwards.unchecked_append({ name });
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

    for (u32 i = 0; i < expressions.expressions.size(); i++)
        TRY(typecheck_expression(output, expressions, i));

    return output;
}

static TypecheckSingleItemResult typecheck_expression(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return CheckedExpression {
        Id<CheckedInvalid>::invalid(),
        0,
        0,
    };
}

static TypecheckSingleItemResult typecheck_uninitialized(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_literal(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult
typecheck_private_constant_declaration(TypecheckedExpressions&,
    ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult
typecheck_private_variable_declaration(TypecheckedExpressions&,
    ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult
typecheck_public_constant_declaration(TypecheckedExpressions&,
    ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult
typecheck_public_variable_declaration(TypecheckedExpressions&,
    ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_variable_assignment(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_struct_declaration(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_struct_initializer(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_member_access(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_array_access(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_enum_declaration(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_union_declaration(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_variant_declaration(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_lvalue(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_rvalue(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_if_statement(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_return_statement(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_while_statement(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_block(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_function_call(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_private_c_function(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_private_function(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_public_c_function(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_public_function(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_import_c(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_inline_c(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_moved_value(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_invalid(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Core::Error::from_string_literal("unimplemented");
}

[[deprecated("can't typecheck invalid")]] //
[[maybe_unused]] static TypecheckSingleItemResult
parse_invalid(TypecheckedExpressions&, ParsedExpressions const&,
    u32 index)
{
    return TypecheckError {
        "trying to typecheck invalid",
        index,
    };
}

[[deprecated("can't parse moved value")]] //
[[maybe_unused]] static TypecheckSingleItemResult
parse_moved_value(TypecheckedExpressions&, ParsedExpressions const&,
    u32 start)
{
    return TypecheckError {
        "trying to typecheck moved",
        start,
    };
}

Core::ErrorOr<void> TypecheckError::show(Context const&) const
{
    return Core::Error::from_string_literal("unimplemented");
}

}
