#include <FileBuffer.h>
#include <He/Context.h>
#include <He/Expression.h>
#include <He/Parser.h>
#include <He/TypecheckedExpression.h>
#include <SourceFile.h>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/uio.h>
#include <thread>
#include <type_traits>
#include <vector>

namespace He {

#define FORWARD_DECLARE_CODEGEN(T, name)                    \
    static void codegen_##name(FileBuffer&, Context const&, \
        T const&)

#define X(T, name, ...) FORWARD_DECLARE_CODEGEN(T, name);
EXPRESSIONS
#undef X

FORWARD_DECLARE_CODEGEN(Expression, expression);
FORWARD_DECLARE_CODEGEN(Parameters, parameters);

#undef FORWARD_DECLARE_CODEGEN

static char* create_buffer_for_each_thread(u32 size,
    u32 threads = std::thread::hardware_concurrency())
{
    auto memory_size = size * threads;
    auto* memory = mmap(0, memory_size, PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED)
        return nullptr;
    return (char*)memory;
}

static void destroy_buffer_for_each_thread(char* buf, u32 size,
    u32 threads = std::thread::hardware_concurrency())
{
    auto memory_size = size * threads;
    munmap(buf, memory_size);
}

static FileBuffer* create_file_for_each_thread(u32 size,
    u32 threads = std::thread::hardware_concurrency())
{
    auto* files = (FileBuffer*)malloc(sizeof(FileBuffer) * threads);
    if (!files)
        return files;
    auto* buf = create_buffer_for_each_thread(size);
    if (!buf) {
        free(files);
        return nullptr;
    }
    for (u64 i = 0; i < threads; i++) {
        files[i] = FileBuffer {
            &buf[i * size],
            size,
        };
    }
    return files;
}

static void destroy_file_for_each_thread(FileBuffer* file,
    u32 threads = std::thread::hardware_concurrency())
{
    destroy_buffer_for_each_thread(file->data, file->capacity,
        threads);
    free(file);
}

void TypecheckedExpressions::codegen(int out_fd,
    Context const& context) const
{
    auto* files = create_file_for_each_thread(2 * 1024 * 1024);
    auto& out = files[0];
    auto const* prelude = R"c(
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

)c";
    out.write(prelude);

    for (auto filename : import_cs) {
        out.writeln("#include ",
            filename.text(context.source.text));
    }

    for (auto inline_c_expression : inline_cs)
        out.write(inline_c_expression.text(context.source.text));

    for (auto declaration : enum_forwards) {
        auto name = declaration.name.text(context.source.text);
        out.writeln("typedef enum ", name, ' ', name, ';');
    }

    for (auto declaration : struct_forwards) {
        auto name = declaration.name.text(context.source.text);
        out.writeln("typedef struct ", name, ' ', name, ';');
    }

    for (auto declaration : union_forwards) {
        auto name = declaration.name.text(context.source.text);
        out.writeln("typedef union ", name, ' ', name, ';');
    }

    for (auto declaration : variant_forwards) {
        auto name = declaration.name.text(context.source.text);
        out.writeln("typedef struct ", name, ' ', name, ';');
    }

    for (auto const& function : public_function_forwards) {
        auto type = function.return_type.text(context.source.text);
        auto name = function.name.text(context.source.text);
        out.write(type, ' ', name);
        codegen_parameters(out, context, function.parameters);
        out.writeln(';');
    }

    for (auto const& function : private_function_forwards) {
        auto type = function.return_type.text(context.source.text);
        auto name = function.name.text(context.source.text);
        out.write("static ", type, ' ', name);
        codegen_parameters(out, context, function.parameters);
        out.writeln(';');
    }

    for (auto const& function : public_c_function_forwards) {
        auto type = function.return_type.text(context.source.text);
        auto name = function.name.text(context.source.text);
        out.write(type, ' ', name);
        codegen_parameters(out, context, function.parameters);
        out.writeln(';');
    }

    for (auto const& function : private_c_function_forwards) {
        auto type = function.return_type.text(context.source.text);
        auto name = function.name.text(context.source.text);
        out.write("static ", type, ' ', name);
        codegen_parameters(out, context, function.parameters);
        out.writeln(';');
    }

    for (auto const& expression : context.expressions.expressions)
        codegen_expression(out, context, expression);

    u16 vecs = std::thread::hardware_concurrency();
    auto* iovec
        = (struct iovec*)malloc(sizeof(struct iovec) * vecs);
    for (u32 i = 0; i < vecs; i++) {
        iovec[i] = {
            .iov_base = files[i].data,
            .iov_len = files[i].size,
        };
    }
    writev(out_fd, iovec, vecs);
    free(iovec);
    destroy_file_for_each_thread(files);
}

