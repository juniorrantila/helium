#include "Context.h"
#include "Expression.h"
#include "Parser.h"
#include "SourceFile.h"
#include "Token.h"
#include "TypecheckedExpression.h"
#include <Core/File.h>
#include <Core/Threads.h>
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

}

ErrorOr<StringBuffer> codegen(Context const& context,
    TypecheckedExpressions const& typechecked)
{
    auto estimate = context.expressions.expressions.size() * 512;
    auto out = TRY(StringBuffer::create(estimate));
    auto prelude = R"c(
#include <stdint.h>
#include <stddef.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef char c_char;
typedef short c_short;
typedef int c_int;
typedef long c_long;
typedef long long c_longlong;

typedef unsigned char c_uchar;
typedef unsigned short c_ushort;
typedef unsigned int c_uint;
typedef unsigned long c_ulong;
typedef unsigned long long c_ulonglong;

typedef float c_float;
typedef double c_double;

typedef size_t usize;
typedef void c_void;

typedef char const* c_string;

#define true 1
#define false 0

#define let __auto_type const
#define var __auto_type

)c"sv;
    TRY(out.write(prelude));

    for (auto filename : typechecked.import_c_quoted_filenames) {
        TRY(out.writeln("#include "sv,
            filename.text(context.source.text)));
    }

    for (auto inline_c_expression : typechecked.inline_c_texts) {
        TRY(out.write(
            inline_c_expression.text(context.source.text)));
    }

    for (auto declaration : typechecked.enum_forwards) {
        auto name = declaration.name.text(context.source.text);
        TRY(out.writeln("typedef enum "sv, name, " "sv, name,
            ";"sv));
    }

    for (auto declaration : typechecked.struct_forwards) {
        auto name = declaration.name.text(context.source.text);
        TRY(out.writeln("typedef struct "sv, name, " "sv, name,
            ";"sv));
    }

    for (auto declaration : typechecked.union_forwards) {
        auto name = declaration.name.text(context.source.text);
        TRY(out.writeln("typedef union "sv, name, " "sv, name,
            ";"sv));
    }

    for (auto declaration : typechecked.variant_forwards) {
        auto name = declaration.name.text(context.source.text);
        TRY(out.writeln("typedef struct "sv, name, " "sv, name,
            ";"sv));
    }

    for (auto const& function :
        typechecked.public_function_forwards) {
        auto type = function.return_type.text(context.source.text);
        auto name = function.name.text(context.source.text);
        TRY(out.write(type, " "sv, name));
        TRY(codegen_parameters(out, context, function.parameters));
        TRY(out.writeln(";"sv));
    }

    for (auto const& function :
        typechecked.private_function_forwards) {
        auto type = function.return_type.text(context.source.text);
        auto name = function.name.text(context.source.text);
        TRY(out.write("static "sv, type, " "sv, name));
        TRY(codegen_parameters(out, context, function.parameters));
        TRY(out.writeln(";"sv));
    }

    for (auto const& function :
        typechecked.public_c_function_forwards) {
        auto type = function.return_type.text(context.source.text);
        auto name = function.name.text(context.source.text);
        TRY(out.write(type, " "sv, name));
        TRY(codegen_parameters(out, context, function.parameters));
        TRY(out.writeln(";"sv));
    }

    for (auto const& function :
        typechecked.private_c_function_forwards) {
        auto type = function.return_type.text(context.source.text);
        auto name = function.name.text(context.source.text);
        TRY(out.write("static "sv, type, " "sv, name));
        TRY(codegen_parameters(out, context, function.parameters));
        TRY(out.writeln(";"sv));
    }

    for (auto& enum_ : context.expressions.enum_declarations) {
        TRY(codegen_enum_declaration(out, context, enum_));
        enum_.name.type = TokenType::Invalid;
    }

    for (auto& struct_ : context.expressions.struct_declarations) {
        TRY(codegen_struct_declaration(out, context, struct_));
        struct_.name.type = TokenType::Invalid;
    }

    for (auto& union_ : context.expressions.union_declarations) {
        TRY(codegen_union_declaration(out, context, union_));
        union_.name.type = TokenType::Invalid;
    }

    for (auto& variant : context.expressions.variant_declarations) {
        TRY(codegen_variant_declaration(out, context, variant));
        variant.name.type = TokenType::Invalid;
    }

    for (auto const& expression : context.expressions.expressions)
        TRY(codegen_expression(out, context, expression));

    return out;
}

