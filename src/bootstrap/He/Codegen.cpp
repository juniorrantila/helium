#include <FileBuffer.h>
#include <He/Codegen.h>
#include <He/Context.h>
#include <He/Expression.h>
#include <He/Parser.h>
#include <SourceFile.h>
#include <iostream>
#include <string_view>
#include <sys/mman.h>
#include <sys/uio.h>
#include <thread>
#include <type_traits>
#include <vector>

namespace He {

static void dump_literal(FileBuffer& out, Context const&,
    Id<Literal>);
static void dump_lvalue(FileBuffer& out, Context const&,
    LValue const&);
static void dump_rvalue(FileBuffer& out, Context const&,
    RValue const&);
static void dump_if_statement(FileBuffer& out, Context const&,
    If const&);
static void dump_return(FileBuffer& out, Context const&,
    Return const&);
static void dump_block(FileBuffer& out, Context const&,
    Block const&);

static void dump_expression(FileBuffer& out, Context const&,
    Expression const&, bool in_rvalue_expression);

static void dump_public_variable_declaration(FileBuffer& out,
    Context const&, PublicVariableDeclaration const&);

static void dump_private_variable_declaration(FileBuffer& out,
    Context const&, PrivateVariableDeclaration const&);

static void dump_public_constant_declaration(FileBuffer& out,
    Context const&, PublicConstantDeclaration const&);

static void dump_private_constant_declaration(FileBuffer& out,
    Context const&, PrivateConstantDeclaration const&);

static void dump_struct_declaration(FileBuffer& out, Context const&,
    StructDeclaration const&);

static void dump_while_loop(FileBuffer& out, Context const&,
    While const&);
static void dump_public_function(FileBuffer& out, Context const&,
    PublicFunction const&);

static void dump_private_function(FileBuffer& out, Context const&,
    PrivateFunction const&);

static void dump_public_c_function(FileBuffer& out, Context const&,
    PublicCFunction const&);

static void dump_private_c_function(FileBuffer& out, Context const&,
    PrivateCFunction const&);

static void dump_parameters(FileBuffer& out, Context const&,
    Parameters const&);

static void dump_function_call(FileBuffer& out, Context const&,
    FunctionCall const&);

static void dump_import_c(FileBuffer& out, Context const&,
    ImportC const&);

static void dump_inline_c(FileBuffer& out, Context const&,
    InlineC const&);

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

void Codegen::dump(int out_fd, Context const& context) const
{
    auto* files = create_file_for_each_thread(2 * 1024 * 1024);
    auto& out = files[0];
    auto const* prelude = R"c(
#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef char const* c_string;

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

    for (auto const& declaration :
        public_function_forward_declarations) {
        auto type
            = declaration.return_type.text(context.source.text);
        auto name = declaration.name.text(context.source.text);
        out.write(type, ' ', name);
        dump_parameters(out, context, declaration.parameters);
        out.writeln(';');
    }

    for (auto const& declaration :
        private_function_forward_declarations) {
        auto type
            = declaration.return_type.text(context.source.text);
        auto name = declaration.name.text(context.source.text);
        out.write("static ", type, ' ', name);
        dump_parameters(out, context, declaration.parameters);
        out.writeln(';');
    }

    for (auto const& expression : context.expressions.expressions)
        dump_expression(out, context, expression, false);

    u16 vecs = std::thread::hardware_concurrency();
    auto* iovec
        = (struct iovec*)malloc(sizeof(struct iovec) * vecs);
    for (u32 i = 0; i < vecs; i++) {
        iovec[i] = { .iov_base = files[i].data,
            .iov_len = files[i].size };
    }
    writev(out_fd, iovec, vecs);
    free(iovec);
    destroy_file_for_each_thread(files);
}

static void dump_parameters(FileBuffer& out, Context const& context,
    Parameters const& parameters)
{
    auto source = context.source.text;
    out.write('(');
    if (parameters.empty()) {
        out.write("void");
    } else {
        auto last_parameter = parameters.size() - 1;
        for (u32 i = 0; i < last_parameter; i++) {
            auto parameter = parameters[i];
            out.write(parameter.type.text(source), ' ',
                parameter.name.text(source), ", ");
        }
        auto parameter = parameters[last_parameter];
        out.write(parameter.type.text(source), ' ',
            parameter.name.text(source));
    }
    out.write(')');
}

static void dump_expression(FileBuffer& out, Context const& context,
    Expression const& expression, bool in_rvalue_expression)
{
    switch (expression.type()) {
    case ExpressionType::Literal:
        dump_literal(out, context, expression.as_literal());
        break;

    case ExpressionType::PrivateVariableDeclaration:
        dump_private_variable_declaration(out, context,
            expression.as_private_variable_declaration());
        break;

    case ExpressionType::PublicVariableDeclaration:
        dump_public_variable_declaration(out, context,
            expression.as_public_variable_declaration());
        break;

    case ExpressionType::PrivateConstantDeclaration:
        dump_private_constant_declaration(out, context,
            expression.as_private_constant_declaration());
        break;

    case ExpressionType::PublicConstantDeclaration:
        dump_public_constant_declaration(out, context,
            expression.as_public_constant_declaration());
        break;

    case ExpressionType::StructDeclaration:
        dump_struct_declaration(out, context,
            expression.as_struct_declaration());
        break;

    case ExpressionType::LValue:
        dump_lvalue(out, context, expression.as_lvalue());
        break;

    case ExpressionType::RValue:
        dump_rvalue(out, context, expression.as_rvalue());
        break;

    case ExpressionType::If:
        dump_if_statement(out, context, expression.as_if());
        break;

    case ExpressionType::While:
        dump_while_loop(out, context, expression.as_while());
        break;

    case ExpressionType::Block:
        dump_block(out, context, expression.as_block());
        break;

    case ExpressionType::PrivateFunction:
        dump_private_function(out, context,
            expression.as_private_function());
        break;

    case ExpressionType::PublicFunction:
        dump_public_function(out, context,
            expression.as_public_function());
        break;

    case ExpressionType::PrivateCFunction:
        dump_private_c_function(out, context,
            expression.as_private_c_function());
        break;

    case ExpressionType::PublicCFunction:
        dump_public_c_function(out, context,
            expression.as_public_c_function());
        break;

    case ExpressionType::FunctionCall:
        dump_function_call(out, context,
            expression.as_function_call());
        if (!in_rvalue_expression)
            out.write(';');
        break;

    case ExpressionType::Return:
        dump_return(out, context, expression.as_return());
        break;

    case ExpressionType::ImportC:
        dump_import_c(out, context, expression.as_import_c());
        break;

    case ExpressionType::InlineC:
        dump_inline_c(out, context, expression.as_inline_c());
        break;

    case ExpressionType::Invalid:
        std::cerr << "ExpressionType::Invalid in codegen\n";
        break;
    }
}

static void dump_public_variable_declaration(FileBuffer& out,
    Context const& context,
    PublicVariableDeclaration const& variable)
{
    auto source = context.source.text;
    out.write(variable.type.text(source), ' ',
        variable.name.text(source), " = ");
    dump_rvalue(out, context, variable.value);
    out.writeln(';');
}

static void dump_private_variable_declaration(FileBuffer& out,
    Context const& context,
    PrivateVariableDeclaration const& variable)
{
    auto source = context.source.text;
    out.write("static ", variable.type.text(source), ' ',
        variable.name.text(source), " = ");
    dump_rvalue(out, context, variable.value);
    out.writeln(';');
}

static void dump_public_constant_declaration(FileBuffer& out,
    Context const& context,
    PublicConstantDeclaration const& variable)
{
    auto source = context.source.text;
    out.write(variable.type.text(source), " const ",
        variable.name.text(source), " = ");
    dump_rvalue(out, context, variable.value);
    out.writeln(';');
}

static void dump_private_constant_declaration(FileBuffer& out,
    Context const& context,
    PrivateConstantDeclaration const& variable)
{
    auto source = context.source.text;
    out.write("static ", variable.type.text(source), " const ",
        variable.name.text(source), " = ");
    dump_rvalue(out, context, variable.value);
    out.writeln(';');
}

static void dump_struct_declaration(FileBuffer& out,
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

static void dump_literal(FileBuffer& out, Context const& context,
    Id<Literal> literal)
{
    auto source = context.source.text;
    auto token = context.expressions[literal].token;
    out.write(token.text(source));
}

static void dump_lvalue(FileBuffer& out, Context const& context,
    LValue const& lvalue)
{
    auto source = context.source.text;
    out.write(lvalue.token.text(source));
}

static void dump_rvalue(FileBuffer& out, Context const& context,
    RValue const& rvalue)
{
    for (auto const& expression : rvalue.expressions)
        dump_expression(out, context, expression, true);
}

static void dump_if_statement(FileBuffer& out,
    Context const& context, If const& if_statement)
{
    out.write("if (");
    dump_rvalue(out, context, if_statement.condition);
    out.write(") ");
    dump_block(out, context, if_statement.block);
}

static void dump_while_loop(FileBuffer& out, Context const& context,
    While const& while_loop)
{
    out.write("while (");
    dump_rvalue(out, context, while_loop.condition);
    out.write(") ");
    dump_block(out, context, while_loop.block);
}

static void dump_block(FileBuffer& out, Context const& context,
    Block const& block)
{
    out.writeln('{');
    for (auto const& expression : block.expressions)
        dump_expression(out, context, expression, false);
    out.writeln('}');
}

static void dump_private_function(FileBuffer& out,
    Context const& context, PrivateFunction const& function)
{
    auto source = context.source.text;
    out.write("static ", function.return_type.text(source), " ",
        function.name.text(source));
    dump_parameters(out, context, function.parameters);
    dump_block(out, context, context.expressions[function.block]);
}

static void dump_public_function(FileBuffer& out,
    Context const& context, PublicFunction const& function)
{
    auto source = context.source.text;
    out.write(function.return_type.text(source), " ",
        function.name.text(source));
    dump_parameters(out, context, function.parameters);
    dump_block(out, context, context.expressions[function.block]);
}

static void dump_private_c_function(FileBuffer& out,
    Context const& context, PrivateCFunction const& function)
{
    auto source = context.source.text;
    out.write("static ", function.return_type.text(source), " ",
        function.name.text(source));
    dump_parameters(out, context, function.parameters);
    dump_block(out, context, context.expressions[function.block]);
}

static void dump_public_c_function(FileBuffer& out,
    Context const& context, PublicCFunction const& function)
{
    auto source = context.source.text;
    out.write(function.return_type.text(source), " ",
        function.name.text(source));
    dump_parameters(out, context, function.parameters);
    dump_block(out, context, context.expressions[function.block]);
}

static void dump_function_call(FileBuffer& out,
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
        dump_rvalue(out, context, argument);
        out.write(", ");
    }
    auto last_index = function.arguments.size() - 1;
    auto const& last_argument = function.arguments[last_index];
    dump_rvalue(out, context, last_argument.as_rvalue());
    out.writeln(')');
}

static void dump_return(FileBuffer& out, Context const& context,
    Return const& return_)
{
    out.write("return ");
    dump_rvalue(out, context, return_.rvalue);
    out.writeln(';');
}

static void dump_import_c(FileBuffer& out, Context const& context,
    ImportC const& import_c)
{
    auto source = context.source.text;
    auto filename = import_c.filename.text(source);
    if (!filename.empty())
        out.writeln("#include ", import_c.filename.text(source));
}

static void dump_inline_c(FileBuffer& out, Context const& context,
    InlineC const& inline_c)
{
    auto source = context.source.text;
    out.write(inline_c.literal.text(source));
}

}
