#include "Context.h"
#include "Expression.h"
#include "Mem/Sizes.h"
#include "Parser.h"
#include "SourceFile.h"
#include "Token.h"
#include "TypecheckedExpression.h"
#include <Core/File.h>
#include <Mem/Locality.h>
#include <Ty/Defer.h>
#include <Ty/StringBuffer.h>

namespace He {

namespace {

#define FORWARD_DECLARE_CODEGEN(T, name)                        \
    ErrorOr<void> codegen_##name(StringBuffer&, Context const&, \
        T const&)

#define X(T, name, ...) FORWARD_DECLARE_CODEGEN(T, name);
EXPRESSIONS
#undef X

FORWARD_DECLARE_CODEGEN(Expression, expression);
FORWARD_DECLARE_CODEGEN(Parameters, parameters);

#undef FORWARD_DECLARE_CODEGEN

ErrorOr<void> forward_declare_structures_short_spelling(
    StringBuffer& out, Context const&);

ErrorOr<void> forward_declare_functions_short_spelling(
    StringBuffer& out, Context const&);

ErrorOr<void> codegen_structures(StringBuffer& out, Context const&);

ErrorOr<void> codegen_top_level_variables(StringBuffer& out,
    Context const&);

ErrorOr<void> codegen_functions(StringBuffer& out, Context const&);

ErrorOr<void> codegen_prelude(StringBuffer& out);

ErrorOr<void> codegen_imports(StringBuffer& out, Context const&);

ErrorOr<void> codegen_top_level_inline_cs(StringBuffer& out,
    Context const&);

}

ErrorOr<StringBuffer> codegen_header(Context const&,
    TypecheckedExpressions const&)
{
    auto out = TRY(StringBuffer::create(64 * Mem::MiB));
    return out;
}

ErrorOr<StringBuffer> codegen(Context const& context,
    TypecheckedExpressions const&)
{
    auto out = TRY(StringBuffer::create(256 * Mem::MiB));

    TRY(codegen_prelude(out));
    TRY(codegen_imports(out, context));
    TRY(codegen_top_level_inline_cs(out, context));

    TRY(forward_declare_structures_short_spelling(out, context));
    TRY(forward_declare_functions_short_spelling(out, context));

    TRY(codegen_structures(out, context));
    TRY(codegen_top_level_variables(out, context));
    TRY(codegen_functions(out, context));

    return out;
}

namespace {

ErrorOr<void> codegen_imports(StringBuffer& out,
    Context const& context)
{
    auto const& expressions = context.expressions;

    for (auto const& import_c : expressions.import_cs) {
        Mem::mark_read_once(&import_c);
        TRY(codegen_import_c(out, context, import_c));
    }

    for (auto const& import_he : expressions.import_hes) {
        Mem::mark_read_once(&import_he);
        TRY(codegen_import_he(out, context, import_he));
    }

    return {};
}

ErrorOr<void> codegen_top_level_inline_cs(StringBuffer& out,
    Context const& context)
{
    for (auto inline_c : context.expressions.top_level_inline_cs) {
        TRY(out.writeln(inline_c.literal.text(context.source),
            ";"sv));
    }
    return {};
}

ErrorOr<void> codegen_top_level_variables(StringBuffer& out,
    Context const& context)
{
    auto const& expressions = context.expressions;

    auto const& public_constants
        = expressions.top_level_public_constants;
    for (auto const& constant : public_constants) {
        Mem::mark_read_once(&constant);
        TRY(codegen_public_constant_declaration(out, context,
            constant));
    }

    auto const& private_constants
        = expressions.top_level_private_constants;
    for (auto const& constant : private_constants) {
        Mem::mark_read_once(&constant);
        TRY(codegen_private_constant_declaration(out, context,
            constant));
    }

    auto const& public_variables
        = expressions.top_level_public_variables;
    for (auto const& variable : public_variables) {
        Mem::mark_read_once(&variable);
        TRY(codegen_public_variable_declaration(out, context,
            variable));
    }

    auto const& private_variables
        = expressions.top_level_private_variables;
    for (auto const& variable : private_variables) {
        Mem::mark_read_once(&variable);
        TRY(codegen_private_variable_declaration(out, context,
            variable));
    }

    return {};
}

ErrorOr<void> codegen_prelude(StringBuffer& out)
{
    auto prelude = R"php(<?php
declare(strict_types=1);
main();
function ensure_var($value) {
    return $value;
}

function ensure_let($value) {
    return $value;
}

function ensure_function(callable $value): callable {
    return $value;
}

function ensure_string(string $value): string {
    return $value;
}

function ensure_int(int $value): int {
    return $value;
}

function ensure_float(float $value): float {
    return $value;
}

function ensure_array(array $value): array {
    return $value;
}
)php"sv;
    TRY(out.write(prelude));

