#include "Expression.h"
#include "Parser.h"
#include <iostream>

namespace He {

void Expression::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    switch (type()) {
#define X(type_name, instance_name, ...)                    \
    case ExpressionType::type_name:                         \
        expressions[as_##instance_name()].dump(expressions, \
            source, indent);                                \
        break;

        EXPRESSIONS
#undef X
    }
}

void PrivateVariableDeclaration::dump(
    ParsedExpressions const& expressions, StringView source,
    u32 indent) const
{
    std::cerr << "PrivateVariable(" << '\'' << name.text(source)
              << '\'' << " '" << type.text(source) << "' ";
    expressions[value].dump(expressions, source, indent);
    std::cerr << ')';
}

void PublicVariableDeclaration::dump(
    ParsedExpressions const& expressions, StringView source,
    u32 indent) const
{
    std::cerr << "PublicVariable(" << '\'' << name.text(source)
              << '\'' << " '" << type.text(source) << "' ";
    expressions[value].dump(expressions, source, indent);
    std::cerr << ')';
}

void PrivateConstantDeclaration::dump(
    ParsedExpressions const& expressions, StringView source,
    u32 indent) const
{
    std::cerr << "PrivateConstant(" << '\'' << name.text(source)
              << '\'' << " '" << type.text(source) << "' ";
    expressions[value].dump(expressions, source, indent);
    std::cerr << ')';
}

void PublicConstantDeclaration::dump(
    ParsedExpressions const& expressions, StringView source,
    u32 indent) const
{
    std::cerr << "PublicConstant(" << '\'' << name.text(source)
              << '\'' << " '" << type.text(source) << "' ";
    expressions[value].dump(expressions, source, indent);
    std::cerr << ')';
}

void VariableAssignment::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    std::cerr << "VariableAssignment(" << '\'' << name.text(source)
              << "' ";
    expressions[value].dump(expressions, source, indent);
    std::cerr << ')';
}

void StructDeclaration::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    std::cerr << "Struct(" << '\'' << name.text(source) << "' "
              << "[ ";
    for (auto member : expressions[members]) {
        auto type = member.type;
        auto name = member.name;
        std::cerr << '\'' << name.text(source) << "' '"
                  << type.text(source) << "', ";
    }
    std::cerr << "\b\b ";
    std::cerr << "])";
}

void EnumDeclaration::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    std::cerr << "Enum(" << '\'' << name.text(source) << "' "
              << "[ ";
    for (auto member : expressions[members]) {
        auto type = member.type;
        auto name = member.name;
        std::cerr << '\'' << name.text(source) << "' '"
                  << type.text(source) << "', ";
    }
    std::cerr << "\b\b ";
    std::cerr << "])";
}

void UnionDeclaration::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    std::cerr << "Union(" << '\'' << name.text(source) << "' "
              << "[ ";
    for (auto member : expressions[members]) {
        auto type = member.type;
        auto name = member.name;
        std::cerr << '\'' << name.text(source) << "' '"
                  << type.text(source) << "', ";
    }
    std::cerr << "\b\b ";
    std::cerr << "])";
}

void VariantDeclaration::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    std::cerr << "Variant(" << '\'' << name.text(source) << "' "
              << "[ ";
    for (auto member : expressions[members]) {
        auto type = member.type;
        auto name = member.name;
        std::cerr << '\'' << name.text(source) << "' '"
                  << type.text(source) << "', ";
    }
    std::cerr << "\b\b ";
    std::cerr << "])";
}

void StructInitializer::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    std::cerr << "StructInitializer(" << '\'' << type.text(source)
              << "' "
              << "{ ";
    for (auto member : expressions[initializers]) {
        auto name = member.name;
        std::cerr << '\'' << name.text(source) << "' = ";
        expressions[member.value].dump(expressions, source, indent);
        std::cerr << ", ";
    }
    std::cerr << "\b\b ";
    std::cerr << "})";
}

void MemberAccess::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    std::cerr << "MemberAccess(";
    for (auto member : expressions[members]) {
        std::cerr << "'" << member.text(source) << "' ";
    }
    std::cerr << "\b)";
}

void ArrayAccess::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    std::cerr << "ArrayAccess('" << name.text(source) << "' ";
    expressions[index].dump(expressions, source, indent);
    std::cerr << ")";
}