static void codegen_parameters(FileBuffer& out,
    Context const& context, Parameters const& parameters)
{
    auto source = context.source.text;
    if (parameters.is_empty()) {
        out.write("(void)");
        return;
    }
    out.write('(');
    auto last_parameter = parameters.size() - 1;
    for (u32 i = 0; i < last_parameter; i++) {
        auto parameter = parameters[i];
        out.write(parameter.type.text(source), ' ',
            parameter.name.text(source), ", ");
    }
    auto parameter = parameters[last_parameter];
    out.write(parameter.type.text(source), ' ',
        parameter.name.text(source));
    out.write(')');
}

static void codegen_member_access(FileBuffer& out,
    Context const& context, MemberAccess const& access)
{
    auto source = context.source.text;
    auto const& members = context.expressions[access.members];

    for (u32 i = 0; i < members.size() - 1; i++) {
        auto member = members[i];
        out.write(member.text(source), '.');
    }
    out.write(members[members.size() - 1].text(source));
}

static void codegen_array_access(FileBuffer& out,
    Context const& context, ArrayAccess const& access)
{
    auto source = context.source.text;
    auto name = access.name.text(source);
    out.write(name, '[');
    auto const& index = context.expressions[access.index];
    codegen_rvalue(out, context, index);
    out.write(']');
}

static void codegen_moved_value(FileBuffer&, Context const&,
    Moved const&)
{
}

static void codegen_invalid(FileBuffer&, Context const&,
    Invalid const&)
{
    std::cerr << "ExpressionType::Invalid in codegen\n";
    __builtin_abort();
}

static void codegen_expression(FileBuffer& out,
    Context const& context, Expression const& expression)
{
    auto const& expressions = context.expressions;
    auto const type = expression.type();
    switch (type) {
#define CODEGEN(T, name)                          \
    case ExpressionType::T: {                     \
        auto id = expression.as_##name();         \
        auto const& value = expressions[id];      \
        codegen_##name(out, context, value);      \
        if (type == ExpressionType::FunctionCall) \
            out.write(';');                       \
    } break
#define X(T, name, ...) CODEGEN(T, name);
        EXPRESSIONS
#undef X
#undef CODEGEN
    }
}

static void codegen_expression_in_rvalue(FileBuffer& out,
    Context const& context, Expression const& expression)
{
    auto const& expressions = context.expressions;
    switch (expression.type()) {
#define CODEGEN(T, name)                     \
    case ExpressionType::T: {                \
        auto id = expression.as_##name();    \
        auto const& value = expressions[id]; \
        codegen_##name(out, context, value); \
    } break
#define X(T, name, ...) CODEGEN(T, name);
        EXPRESSIONS
#undef X
#undef CODEGEN
    }
}

static void codegen_public_variable_declaration(FileBuffer& out,
    Context const& context,
    PublicVariableDeclaration const& variable)
{
    auto source = context.source.text;
    out.write(variable.type.text(source), ' ',
        variable.name.text(source), " = ");
    auto const& expressions = context.expressions;
    auto const& value = expressions[variable.value];
    codegen_expression(out, context, value);
    out.writeln(';');
}

static void codegen_private_variable_declaration(FileBuffer& out,
    Context const& context,
    PrivateVariableDeclaration const& variable)
{
    auto source = context.source.text;
    out.write("static ", variable.type.text(source), ' ',
        variable.name.text(source), " = ");
    auto const& expressions = context.expressions;
    auto const& value = expressions[variable.value];
    codegen_expression(out, context, value);
    out.writeln(';');
}

