#include "Typecheck.h"
#include "Context.h"
#include "Expression.h"
#include "Parser.h"
#include "SourceFile.h"
#include "TypecheckedExpression.h"
#include <Ty/ErrorOr.h>

namespace He {

using TypecheckSingleItemResult
    = ErrorOr<CheckedExpression, TypecheckError>;

#define FORWARD_DECLARE_TYPECHECKER(name)                    \
    [[maybe_unused]] static TypecheckSingleItemResult        \
        typecheck_##name(                                    \
            TypecheckedExpressions& typechecked_expressions, \
            ParsedExpressions const& parsed_expressions,     \
            u32 start)

#define X(T, name, ...) FORWARD_DECLARE_TYPECHECKER(name);
CHECKED_EXPRESSIONS
#undef X

FORWARD_DECLARE_TYPECHECKER(expression);

#undef FORWARD_DECLARE_TYPECHECKER

TypecheckResult typecheck(Context&)
{
    auto output = TRY(TypecheckedExpressions::create());

#if 0
    for (u32 i = 0; i < expressions.expressions.size(); i++)
        TRY(typecheck_expression(output, expressions, i));
#endif

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
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_literal(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult
typecheck_private_constant_declaration(TypecheckedExpressions&,
    ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult
typecheck_private_variable_declaration(TypecheckedExpressions&,
    ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult
typecheck_public_constant_declaration(TypecheckedExpressions&,
    ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult
typecheck_public_variable_declaration(TypecheckedExpressions&,
    ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_variable_assignment(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_struct_declaration(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_struct_initializer(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_member_access(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_array_access(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_enum_declaration(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_union_declaration(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_variant_declaration(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_lvalue(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_rvalue(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_if_statement(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_return_statement(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_while_statement(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_block(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_function_call(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_private_function(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_public_c_function(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_public_function(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_import_c(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_inline_c(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_moved_value(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

static TypecheckSingleItemResult typecheck_invalid(
    TypecheckedExpressions&, ParsedExpressions const&, u32)
{
    return Error::from_string_literal("unimplemented");
}

[[deprecated("can't typecheck invalid")]] //
[[maybe_unused]] static TypecheckSingleItemResult
parse_invalid(TypecheckedExpressions&, ParsedExpressions const&,
    u32 index)
{
    return TypecheckError {
        "trying to typecheck invalid"sv,
        index,
    };
}

[[deprecated("can't parse moved value")]] //
[[maybe_unused]] static TypecheckSingleItemResult
parse_moved_value(TypecheckedExpressions&, ParsedExpressions const&,
    u32 start)
{
    return TypecheckError {
        "trying to typecheck moved"sv,
        start,
    };
}

ErrorOr<void> TypecheckError::show(Context const&) const
{
    return Error::from_string_literal("unimplemented");
}

}