void Literal::dump(ParsedExpressions const&, StringView source,
    u32) const
{
    std::cerr << "Literal(" << token.text(source) << ")";
}

void LValue::dump(ParsedExpressions const&, StringView source,
    u32) const
{
    std::cerr << "LValue('" << token.text(source) << "')";
}

void RValue::dump(ParsedExpressions const& parsed_expressions,
    StringView source, u32 indent) const
{
    std::cerr << "RValue(";
    for (auto const& expression : parsed_expressions[expressions]) {
        expression.dump(parsed_expressions, source, indent);
        std::cerr << ' ';
    }
    std::cerr << '\b';
    std::cerr << ')';
}

void If::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    std::cerr << "If(\n";
    for (u32 i = 0; i < indent + 1; i++)
        std::cerr << ' ';
    expressions[condition].dump(expressions, source, indent + 1);
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
    StringView source, u32 indent) const
{
    std::cerr << "While(\n";
    for (u32 i = 0; i < indent + 1; i++)
        std::cerr << ' ';
    expressions[condition].dump(expressions, source, indent + 1);
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
    StringView source, u32 indent) const
{
    std::cerr << "PrivateFunction('";
    std::cerr << name.text(source) << "' "
              << return_type.text(source) << ' ';
    std::cerr << "[ ";
    for (auto parameter : expressions[parameters]) {
        std::cerr << '\'' << parameter.name.text(source) << "': ";
        std::cerr << '\'' << parameter.type.text(source) << "' ";
    }
    std::cerr << "] ";
    expressions[block].dump(expressions, source, indent);
    std::cerr << ')';
}

void PublicFunction::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    std::cerr << "PublicFunction(";
    std::cerr << '\'' << name.text(source) << "' " << '\''
              << return_type.text(source) << "' ";
    std::cerr << "[ ";
    for (auto parameter : expressions[parameters]) {
        std::cerr << '\'' << parameter.name.text(source) << "': ";
        std::cerr << '\'' << parameter.type.text(source) << "' ";
    }
    std::cerr << "] ";
    expressions[block].dump(expressions, source, indent);
    std::cerr << ')';
}

void PrivateCFunction::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    std::cerr << "PrivateCFunction(";
    std::cerr << '\'' << name.text(source) << "' " << '\''
              << return_type.text(source) << "' ";
    std::cerr << "[ ";
    for (auto parameter : expressions[parameters]) {
        std::cerr << '\'' << parameter.name.text(source) << "': ";
        std::cerr << '\'' << parameter.type.text(source) << "' ";
    }
    std::cerr << "] ";
    expressions[block].dump(expressions, source, indent);
    std::cerr << ')';
}

void PublicCFunction::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    std::cerr << "PublicCFunction(";
    std::cerr << '\'' << name.text(source) << "' " << '\''
              << return_type.text(source) << "' ";
    std::cerr << "[ ";
    for (auto parameter : expressions[parameters]) {
        std::cerr << '\'' << parameter.name.text(source) << "': ";
        std::cerr << '\'' << parameter.type.text(source) << "' ";
    }
    std::cerr << "] ";
    expressions[block].dump(expressions, source, indent);
    std::cerr << ')';
}

void FunctionCall::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    std::cerr << "FunctionCall(" << '\'' << name.text(source)
              << '\'' << " [";
    if (!expressions[arguments].is_empty()) {
        for (auto const& argument : expressions[arguments]) {
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
    StringView source, u32 indent) const
{
    std::cerr << "{\n";
    for (auto const& expression : parsed_expressions[expressions]) {
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
    StringView source, u32 indent) const
{
    std::cerr << "Return(";
    expressions[value].dump(expressions, source, indent);
    std::cerr << ')';
}

void ImportC::dump(ParsedExpressions const&, StringView source,
    u32) const
{
    std::cerr << "ImportC(" << filename.text(source) << ")";
}

void InlineC::dump(ParsedExpressions const&, StringView source,
    u32) const
{
    std::cerr << "InlineC('" << literal.text(source) << "')";
}

void Uninitialized::dump(ParsedExpressions const&, StringView, u32)
{
    std::cerr << "Uninitialized()";
}

void Invalid::dump(ParsedExpressions const&, StringView, u32)
{
    std::cerr << "Trying to dump ExpressionType::Invalid\n";
    __builtin_abort();
}

}