    return {};
}

ErrorOr<void> forward_declare_structures_short_spelling(
    StringBuffer&, Context const&)
{
    return {};
}

ErrorOr<void> forward_declare_functions_short_spelling(
    StringBuffer&, Context const&)
{
    return {};
}

ErrorOr<void> codegen_structures(StringBuffer& out,
    Context const& context)
{
    auto const& expressions = context.expressions;

    for (auto const& type : expressions.enum_declarations) {
        Mem::mark_read_once(&type);
        TRY(codegen_enum_declaration(out, context, type));
    }

    for (auto const& type : expressions.struct_declarations) {
        Mem::mark_read_once(&type);
        TRY(codegen_struct_declaration(out, context, type));
    }

    for (auto const& type : expressions.variant_declarations) {
        Mem::mark_read_once(&type);
        TRY(codegen_variant_declaration(out, context, type));
    }

    return {};
}

ErrorOr<void> codegen_functions(StringBuffer& out,
    Context const& context)
{
    auto const& expressions = context.expressions;

    auto const& public_functions = expressions.public_functions;
    for (auto const& function : public_functions) {
        Mem::mark_read_once(&function);
        TRY(codegen_public_function(out, context, function));
    }

    auto const& private_functions = expressions.private_functions;
    for (auto const& function : private_functions) {
        Mem::mark_read_once(&function);
        TRY(codegen_private_function(out, context, function));
    }

    auto const& public_c_functions = expressions.public_c_functions;
    for (auto const& function : public_c_functions) {
        Mem::mark_read_once(&function);
        TRY(codegen_public_c_function(out, context, function));
    }

    auto const& private_c_functions
        = expressions.private_c_functions;
    for (auto const& function : private_c_functions) {
        Mem::mark_read_once(&function);
        TRY(codegen_private_c_function(out, context, function));
    }

    return {};
}

ErrorOr<void> codegen_parameters(StringBuffer& out,
    Context const& context, Parameters const& parameters)
{
    auto source = context.source;
    if (parameters.is_empty()) {
        TRY(out.write("()"sv));
        return {};
    }
    TRY(out.write("("sv));
    auto last_parameter = parameters.size() - 1;
    for (u32 i = 0; i < last_parameter; i++) {
        auto parameter = parameters[i];
        TRY(out.write(parameter.type.text(source), " $"sv,
            parameter.name.text(source), ", "sv));
    }
    auto parameter = parameters[last_parameter];
    TRY(out.write(parameter.type.text(source), " $"sv,
        parameter.name.text(source)));
    TRY(out.write(")"sv));

    return {};
}

ErrorOr<void> codegen_member_access(StringBuffer& out,
    Context const& context, MemberAccess const& access)
{
    auto source = context.source;
    auto const& members = context.expressions[access.members];

    for (u32 i = 0; i < members.size() - 1; i++) {
        auto member = members[i];
        TRY(out.write("$"sv, member.text(source), "->"sv));
    }
    TRY(out.write(members[members.size() - 1].text(source)));

    return {};
}

