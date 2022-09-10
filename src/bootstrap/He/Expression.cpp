#include <He/Expression.h>
#include <iostream>

namespace He {

void Expression::dump(std::string_view source, u32 indent) const
{
    switch (type()) {
    case ExpressionType::VariableDeclaration:
        as_variable_declaration().dump(source, indent);
        break;

    case ExpressionType::StructDeclaration:
        as_struct_declaration().dump(source, indent);
        break;

    case ExpressionType::LValue:
        as_lvalue().dump(source, indent);
        break;

    case ExpressionType::RValue:
        as_rvalue().dump(source, indent);
        break;

    case ExpressionType::If: as_if().dump(source, indent); break;

    case ExpressionType::While:
        as_while().dump(source, indent);
        break;

    case ExpressionType::Literal:
        // as_literal().dump(source, indent);
        std::cerr << "Literal(fixme)";
        break;

    case ExpressionType::Block:
        as_block().dump(source, indent);
        break;

    case ExpressionType::PublicFunction:
        as_public_function().dump(source, indent);
        break;

    case ExpressionType::PrivateFunction:
        as_private_function().dump(source, indent);
        break;

    case ExpressionType::FunctionCall:
        as_function_call().dump(source, indent);
        break;

    case ExpressionType::Return:
        as_return().dump(source, indent);
        break;

    case ExpressionType::ImportC:
        as_import_c().dump(source, indent);
        break;

    case ExpressionType::InlineC:
        as_inline_c().dump(source, indent);
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
        CASE_RETURN(VariableDeclaration);
        CASE_RETURN(StructDeclaration);

        CASE_RETURN(LValue);
        CASE_RETURN(RValue);

        CASE_RETURN(If);
        CASE_RETURN(While);

        CASE_RETURN(Block);
        CASE_RETURN(PublicFunction);
        CASE_RETURN(PrivateFunction);
        CASE_RETURN(FunctionCall);

        CASE_RETURN(Return);

        CASE_RETURN(ImportC);
        CASE_RETURN(InlineC);

        CASE_RETURN(Invalid);
#undef CASE_RETURN
    }
}


void VariableDeclaration::dump(std::string_view source,
    u32 indent) const
{
    std::cerr << "Variable(" << '\'' << name.text(source) << '\''
              << " '" << type.text(source) << "' ";
    value.dump(source, indent);
    std::cerr << ')';
}

void StructDeclaration::dump(std::string_view source, u32) const
{
    std::cerr << "Struct(" << '\'' << name.text(source) << '\''
              << "[";
    for (auto member : members) {
        auto type = member.type;
        auto name = member.name;
        std::cerr << name.text(source) << ' ' << type.text(source)
                  << ' ';
    }
    std::cerr << "])";
}

void Literal::dump(std::string_view source, u32) const
{
    std::cerr << "Literal(" << token.text(source) << ")";
}

void LValue::dump(std::string_view source, u32) const
{
    std::cerr << "LValue('" << token.text(source) << "')";
}

void RValue::dump(std::string_view source, u32 indent) const
{
    std::cerr << "RValue(";
    for (auto const& expression : expressions) {
        expression.dump(source, indent);
        std::cerr << ' ';
    }
    std::cerr << '\b';
    std::cerr << ')';
}

void If::dump(std::string_view source, u32 indent) const
{
    std::cerr << "If(\n";
    for (u32 i = 0; i < indent + 1; i++)
        std::cerr << ' ';
    condition.dump(source, indent + 1);
    std::cerr << '\n';
    for (u32 i = 0; i < indent + 1; i++)
        std::cerr << ' ';
    block.dump(source, indent + 1);
    std::cerr << '\n';
    for (u32 i = 0; i < indent; i++)
        std::cerr << ' ';
    std::cerr << ')';
}

void While::dump(std::string_view source, u32 indent) const
{
    std::cerr << "While(\n";
    for (u32 i = 0; i < indent + 1; i++)
        std::cerr << ' ';
    condition.dump(source, indent + 1);
    std::cerr << '\n';
    for (u32 i = 0; i < indent + 1; i++)
        std::cerr << ' ';
    block.dump(source, indent + 1);
    std::cerr << '\n';
    for (u32 i = 0; i < indent; i++)
        std::cerr << ' ';
    std::cerr << ')';
}

void PrivateFunction::dump(std::string_view source,
    u32 indent) const
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
    block.dump(source, indent);
    std::cerr << ')';
}

void PublicFunction::dump(std::string_view source, u32 indent) const
{
    std::cerr << "PublicFunction('";
    std::cerr << name.text(source) << "' "
              << return_type.text(source) << ' ';
    std::cerr << "[ ";
    for (auto parameter : parameters) {
        std::cerr << '\'' << parameter.name.text(source) << "': ";
        std::cerr << '\'' << parameter.type.text(source) << "' ";
    }
    std::cerr << "] ";
    block.dump(source, indent);
    std::cerr << ')';
}

void FunctionCall::dump(std::string_view source, u32 indent) const
{
    std::cerr << "FunctionCall(" << '\'' << name.text(source)
              << '\'' << " [";
    if (!arguments.empty()) {
        for (auto const& argument : arguments) {
            std::cerr << '\n';
            for (u32 i = 0; i < indent + 1; i++)
                std::cerr << ' ';
            argument.dump(source, indent + 1);
        }
        std::cerr << '\n';
        for (u32 i = 0; i < indent; i++)
            std::cerr << ' ';
    }
    std::cerr << "])";
}

void Block::dump(std::string_view source, u32 indent) const
{
    std::cerr << "{\n";
    for (auto const& expression : expressions) {
        for (u32 i = 0; i < indent + 1; i++)
            std::cerr << ' ';
        expression.dump(source, indent + 1);
        std::cerr << '\n';
    }
    for (u32 i = 0; i < indent; i++)
        std::cerr << ' ';
    std::cerr << '}';
}

void Return::dump(std::string_view source, u32 indent) const
{
    std::cerr << "Return(";
    rvalue.dump(source, indent);
    std::cerr << ')';
}

void ImportC::dump(std::string_view source, u32) const
{
    std::cerr << "ImportC(" << filename.text(source) << ")";
}

void InlineC::dump(std::string_view source, u32) const
{
    std::cerr << "InlineC('" << literal.text(source) << "')";
}

}
