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

static void codegen_literal(FileBuffer& out, Context const&,
    Id<Literal>);

static void codegen_lvalue(FileBuffer& out, Context const&,
    LValue const&);

static void codegen_rvalue(FileBuffer& out, Context const&,
    RValue const&);

static void codegen_if_statement(FileBuffer& out, Context const&,
    If const&);

static void codegen_return(FileBuffer& out, Context const&,
    Return const&);

static void codegen_block(FileBuffer& out, Context const&,
    Block const&);

static void codegen_expression(FileBuffer& out, Context const&,
    Expression const&, bool in_rvalue_expression);

static void codegen_public_variable_declaration(FileBuffer& out,
    Context const&, PublicVariableDeclaration const&);

static void codegen_private_variable_declaration(FileBuffer& out,
    Context const&, PrivateVariableDeclaration const&);

static void codegen_public_constant_declaration(FileBuffer& out,
    Context const&, PublicConstantDeclaration const&);

static void codegen_private_constant_declaration(FileBuffer& out,
    Context const&, PrivateConstantDeclaration const&);

static void codegen_struct_declaration(FileBuffer& out,
    Context const&, StructDeclaration const&);

static void codegen_struct_initializer(FileBuffer& out,
    Context const&, StructInitializer const&);

static void codegen_while_loop(FileBuffer& out, Context const&,
    While const&);

static void codegen_public_function(FileBuffer& out, Context const&,
    PublicFunction const&);

static void codegen_private_function(FileBuffer& out,
    Context const&, PrivateFunction const&);

static void codegen_public_c_function(FileBuffer& out,
    Context const&, PublicCFunction const&);

static void codegen_private_c_function(FileBuffer& out,
    Context const&, PrivateCFunction const&);

static void codegen_parameters(FileBuffer& out, Context const&,
    Parameters const&);

static void codegen_function_call(FileBuffer& out, Context const&,
    FunctionCall const&);

static void codegen_import_c(FileBuffer& out, Context const&,
    ImportC const&);

static void codegen_inline_c(FileBuffer& out, Context const&,
    InlineC const&);