ErrorOr<void> codegen_array_access(StringBuffer& out,
    Context const& context, ArrayAccess const& access)
{
    auto source = context.source;
    auto name = access.name.text(source);
    TRY(out.write("$"sv, name, "["sv));
    auto const& index = context.expressions[access.index];
    TRY(codegen_rvalue(out, context, index));
    TRY(out.write("]"sv));

    return {};
}

ErrorOr<void> codegen_moved_value(StringBuffer&, Context const&,
    Moved const&)
{
    return {};
}

ErrorOr<void> codegen_invalid(StringBuffer&, Context const&,
    Invalid const&)
{
    return Error::from_string_literal("trying to codegen invalid");
}

ErrorOr<void> codegen_expression(StringBuffer& out,
    Context const& context, Expression const& expression)
{
    auto const& expressions = context.expressions;
    auto const type = expression.type();
    switch (type) {
#define CODEGEN(T, name)                          \
    case ExpressionType::T: {                     \
        auto id = expression.as_##name();         \
        auto const& value = expressions[id];      \
        TRY(codegen_##name(out, context, value)); \
        if (type == ExpressionType::FunctionCall) \
            TRY(out.write(";"sv));                \
    } break
#define X(T, name, ...) CODEGEN(T, name);
        EXPRESSIONS
#undef X
#undef CODEGEN
    }

    return {};
}

ErrorOr<void> codegen_expression_in_rvalue(StringBuffer& out,
    Context const& context, Expression const& expression)
{
    auto const& expressions = context.expressions;
    switch (expression.type()) {
#define CODEGEN(T, name)                          \
    case ExpressionType::T: {                     \
        auto id = expression.as_##name();         \
        auto const& value = expressions[id];      \
        TRY(codegen_##name(out, context, value)); \
    } break
#define X(T, name, ...) CODEGEN(T, name);
        EXPRESSIONS
#undef X
#undef CODEGEN
    }

    return {};
}

ErrorOr<void> codegen_public_variable_declaration(StringBuffer& out,
    Context const& context,
    PublicVariableDeclaration const& variable)
{
    auto source = context.source;
    auto type = variable.type.text(source);
    TRY(out.write("$"sv, variable.name.text(source), " = ensure_"sv,
        type, "("sv));
    auto const& expressions = context.expressions;
    auto const& value = expressions[variable.value];
    TRY(codegen_expression(out, context, value));
    TRY(out.writeln(");"sv));

    return {};
}

ErrorOr<void> codegen_private_variable_declaration(
    StringBuffer& out, Context const& context,
    PrivateVariableDeclaration const& variable)
{
    auto source = context.source;
    auto type = variable.type.text(source);
    TRY(out.write("$"sv, variable.name.text(source), " = ensure_"sv,
        type, "("sv));
    auto const& expressions = context.expressions;
    auto const& value = expressions[variable.value];
    TRY(codegen_expression(out, context, value));
    TRY(out.writeln(");"sv));

    return {};
}

ErrorOr<void> codegen_public_constant_declaration(StringBuffer&,
    Context const&, PublicConstantDeclaration const&)
{
    return Error::from_string_literal(
        "constants not currently supported");
}

ErrorOr<void> codegen_private_constant_declaration(StringBuffer&,
    Context const&, PrivateConstantDeclaration const&)
{
    return Error::from_string_literal(
        "constants not currently supported");
}

ErrorOr<void> codegen_variable_assignment(StringBuffer& out,
    Context const& context, VariableAssignment const& variable)
{
    auto source = context.source;
    TRY(out.write("$"sv, variable.name.text(source), " = "sv));
    auto const& expressions = context.expressions;
    auto const& value = expressions[variable.value];
    TRY(codegen_rvalue(out, context, value));
    TRY(out.writeln(";"sv));

    return {};
}

ErrorOr<void> codegen_mutable_reference(StringBuffer& out,
    Context const& context, MutableReference const& reference)
{
    TRY(codegen_lvalue(out, context,
        context.expressions[reference.lvalue]));

    return {};
}

ErrorOr<void> codegen_struct_declaration(StringBuffer& out,
    Context const& context, StructDeclaration const& struct_)
{
    if (struct_.name.is(TokenType::Invalid))
        return {};

    auto source = context.source;
    auto struct_name = struct_.name.text(source);
    TRY(out.writeln("class "sv, struct_name, "{"sv));
    for (auto member : context.expressions[struct_.members]) {
        auto type = member.type.text(source);
        auto name = member.name.text(source);
        TRY(out.writeln("public "sv, type, " $"sv, name, ";"sv));
        TRY(out.writeln("public function set_"sv, name, "("sv, type,
            " $value): "sv, struct_name, "{"sv, "$this->"sv, name,
            "= $value; return $this; }"sv));
    }
    TRY(out.writeln("};"sv));
    TRY(out.writeln("function ensure_"sv, struct_name, "("sv,
        struct_name, " $value):"sv, struct_name,
        "{ return $value; }"sv));

    return {};
}

ErrorOr<void> codegen_enum_declaration(StringBuffer& out,
    Context const& context, EnumDeclaration const& enum_)
{
    if (enum_.name.is(TokenType::Invalid))
        return {};

    auto source = context.source;
    auto enum_name = enum_.name.text(source);
    TRY(out.writeln("enum "sv, enum_name));
    if (enum_.underlying_type.type != TokenType::Invalid) {
        TRY(out.write(" :"sv, enum_.underlying_type.text(source),
            " "sv));
    }
    TRY(out.write("{"sv));
    for (auto member : context.expressions[enum_.members]) {
        auto name = member.name.text(source);
        TRY(out.writeln(enum_name, "$"sv, name, ","sv));
    }
    TRY(out.writeln("};"sv));

    return {};
}

ErrorOr<void> codegen_union_declaration(StringBuffer&,
    Context const&, UnionDeclaration const&)
{
    return Error::from_string_literal(
        "unions currently not supported");
}

ErrorOr<void> codegen_variant_declaration(StringBuffer&,
    Context const&, VariantDeclaration const&)
{
    return Error::from_string_literal(
        "variants currently not supported");
}

ErrorOr<void> codegen_struct_initializer(StringBuffer& out,
    Context const& context, StructInitializer const& initializer)
{
    auto source = context.source;
    auto const& expressions = context.expressions;
    TRY(out.writeln("(new "sv, initializer.type.text(source),
        "())"sv));
    for (auto member : expressions[initializer.initializers]) {
        auto name = member.name.text(source);
        TRY(out.writeln("->set_"sv, name, "("sv));
        auto const& irvalue = context.expressions[member.value];
        TRY(codegen_rvalue(out, context, irvalue));
        TRY(out.writeln(")"sv));
    }

    return {};
}

ErrorOr<void> codegen_literal(StringBuffer& out,
    Context const& context, Literal const& literal)
{
    auto source = context.source;
    TRY(out.write(literal.token.text(source)));

    return {};
}

ErrorOr<void> codegen_lvalue(StringBuffer& out,
    Context const& context, LValue const& lvalue)
{
    auto source = context.source;
    TRY(out.write("$"sv, lvalue.token.text(source)));

    return {};
}

ErrorOr<void> codegen_rvalue(StringBuffer& out,
    Context const& context, RValue const& rvalue)
{
    auto const& expressions = context.expressions;

    for (auto const& expression : expressions[rvalue.expressions])
        TRY(codegen_expression_in_rvalue(out, context, expression));

    return {};
}

ErrorOr<void> codegen_if_statement(StringBuffer& out,
    Context const& context, If const& if_statement)
{
    TRY(out.write("if ("sv));
    TRY(codegen_rvalue(out, context,
        context.expressions[if_statement.condition]));
    TRY(out.write(") "sv));
    TRY(codegen_block(out, context,
        context.expressions[if_statement.block]));

    return {};
}

ErrorOr<void> codegen_while_statement(StringBuffer& out,
    Context const& context, While const& while_loop)
{
    TRY(out.write("while ("sv));
    TRY(codegen_rvalue(out, context,
        context.expressions[while_loop.condition]));
    TRY(out.write(") "sv));
    TRY(codegen_block(out, context,
        context.expressions[while_loop.block]));

    return {};
}

ErrorOr<void> codegen_block(StringBuffer& out,
    Context const& context, Block const& block)
{
    TRY(out.writeln("{"sv));
    auto const& expressions
        = context.expressions[block.expressions];
    for (auto const& expression : expressions)
        TRY(codegen_expression(out, context, expression));
    TRY(out.writeln("}"sv));

    return {};
}

ErrorOr<void> codegen_private_function(StringBuffer&,
    Context const&, PrivateFunction const&)
{
    return Error::from_string_literal(
        "private functions not yet supported");
}

ErrorOr<void> codegen_public_function(StringBuffer& out,
    Context const& context, PublicFunction const& function)
{
    auto source = context.source;
    auto const& expressions = context.expressions;

    auto name = function.name.text(source);
    auto return_type = function.return_type.text(source);
    auto const& parameters = expressions[function.parameters];
    TRY(out.write("function "sv, name));
    TRY(codegen_parameters(out, context, parameters));
    TRY(out.write(":"sv, return_type));
    TRY(codegen_block(out, context, expressions[function.block]));

    return {};
}

ErrorOr<void> codegen_private_c_function(StringBuffer&,
    Context const&, PrivateCFunction const&)
{
    return Error::from_string_literal("no such thing in php");
}

ErrorOr<void> codegen_public_c_function(StringBuffer&,
    Context const&, PublicCFunction const&)
{
    return Error::from_string_literal("no such thing in php");
}

ErrorOr<void> codegen_function_call(StringBuffer& out,
    Context const& context, FunctionCall const& function)
{
    auto source = context.source;
    auto const& expressions = context.expressions;
    auto const& arguments = expressions[function.arguments];

    TRY(out.write(function.name.text(source), "("sv));
    if (arguments.is_empty()) {
        TRY(out.writeln(")"sv));
        return {};
    }
    for (u32 i = 0; i < arguments.size() - 1; i++) {
        auto const& argument = arguments[i].as_rvalue();
        TRY(codegen_rvalue(out, context, expressions[argument]));
        TRY(out.write(", "sv));
    }
    auto last_index = arguments.size() - 1;
    auto const& last_argument = arguments[last_index];
    TRY(codegen_rvalue(out, context,
        expressions[last_argument.as_rvalue()]));
    TRY(out.writeln(")"sv));

    return {};
}

ErrorOr<void> codegen_return_statement(StringBuffer& out,
    Context const& context, Return const& return_)
{
    TRY(out.write("return "sv));
    auto const& expressions = context.expressions;
    auto const& value = expressions[return_.value];
    TRY(codegen_expression(out, context, value));
    TRY(out.writeln(";"sv));

    return {};
}

ErrorOr<void> codegen_import_he(StringBuffer&, Context const&,
    Import const&)
{
    return Error::from_string_literal("unsupported");
}

ErrorOr<void> codegen_import_c(StringBuffer&, Context const&,
    ImportC const&)
{
    return Error::from_string_literal("unsupported");
}

ErrorOr<void> codegen_inline_c(StringBuffer& out,
    Context const& context, InlineC const& inline_c)
{
    auto source = context.source;
    TRY(out.writeln(inline_c.literal.text(source), ";"sv));

    return {};
}

ErrorOr<void> codegen_uninitialized(StringBuffer& out,
    Context const&, Uninitialized const&)
{
    TRY(out.write(""sv));

    return {};
}

}

}