namespace {

ErrorOr<void> codegen_parameters(StringBuffer& out,
    Context const& context, Parameters const& parameters)
{
    auto source = context.source.text;
    if (parameters.is_empty()) {
        TRY(out.write("(void)"sv));
        return {};
    }
    TRY(out.write("("sv));
    auto last_parameter = parameters.size() - 1;
    for (u32 i = 0; i < last_parameter; i++) {
        auto parameter = parameters[i];
        TRY(out.write(parameter.type.text(source), " "sv,
            parameter.name.text(source), ", "sv));
    }
    auto parameter = parameters[last_parameter];
    TRY(out.write(parameter.type.text(source), " "sv,
        parameter.name.text(source)));
    TRY(out.write(")"sv));

    return {};
}

ErrorOr<void> codegen_member_access(StringBuffer& out,
    Context const& context, MemberAccess const& access)
{
    auto source = context.source.text;
    auto const& members = context.expressions[access.members];

    for (u32 i = 0; i < members.size() - 1; i++) {
        auto member = members[i];
        TRY(out.write(member.text(source), "."sv));
    }
    TRY(out.write(members[members.size() - 1].text(source)));

    return {};
}

ErrorOr<void> codegen_array_access(StringBuffer& out,
    Context const& context, ArrayAccess const& access)
{
    auto source = context.source.text;
    auto name = access.name.text(source);
    TRY(out.write(name, "["sv));
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
    auto source = context.source.text;
    TRY(out.write(variable.type.text(source), " "sv,
        variable.name.text(source), " = "sv));
    auto const& expressions = context.expressions;
    auto const& value = expressions[variable.value];
    TRY(codegen_expression(out, context, value));
    TRY(out.writeln(";"sv));

    return {};
}

ErrorOr<void> codegen_private_variable_declaration(
    StringBuffer& out, Context const& context,
    PrivateVariableDeclaration const& variable)
{
    auto source = context.source.text;
    TRY(out.write("static "sv, variable.type.text(source), " "sv,
        variable.name.text(source), " = "sv));
    auto const& expressions = context.expressions;
    auto const& value = expressions[variable.value];
    TRY(codegen_expression(out, context, value));
    TRY(out.writeln(";"sv));

    return {};
}

ErrorOr<void> codegen_public_constant_declaration(StringBuffer& out,
    Context const& context,
    PublicConstantDeclaration const& variable)
{
    auto source = context.source.text;
    TRY(out.write(variable.type.text(source), " const "sv,
        variable.name.text(source), " = "sv));
    auto const& expressions = context.expressions;
    auto const& value = expressions[variable.value];
    TRY(codegen_expression(out, context, value));
    TRY(out.writeln(";"sv));

    return {};
}

ErrorOr<void> codegen_private_constant_declaration(
    StringBuffer& out, Context const& context,
    PrivateConstantDeclaration const& variable)
{
    auto source = context.source.text;
    TRY(out.write("static "sv, variable.type.text(source),
        " const "sv, variable.name.text(source), " = "sv));
    auto const& expressions = context.expressions;
    auto const& value = expressions[variable.value];
    TRY(codegen_expression(out, context, value));
    TRY(out.writeln(";"sv));

    return {};
}

ErrorOr<void> codegen_variable_assignment(StringBuffer& out,
    Context const& context, VariableAssignment const& variable)
{
    auto source = context.source.text;
    TRY(out.write(variable.name.text(source), " = "sv));
    auto const& expressions = context.expressions;
    auto const& value = expressions[variable.value];
    TRY(codegen_rvalue(out, context, value));
    TRY(out.writeln(";"sv));

    return {};
}

ErrorOr<void> codegen_struct_declaration(StringBuffer& out,
    Context const& context, StructDeclaration const& struct_)
{
    if (struct_.name.is(TokenType::Invalid))
        return {};

    auto source = context.source.text;
    TRY(out.writeln("struct "sv, struct_.name.text(source), "{"sv));
    for (auto member : context.expressions[struct_.members]) {
        auto type = member.type.text(source);
        auto name = member.name.text(source);
        TRY(out.writeln(type, " "sv, name, ";"sv));
    }
    TRY(out.writeln("};"sv));

    return {};
}

ErrorOr<void> codegen_enum_declaration(StringBuffer& out,
    Context const& context, EnumDeclaration const& enum_)
{
    if (enum_.name.is(TokenType::Invalid))
        return {};

    auto source = context.source.text;
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

ErrorOr<void> codegen_union_declaration(StringBuffer& out,
    Context const& context, UnionDeclaration const& union_)
{
    if (union_.name.is(TokenType::Invalid))
        return {};

    auto source = context.source.text;
    TRY(out.writeln("union "sv, union_.name.text(source), "{"sv));
    for (auto member : context.expressions[union_.members]) {
        auto type = member.type.text(source);
        auto name = member.name.text(source);
        TRY(out.writeln(type, " "sv, name, ";"sv));
    }
    TRY(out.writeln("};"sv));

    return {};
}

ErrorOr<void> codegen_variant_declaration(StringBuffer& out,
    Context const& context, VariantDeclaration const& variant)
{
    if (variant.name.is(TokenType::Invalid))
        return {};

    auto source = context.source.text;
    auto variant_name = variant.name.text(source);
    TRY(out.writeln("struct "sv, variant_name, "{"sv));

    TRY(out.writeln("union {"sv));
    for (auto member : context.expressions[variant.members]) {
        auto type = member.type.text(source);
        auto name = member.name.text(source);
        TRY(out.writeln(type, " "sv, name, ";"sv));
    }
    TRY(out.writeln("};"sv));

    TRY(out.writeln("enum {"sv));
    for (auto member : context.expressions[variant.members]) {
        auto name = member.name.text(source);
        TRY(out.writeln(variant_name, "$Type$"sv, name, ","sv));
    }
    TRY(out.writeln("} type;"sv));

    TRY(out.writeln("};"sv));

    return {};
}

ErrorOr<void> codegen_struct_initializer(StringBuffer& out,
    Context const& context, StructInitializer const& initializer)
{
    auto source = context.source.text;
    auto const& expressions = context.expressions;
    TRY(out.write("("sv, initializer.type.text(source), ") {"sv));
    for (auto member : expressions[initializer.initializers]) {
        auto name = member.name.text(source);
        TRY(out.writeln("."sv, name, "="sv));
        auto const& irvalue = context.expressions[member.value];
        TRY(codegen_rvalue(out, context, irvalue));
        TRY(out.writeln(","sv));
    }
    TRY(out.writeln("}"sv));

    return {};
}

ErrorOr<void> codegen_literal(StringBuffer& out,
    Context const& context, Literal const& literal)
{
    auto source = context.source.text;
    TRY(out.write(literal.token.text(source)));

    return {};
}

ErrorOr<void> codegen_lvalue(StringBuffer& out,
    Context const& context, LValue const& lvalue)
{
    auto source = context.source.text;
    TRY(out.write(lvalue.token.text(source)));

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

ErrorOr<void> codegen_private_function(StringBuffer& out,
    Context const& context, PrivateFunction const& function)
{
    auto source = context.source.text;
    auto const& expressions = context.expressions;
    TRY(out.write("static "sv, function.return_type.text(source),
        " "sv, function.name.text(source)));
    TRY(codegen_parameters(out, context,
        expressions[function.parameters]));
    TRY(codegen_block(out, context,
        context.expressions[function.block]));

    return {};
}

ErrorOr<void> codegen_public_function(StringBuffer& out,
    Context const& context, PublicFunction const& function)
{
    auto source = context.source.text;
    auto const& expressions = context.expressions;

    TRY(out.write(function.return_type.text(source), " "sv,
        function.name.text(source)));
    TRY(codegen_parameters(out, context,
        expressions[function.parameters]));
    TRY(codegen_block(out, context,
        context.expressions[function.block]));

    return {};
}

ErrorOr<void> codegen_private_c_function(StringBuffer& out,
    Context const& context, PrivateCFunction const& function)
{
    auto source = context.source.text;
    auto const& expressions = context.expressions;

    TRY(out.write("static "sv, function.return_type.text(source),
        " "sv, function.name.text(source)));
    TRY(codegen_parameters(out, context,
        expressions[function.parameters]));
    TRY(codegen_block(out, context,
        context.expressions[function.block]));

    return {};
}

ErrorOr<void> codegen_public_c_function(StringBuffer& out,
    Context const& context, PublicCFunction const& function)
{
    auto source = context.source.text;
    auto const& expressions = context.expressions;

    TRY(out.write(function.return_type.text(source), " "sv,
        function.name.text(source)));
    TRY(codegen_parameters(out, context,
        expressions[function.parameters]));
    TRY(codegen_block(out, context,
        context.expressions[function.block]));

    return {};
}

ErrorOr<void> codegen_function_call(StringBuffer& out,
    Context const& context, FunctionCall const& function)
{
    auto source = context.source.text;
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

ErrorOr<void> codegen_import_c(StringBuffer& out,
    Context const& context, ImportC const& import_c)
{
    auto source = context.source.text;
    auto filename = import_c.filename.text(source);
    if (!filename.is_empty())
        TRY(out.writeln("#include "sv,
            import_c.filename.text(source)));

    return {};
}

ErrorOr<void> codegen_inline_c(StringBuffer& out,
    Context const& context, InlineC const& inline_c)
{
    auto source = context.source.text;
    TRY(out.write(inline_c.literal.text(source)));

    return {};
}

ErrorOr<void> codegen_uninitialized(StringBuffer& out,
    Context const&, Uninitialized const&)
{
    TRY(out.write("{ 0 }"sv));

    return {};
}

}

}