static void codegen_compiler_provided_u64(FileBuffer& out,
    Context const&, CompilerProvidedU64 const&);

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

    for (auto filename : import_c_filenames) {
        out.writeln("#include ",
            filename.text(context.source.text));
    }

    for (auto inline_c_expression : inline_c_expressions)
        out.write(inline_c_expression.text(context.source.text));

    for (auto declaration : struct_forward_declarations) {
        auto name = declaration.name.text(context.source.text);
        out.writeln("typedef struct ", name, ' ', name, ';');
    }

    for (auto const& function :
        public_function_forward_declarations) {
        auto type = function.return_type.text(context.source.text);
        auto name = function.name.text(context.source.text);
        out.write(type, ' ', name);
        codegen_parameters(out, context, function.parameters);
        out.writeln(';');
    }

    for (auto const& function :
        private_function_forward_declarations) {
        auto type = function.return_type.text(context.source.text);
        auto name = function.name.text(context.source.text);
        out.write("static ", type, ' ', name);
        codegen_parameters(out, context, function.parameters);
        out.writeln(';');
    }

    for (auto const& function :
        public_c_function_forward_declarations) {
        auto type = function.return_type.text(context.source.text);
        auto name = function.name.text(context.source.text);
        out.write(type, ' ', name);
        codegen_parameters(out, context, function.parameters);
        out.writeln(';');
    }

    for (auto const& function :
        private_c_function_forward_declarations) {
        auto type = function.return_type.text(context.source.text);
        auto name = function.name.text(context.source.text);
        out.write("static ", type, ' ', name);
        codegen_parameters(out, context, function.parameters);
        out.writeln(';');
    }

    for (auto const& expression : context.expressions.expressions)
        codegen_expression(out, context, expression, false);

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
    if (parameters.empty()) {
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

static void codegen_expression(FileBuffer& out,
    Context const& context, Expression const& expression,
    bool in_rvalue_expression)
{
    auto const& expressions = context.expressions;
    switch (expression.type()) {
    case ExpressionType::Literal:
        codegen_literal(out, context, expression.as_literal());
        break;

    case ExpressionType::PrivateVariableDeclaration: {
        auto id = expression.as_private_variable_declaration();
        auto const& variable = expressions[id];
        codegen_private_variable_declaration(out, context,
            variable);
    } break;

    case ExpressionType::PublicVariableDeclaration: {
        auto id = expression.as_public_variable_declaration();
        auto const& variable = context.expressions[id];
        codegen_public_variable_declaration(out, context, variable);
    } break;

    case ExpressionType::CompilerProvidedU64: {
        auto id = expression.as_compiler_provided_u64();
        auto number = context.expressions[id];
        codegen_compiler_provided_u64(out, context, number);
    } break;

    case ExpressionType::PrivateConstantDeclaration: {
        auto id = expression.as_private_constant_declaration();
        auto const& constant = expressions[id];
        codegen_private_constant_declaration(out, context,
            constant);
    } break;

    case ExpressionType::PublicConstantDeclaration: {
        auto id = expression.as_public_constant_declaration();
        auto const& constant = expressions[id];
        codegen_public_constant_declaration(out, context, constant);
    } break;

    case ExpressionType::StructDeclaration: {
        auto id = expression.as_struct_declaration();
        auto const& struct_ = expressions[id];
        codegen_struct_declaration(out, context, struct_);
    } break;

    case ExpressionType::StructInitializer: {
        auto id = expression.as_struct_initializer();
        auto const& initializer = expressions[id];
        codegen_struct_initializer(out, context, initializer);
    } break;

    case ExpressionType::LValue: {
        auto id = expression.as_lvalue();
        auto const& lvalue = expressions[id];
        codegen_lvalue(out, context, lvalue);
    } break;

    case ExpressionType::RValue: {
        auto id = expression.as_rvalue();
        auto const& rvalue = expressions[id];
        codegen_rvalue(out, context, rvalue);
    } break;

    case ExpressionType::If: {
        auto id = expression.as_if();
        auto const& if_ = expressions[id];
        codegen_if_statement(out, context, if_);
    } break;

    case ExpressionType::While: {
        auto id = expression.as_while();
        auto const& while_ = expressions[id];
        codegen_while_loop(out, context, while_);
    } break;

    case ExpressionType::Block: {
        auto id = expression.as_block();
        auto const& block = expressions[id];
        codegen_block(out, context, block);
    } break;

    case ExpressionType::PrivateFunction: {
        auto id = expression.as_private_function();
        auto const& function = expressions[id];
        codegen_private_function(out, context, function);
    } break;

    case ExpressionType::PublicFunction: {
        auto id = expression.as_public_function();
        auto const& function = expressions[id];
        codegen_public_function(out, context, function);
    } break;

    case ExpressionType::PrivateCFunction: {
        auto id = expression.as_private_c_function();
        auto const& function = expressions[id];
        codegen_private_c_function(out, context, function);
    } break;

    case ExpressionType::PublicCFunction: {
        auto id = expression.as_public_c_function();
        auto const& function = expressions[id];
        codegen_public_c_function(out, context, function);
    } break;

    case ExpressionType::FunctionCall: {
        auto id = expression.as_function_call();
        auto const& function_call = expressions[id];
        codegen_function_call(out, context, function_call);
        if (!in_rvalue_expression)
            out.write(';');
    } break;

    case ExpressionType::Return: {
        auto id = expression.as_return();
        auto const& return_ = expressions[id];
        codegen_return(out, context, return_);
    } break;

    case ExpressionType::ImportC: {
        auto id = expression.as_import_c();
        auto const& import_c = expressions[id];
        codegen_import_c(out, context, import_c);
    } break;

    case ExpressionType::InlineC: {
        auto id = expression.as_inline_c();
        auto const& inline_c = expressions[id];
        codegen_inline_c(out, context, inline_c);
    } break;

    case ExpressionType::Moved: break;

    case ExpressionType::Invalid:
        std::cerr << "ExpressionType::Invalid in codegen\n";
        break;
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
    codegen_expression(out, context, value, false);
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
    codegen_expression(out, context, value, false);
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
    codegen_expression(out, context, value, false);
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
    codegen_expression(out, context, value, false);
    out.writeln(';');
}

static void codegen_struct_declaration(FileBuffer& out,
    Context const& context, StructDeclaration const& struct_)
{
    auto source = context.source.text;
    out.writeln("struct ", struct_.name.text(source), "{");
    for (auto member : struct_.members) {
        auto type = member.type.text(source);
        auto name = member.name.text(source);
        out.writeln(type, ' ', name, ';');
    }
    out.writeln("};");
}

static void codegen_struct_initializer(FileBuffer& out,
    Context const& context, StructInitializer const& initializer)
{
    auto source = context.source.text;
    out.write('(', initializer.type.text(source), ") {");
    for (auto member : initializer.initializers) {
        auto name = member.name.text(source);
        out.writeln('.', name, '=');
        auto const& irvalue = context.expressions[member.value];
        codegen_rvalue(out, context, irvalue);
        out.writeln(',');
    }
    out.writeln("};");
}

static void codegen_literal(FileBuffer& out, Context const& context,
    Id<Literal> literal)
{
    auto source = context.source.text;
    auto token = context.expressions[literal].token;
    out.write(token.text(source));
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
    for (auto const& expression : rvalue.expressions)
        codegen_expression(out, context, expression, true);
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

static void codegen_while_loop(FileBuffer& out,
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
    for (auto const& expression : block.expressions)
        codegen_expression(out, context, expression, false);
    out.writeln('}');
}

static void codegen_private_function(FileBuffer& out,
    Context const& context, PrivateFunction const& function)
{
    auto source = context.source.text;
    out.write("static ", function.return_type.text(source), " ",
        function.name.text(source));
    codegen_parameters(out, context, function.parameters);
    codegen_block(out, context,
        context.expressions[function.block]);
}

static void codegen_public_function(FileBuffer& out,
    Context const& context, PublicFunction const& function)
{
    auto source = context.source.text;
    out.write(function.return_type.text(source), " ",
        function.name.text(source));
    codegen_parameters(out, context, function.parameters);
    codegen_block(out, context,
        context.expressions[function.block]);
}

static void codegen_private_c_function(FileBuffer& out,
    Context const& context, PrivateCFunction const& function)
{
    auto source = context.source.text;
    out.write("static ", function.return_type.text(source), " ",
        function.name.text(source));
    codegen_parameters(out, context, function.parameters);
    codegen_block(out, context,
        context.expressions[function.block]);
}

static void codegen_public_c_function(FileBuffer& out,
    Context const& context, PublicCFunction const& function)
{
    auto source = context.source.text;
    out.write(function.return_type.text(source), " ",
        function.name.text(source));
    codegen_parameters(out, context, function.parameters);
    codegen_block(out, context,
        context.expressions[function.block]);
}

static void codegen_function_call(FileBuffer& out,
    Context const& context, FunctionCall const& function)
{
    auto source = context.source.text;

    out.write(function.name.text(source), "(");
    if (function.arguments.empty()) {
        out.writeln(')');
        return;
    }
    for (u32 i = 0; i < function.arguments.size() - 1; i++) {
        auto const& argument = function.arguments[i].as_rvalue();
        codegen_rvalue(out, context, context.expressions[argument]);
        out.write(", ");
    }
    auto last_index = function.arguments.size() - 1;
    auto const& last_argument = function.arguments[last_index];
    codegen_rvalue(out, context,
        context.expressions[last_argument.as_rvalue()]);
    out.writeln(')');
}

static void codegen_return(FileBuffer& out, Context const& context,
    Return const& return_)
{
    out.write("return ");
    auto const& expressions = context.expressions;
    auto const& value = expressions[return_.value];
    codegen_expression(out, context, value, false);
    out.writeln(';');
}

static void codegen_import_c(FileBuffer& out,
    Context const& context, ImportC const& import_c)
{
    auto source = context.source.text;
    auto filename = import_c.filename.text(source);
    if (!filename.empty())
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