static void codegen_public_constant_declaration(FileBuffer& out,
    Context const& context,
    PublicConstantDeclaration const& variable)
{
    auto source = context.source.text;
    out.write(variable.type.text(source), " const ",
        variable.name.text(source), " = ");
    auto const& expressions = context.expressions;
    auto const& value = expressions[variable.value];
    codegen_expression(out, context, value);
    out.writeln(';');
}

static void codegen_private_constant_declaration(FileBuffer& out,
    Context const& context,
    PrivateConstantDeclaration const& variable)
{
    auto source = context.source.text;
    out.write("static ", variable.type.text(source), " const ",
        variable.name.text(source), " = ");
    auto const& expressions = context.expressions;
    auto const& value = expressions[variable.value];
    codegen_expression(out, context, value);
    out.writeln(';');
}

static void codegen_variable_assignment(FileBuffer& out,
    Context const& context,
    VariableAssignment const& variable)
{
    auto source = context.source.text;
    out.write(variable.name.text(source), " = ");
    auto const& expressions = context.expressions;
    auto const& value = expressions[variable.value];
    codegen_rvalue(out, context, value);
    out.writeln(';');
}

static void codegen_struct_declaration(FileBuffer& out,
    Context const& context, StructDeclaration const& struct_)
{
    auto source = context.source.text;
    out.writeln("struct ", struct_.name.text(source), "{");
    for (auto member : context.expressions[struct_.members]) {
        auto type = member.type.text(source);
        auto name = member.name.text(source);
        out.writeln(type, ' ', name, ';');
    }
    out.writeln("};");
}

static void codegen_enum_declaration(FileBuffer& out,
    Context const& context, EnumDeclaration const& enum_)
{
    auto source = context.source.text;
    auto enum_name = enum_.name.text(source);
    out.writeln("enum ", enum_name);
    if (enum_.underlying_type.type != TokenType::Invalid) {
        out.write(" :", enum_.underlying_type.text(source), " ");
    }
    out.write("{");
    for (auto member : context.expressions[enum_.members]) {
        auto name = member.name.text(source);
        out.writeln(enum_name, '$', name, ',');
    }
    out.writeln("};");
}

static void codegen_union_declaration(FileBuffer& out,
    Context const& context, UnionDeclaration const& union_)
{
    auto source = context.source.text;
    out.writeln("union ", union_.name.text(source), "{");
    for (auto member : context.expressions[union_.members]) {
        auto type = member.type.text(source);
        auto name = member.name.text(source);
        out.writeln(type, ' ', name, ';');
    }
    out.writeln("};");
}

static void codegen_variant_declaration(FileBuffer& out,
    Context const& context, VariantDeclaration const& variant)
{
    auto source = context.source.text;
    auto variant_name = variant.name.text(source);
    out.writeln("struct ", variant_name, "{");

    out.writeln("union {");
    for (auto member : context.expressions[variant.members]) {
        auto type = member.type.text(source);
        auto name = member.name.text(source);
        out.writeln(type, ' ', name, ';');
    }
    out.writeln("};");

    out.writeln("enum {");
    for (auto member : context.expressions[variant.members]) {
        auto name = member.name.text(source);
        out.writeln(variant_name, "$Type$", name, ',');
    }
    out.writeln("} type;");

    out.writeln("};");
}

static void codegen_struct_initializer(FileBuffer& out,
    Context const& context, StructInitializer const& initializer)
{
    auto source = context.source.text;
    auto const& expressions = context.expressions;
    out.write('(', initializer.type.text(source), ") {");
    for (auto member : expressions[initializer.initializers]) {
        auto name = member.name.text(source);
        out.writeln('.', name, '=');
        auto const& irvalue = context.expressions[member.value];
        codegen_rvalue(out, context, irvalue);
        out.writeln(',');
    }
    out.writeln('}');
}

static void codegen_literal(FileBuffer& out, Context const& context,
    Literal const& literal)
{
    auto source = context.source.text;
    out.write(literal.token.text(source));
}

static void codegen_lvalue(FileBuffer& out, Context const& context,
    LValue const& lvalue)
{
    auto source = context.source.text;
    out.write(lvalue.token.text(source));
}

static void codegen_rvalue(FileBuffer& out, Context const& context,
    RValue const& rvalue)
{
    auto const& expressions = context.expressions;

    for (auto const& expression : expressions[rvalue.expressions])
        codegen_expression_in_rvalue(out, context, expression);
}

