#include <He/Expression.h>
#include <He/Parser.h>
#include <iostream>

namespace He {

void Expression::dump(ParsedExpressions const& expressions,
    std::string_view source, u32 indent) const
{
    switch (type()) {
    case ExpressionType::PrivateVariableDeclaration:
        as_private_variable_declaration().dump(expressions, source,
            indent);
        break;

    case ExpressionType::PublicVariableDeclaration:
        as_public_variable_declaration().dump(expressions, source,
            indent);
        break;

    case ExpressionType::PrivateConstantDeclaration:
        as_private_constant_declaration().dump(expressions, source,
            indent);
        break;

    case ExpressionType::PublicConstantDeclaration:
        as_public_constant_declaration().dump(expressions, source,
            indent);
        break;

    case ExpressionType::StructDeclaration:
        as_struct_declaration().dump(expressions, source, indent);
        break;

    case ExpressionType::LValue:
        as_lvalue().dump(expressions, source, indent);
        break;

    case ExpressionType::RValue:
        as_rvalue().dump(expressions, source, indent);
        break;

    case ExpressionType::If:
        as_if().dump(expressions, source, indent);
        break;

    case ExpressionType::While:
        as_while().dump(expressions, source, indent);
        break;

    case ExpressionType::Literal:
        expressions[as_literal()].dump(expressions, source, indent);
        break;

    case ExpressionType::Block:
        expressions[as_block()].dump(expressions, source, indent);
        break;

    case ExpressionType::PublicFunction:
        as_public_function().dump(expressions, source, indent);
        break;

    case ExpressionType::PrivateFunction:
        as_private_function().dump(expressions, source, indent);
        break;

    case ExpressionType::PublicCFunction:
        as_public_c_function().dump(expressions, source, indent);
        break;

    case ExpressionType::PrivateCFunction:
        as_private_c_function().dump(expressions, source, indent);
        break;

    case ExpressionType::FunctionCall:
        as_function_call().dump(expressions, source, indent);
        break;

    case ExpressionType::Return:
        as_return().dump(expressions, source, indent);
        break;

    case ExpressionType::ImportC:
        as_import_c().dump(expressions, source, indent);
        break;

    case ExpressionType::InlineC:
        as_inline_c().dump(expressions, source, indent);
        break;

    case ExpressionType::Invalid:
        std::cerr << "reached ExpressionType::Invalid in "
                  << __FUNCTION__;
        __builtin_unreachable();
        break;
    }
}

std::string_view expression_type_string(ExpressionType type)
{
    switch (type) {
#define CASE_RETURN(variant) \
    case ExpressionType::variant: return #variant
        CASE_RETURN(Literal);
        CASE_RETURN(PrivateVariableDeclaration);
        CASE_RETURN(PublicVariableDeclaration);
        CASE_RETURN(PrivateConstantDeclaration);
        CASE_RETURN(PublicConstantDeclaration);
        CASE_RETURN(StructDeclaration);

        CASE_RETURN(LValue);
        CASE_RETURN(RValue);

        CASE_RETURN(If);
        CASE_RETURN(While);

        CASE_RETURN(Block);
        CASE_RETURN(PublicFunction);
        CASE_RETURN(PrivateFunction);
        CASE_RETURN(PublicCFunction);
        CASE_RETURN(PrivateCFunction);
        CASE_RETURN(FunctionCall);

        CASE_RETURN(Return);

        CASE_RETURN(ImportC);
        CASE_RETURN(InlineC);

        CASE_RETURN(Invalid);
#undef CASE_RETURN
    }
}

void PrivateVariableDeclaration::dump(
    ParsedExpressions const& expressions, std::string_view source,
    u32 indent) const
{
    std::cerr << "PrivateVariable(" << '\'' << name.text(source)
              << '\'' << " '" << type.text(source) << "' ";
    expressions[value].dump(expressions, source, indent);
    std::cerr << ')';
}

void PublicVariableDeclaration::dump(
    ParsedExpressions const& expressions, std::string_view source,
    u32 indent) const
{
    std::cerr << "PublicVariable(" << '\'' << name.text(source)
              << '\'' << " '" << type.text(source) << "' ";
    expressions[value].dump(expressions, source, indent);
    std::cerr << ')';
}

void PrivateConstantDeclaration::dump(
    ParsedExpressions const& expressions, std::string_view source,
    u32 indent) const
{
    std::cerr << "PrivateConstant(" << '\'' << name.text(source)
              << '\'' << " '" << type.text(source) << "' ";
    expressions[value].dump(expressions, source, indent);
    std::cerr << ')';
}

void PublicConstantDeclaration::dump(
    ParsedExpressions const& expressions, std::string_view source,
    u32 indent) const
{
    std::cerr << "PublicConstant(" << '\'' << name.text(source)
              << '\'' << " '" << type.text(source) << "' ";
    expressions[value].dump(expressions, source, indent);
    std::cerr << ')';
}

void StructDeclaration::dump(ParsedExpressions const&,
    std::string_view source, u32) const
{
    std::cerr << "Struct(" << '\'' << name.text(source) << "' "
              << "[ ";
    for (auto member : members) {
        auto type = member.type;
        auto name = member.name;
        std::cerr << '\'' << name.text(source) << "' '"
                  << type.text(source) << "', ";
    }
    std::cerr << "\b\b ";
    std::cerr << "])";
}

void Literal::dump(ParsedExpressions const&,
    std::string_view source, u32) const
{
    std::cerr << "Literal(" << token.text(source) << ")";
}

void LValue::dump(ParsedExpressions const&, std::string_view source,
    u32) const
{
    std::cerr << "LValue('" << token.text(source) << "')";
}

void RValue::dump(ParsedExpressions const& parsed_expressions,
    std::string_view source, u32 indent) const
{
    std::cerr << "RValue(";
    for (auto const& expression : expressions) {
        expression.dump(parsed_expressions, source, indent);
        std::cerr << ' ';
    }
    std::cerr << '\b';
    std::cerr << ')';
}

void If::dump(ParsedExpressions const& expressions,
    std::string_view source, u32 indent) const
{
    std::cerr << "If(\n";
    for (u32 i = 0; i < indent + 1; i++)
        std::cerr << ' ';
    condition.dump(expressions, source, indent + 1);
    std::cerr << '\n';
    for (u32 i = 0; i < indent + 1; i++)
        std::cerr << ' ';
    expressions[block].dump(expressions, source, indent + 1);
    std::cerr << '\n';
    for (u32 i = 0; i < indent; i++)
        std::cerr << ' ';
    std::cerr << ')';
}

void While::dump(ParsedExpressions const& expressions,
    std::string_view source, u32 indent) const
{
    std::cerr << "While(\n";
    for (u32 i = 0; i < indent + 1; i++)
        std::cerr << ' ';
    condition.dump(expressions, source, indent + 1);
    std::cerr << '\n';
    for (u32 i = 0; i < indent + 1; i++)
        std::cerr << ' ';
    expressions[block].dump(expressions, source, indent + 1);
    std::cerr << '\n';
    for (u32 i = 0; i < indent; i++)
        std::cerr << ' ';
    std::cerr << ')';
}

void PrivateFunction::dump(ParsedExpressions const& expressions,
    std::string_view source, u32 indent) const
{
    std::cerr << "PrivateFunction('";
    std::cerr << name.text(source) << "' "
              << return_type.text(source) << ' ';
    std::cerr << "[ ";
    for (auto parameter : parameters) {
        std::cerr << '\'' << parameter.name.text(source) << "': ";
        std::cerr << '\'' << parameter.type.text(source) << "' ";
    }
    std::cerr << "] ";
    expressions[block].dump(expressions, source, indent);
    std::cerr << ')';
}

void PublicFunction::dump(ParsedExpressions const& expressions,
    std::string_view source, u32 indent) const
{
    std::cerr << "PublicFunction(";
    std::cerr << '\'' << name.text(source) << "' " << '\''
              << return_type.text(source) << "' ";
    std::cerr << "[ ";
    for (auto parameter : parameters) {
        std::cerr << '\'' << parameter.name.text(source) << "': ";
        std::cerr << '\'' << parameter.type.text(source) << "' ";
    }
    std::cerr << "] ";
    expressions[block].dump(expressions, source, indent);
    std::cerr << ')';
}

void PrivateCFunction::dump(ParsedExpressions const& expressions,
    std::string_view source, u32 indent) const
{
    std::cerr << "PrivateCFunction(";
    std::cerr << '\'' << name.text(source) << "' " << '\''
              << return_type.text(source) << "' ";
    std::cerr << "[ ";
    for (auto parameter : parameters) {
        std::cerr << '\'' << parameter.name.text(source) << "': ";
        std::cerr << '\'' << parameter.type.text(source) << "' ";
    }
    std::cerr << "] ";
    expressions[block].dump(expressions, source, indent);
    std::cerr << ')';
}

void PublicCFunction::dump(ParsedExpressions const& expressions,
    std::string_view source, u32 indent) const
{
    std::cerr << "PublicCFunction(";
    std::cerr << '\'' << name.text(source) << "' " << '\''
              << return_type.text(source) << "' ";
    std::cerr << "[ ";
    for (auto parameter : parameters) {
        std::cerr << '\'' << parameter.name.text(source) << "': ";
        std::cerr << '\'' << parameter.type.text(source) << "' ";
    }
    std::cerr << "] ";
    expressions[block].dump(expressions, source, indent);
    std::cerr << ')';
}

void FunctionCall::dump(ParsedExpressions const& expressions,
    std::string_view source, u32 indent) const
{
    std::cerr << "FunctionCall(" << '\'' << name.text(source)
              << '\'' << " [";
    if (!arguments.empty()) {
        for (auto const& argument : arguments) {
            std::cerr << '\n';
            for (u32 i = 0; i < indent + 1; i++)
                std::cerr << ' ';
            argument.dump(expressions, source, indent + 1);
        }
        std::cerr << '\n';
        for (u32 i = 0; i < indent; i++)
            std::cerr << ' ';
    }
    std::cerr << "])";
}

void Block::dump(ParsedExpressions const& parsed_expressions,
    std::string_view source, u32 indent) const
{
    std::cerr << "{\n";
    for (auto const& expression : expressions) {
        for (u32 i = 0; i < indent + 1; i++)
            std::cerr << ' ';
        expression.dump(parsed_expressions, source, indent + 1);
        std::cerr << '\n';
    }
    for (u32 i = 0; i < indent; i++)
        std::cerr << ' ';
    std::cerr << '}';
}

void Return::dump(ParsedExpressions const& expressions,
    std::string_view source, u32 indent) const
{
    std::cerr << "Return(";
    rvalue.dump(expressions, source, indent);
    std::cerr << ')';
}

void ImportC::dump(ParsedExpressions const&,
    std::string_view source, u32) const
{
    std::cerr << "ImportC(" << filename.text(source) << ")";
}

void InlineC::dump(ParsedExpressions const&,
    std::string_view source, u32) const
{
    std::cerr << "InlineC('" << literal.text(source) << "')";
}

}