static void codegen_if_statement(FileBuffer& out,
    Context const& context, If const& if_statement)
{
    out.write("if (");
    codegen_rvalue(out, context,
        context.expressions[if_statement.condition]);
    out.write(") ");
    codegen_block(out, context,
        context.expressions[if_statement.block]);
}

static void codegen_while_statement(FileBuffer& out,
    Context const& context, While const& while_loop)
{
    out.write("while (");
    codegen_rvalue(out, context,
        context.expressions[while_loop.condition]);
    out.write(") ");
    codegen_block(out, context,
        context.expressions[while_loop.block]);
}

static void codegen_block(FileBuffer& out, Context const& context,
    Block const& block)
{
    out.writeln('{');
    auto const& expressions
        = context.expressions[block.expressions];
    for (auto const& expression : expressions)
        codegen_expression(out, context, expression);
    out.writeln('}');
}

static void codegen_private_function(FileBuffer& out,
    Context const& context, PrivateFunction const& function)
{
    auto source = context.source.text;
    auto const& expressions = context.expressions;
    out.write("static ", function.return_type.text(source), " ",
        function.name.text(source));
    codegen_parameters(out, context,
        expressions[function.parameters]);
    codegen_block(out, context,
        context.expressions[function.block]);
}

static void codegen_public_function(FileBuffer& out,
    Context const& context, PublicFunction const& function)
{
    auto source = context.source.text;
    auto const& expressions = context.expressions;

    out.write(function.return_type.text(source), " ",
        function.name.text(source));
    codegen_parameters(out, context,
        expressions[function.parameters]);
    codegen_block(out, context,
        context.expressions[function.block]);
}

static void codegen_private_c_function(FileBuffer& out,
    Context const& context, PrivateCFunction const& function)
{
    auto source = context.source.text;
    auto const& expressions = context.expressions;

    out.write("static ", function.return_type.text(source), " ",
        function.name.text(source));
    codegen_parameters(out, context,
        expressions[function.parameters]);
    codegen_block(out, context,
        context.expressions[function.block]);
}

static void codegen_public_c_function(FileBuffer& out,
    Context const& context, PublicCFunction const& function)
{
    auto source = context.source.text;
    auto const& expressions = context.expressions;

    out.write(function.return_type.text(source), " ",
        function.name.text(source));
    codegen_parameters(out, context,
        expressions[function.parameters]);
    codegen_block(out, context,
        context.expressions[function.block]);
}

static void codegen_function_call(FileBuffer& out,
    Context const& context, FunctionCall const& function)
{
    auto source = context.source.text;
    auto const& expressions = context.expressions;
    auto const& arguments = expressions[function.arguments];

    out.write(function.name.text(source), "(");
    if (arguments.is_empty()) {
        out.writeln(')');
        return;
    }
    for (u32 i = 0; i < arguments.size() - 1; i++) {
        auto const& argument = arguments[i].as_rvalue();
        codegen_rvalue(out, context, expressions[argument]);
        out.write(", ");
    }
    auto last_index = arguments.size() - 1;
    auto const& last_argument = arguments[last_index];
    codegen_rvalue(out, context,
        expressions[last_argument.as_rvalue()]);
    out.writeln(')');
}

static void codegen_return_statement(FileBuffer& out,
    Context const& context, Return const& return_)
{
    out.write("return ");
    auto const& expressions = context.expressions;
    auto const& value = expressions[return_.value];
    codegen_expression(out, context, value);
    out.writeln(';');
}

static void codegen_import_c(FileBuffer& out,
    Context const& context, ImportC const& import_c)
{
    auto source = context.source.text;
    auto filename = import_c.filename.text(source);
    if (!filename.is_empty())
        out.writeln("#include ", import_c.filename.text(source));
}

static void codegen_inline_c(FileBuffer& out,
    Context const& context, InlineC const& inline_c)
{
    auto source = context.source.text;
    out.write(inline_c.literal.text(source));
}

static void codegen_compiler_provided_u64(FileBuffer& out,
    Context const&,
    CompilerProvidedU64 const& compiler_provided_u64)
{
    auto number = std::to_string(compiler_provided_u64.number);
    out.write(std::move(number));
}
}
