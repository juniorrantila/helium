#include "Expression.h"
#include "Parser.h"
#include <Core/File.h>

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
    auto& out = Core::File::stderr();
    (void)out.write("PrivateVariable('"sv, name.text(source),
        "' '"sv, type.text(source), "' "sv);
    expressions[value].dump(expressions, source, indent);
    (void)out.write(")"sv);
}

void PublicVariableDeclaration::dump(
    ParsedExpressions const& expressions, StringView source,
    u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.write("PublicVariable('"sv, name.text(source),
        "' '"sv, type.text(source), "' "sv);
    expressions[value].dump(expressions, source, indent);
    (void)out.write(")"sv);
}

void PrivateConstantDeclaration::dump(
    ParsedExpressions const& expressions, StringView source,
    u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.write("PrivateConstant('"sv, name.text(source),
        "' '"sv, type.text(source), "' "sv);
    expressions[value].dump(expressions, source, indent);
    (void)out.write(")"sv);
}

void PublicConstantDeclaration::dump(
    ParsedExpressions const& expressions, StringView source,
    u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.write("PublicConstant('"sv, name.text(source),
        "' '"sv, type.text(source), "' "sv);
    expressions[value].dump(expressions, source, indent);
    (void)out.write(")"sv);
}

void VariableAssignment::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.write("VariableAssignment('"sv, name.text(source),
        "' "sv);
    expressions[value].dump(expressions, source, indent);
    (void)out.write(")"sv);
}

void StructDeclaration::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    auto& out = Core::File::stderr();
    (void)out.write("Struct('"sv, name.text(source), "' ["sv);
    for (auto member : expressions[members]) {
        auto type = member.type;
        auto name = member.name;
        (void)out.write("'"sv, name.text(source), "' '"sv,
            type.text(source), "', "sv);
    }
    (void)out.write("\b\b ])"sv);
}

void EnumDeclaration::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    auto& out = Core::File::stderr();
    (void)out.write("Enum('"sv, name.text(source), "' [ "sv);
    for (auto member : expressions[members]) {
        auto type = member.type;
        auto name = member.name;
        (void)out.write("'"sv, name.text(source), "' '"sv,
            type.text(source), "', "sv);
    }
    (void)out.write("\b\b ])"sv);
}

void UnionDeclaration::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    auto& out = Core::File::stderr();
    (void)out.write("Union('"sv, name.text(source), "' [ "sv);
    for (auto member : expressions[members]) {
        auto type = member.type;
        auto name = member.name;
        (void)out.write("'"sv, name.text(source), "' '"sv,
            type.text(source), "', "sv);
    }
    (void)out.write("\b\b ])"sv);
}

void VariantDeclaration::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    auto& out = Core::File::stderr();
    (void)out.write("Variant('"sv, name.text(source), "' [ "sv);
    for (auto member : expressions[members]) {
        auto type = member.type;
        auto name = member.name;
        (void)out.write("'"sv, name.text(source), "' '"sv,
            type.text(source), "', "sv);
    }
    (void)out.write("\b\b ])"sv);
}

void StructInitializer::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.write("StructInitializer('"sv, type.text(source),
        "' ["sv);
    for (auto member : expressions[initializers]) {
        auto name = member.name;
        (void)out.write("\'"sv, name.text(source), "' = "sv);
        expressions[member.value].dump(expressions, source, indent);
        (void)out.write(", "sv);
    }
    (void)out.write("\b\b })"sv);
}

void MemberAccess::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    auto& out = Core::File::stderr();
    (void)out.write("MemberAccess("sv);
    for (auto member : expressions[members]) {
        (void)out.write("'"sv, member.text(source), "' "sv);
    }
    (void)out.write("\b)"sv);
}

void ArrayAccess::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.write("ArrayAccess('"sv, name.text(source), "' "sv);
    expressions[index].dump(expressions, source, indent);
    (void)out.write(")"sv);
}

void Literal::dump(ParsedExpressions const&, StringView source,
    u32) const
{
    auto& out = Core::File::stderr();
    (void)out.write("Literal("sv, token.text(source), ")"sv);
}

void LValue::dump(ParsedExpressions const&, StringView source,
    u32) const
{
    auto& out = Core::File::stderr();
    (void)out.write("LValue('"sv, token.text(source), "')"sv);
}

void RValue::dump(ParsedExpressions const& parsed_expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.write("RValue("sv);
    for (auto const& expression : parsed_expressions[expressions]) {
        expression.dump(parsed_expressions, source, indent);
        (void)out.write(" "sv);
    }
    (void)out.write("\b)"sv);
}

void If::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.writeln("If("sv);
    for (u32 i = 0; i < indent + 1; i++)
        (void)out.write(" "sv);
    expressions[condition].dump(expressions, source, indent + 1);
    (void)out.writeln();
    for (u32 i = 0; i < indent + 1; i++)
        (void)out.write(" "sv);
    expressions[block].dump(expressions, source, indent + 1);
    (void)out.writeln();
    for (u32 i = 0; i < indent; i++)
        (void)out.write(" "sv);
    (void)out.write(")"sv);
}

void While::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.writeln("While("sv);
    for (u32 i = 0; i < indent + 1; i++)
        (void)out.write(" "sv);
    expressions[condition].dump(expressions, source, indent + 1);
    (void)out.writeln();
    for (u32 i = 0; i < indent + 1; i++)
        (void)out.write(" "sv);
    expressions[block].dump(expressions, source, indent + 1);
    (void)out.writeln();
    for (u32 i = 0; i < indent; i++)
        (void)out.write(" "sv);
    (void)out.write(")"sv);
}

void PrivateFunction::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.write("PrivateFunction('"sv, name.text(source),
        "' "sv, return_type.text(source), " ["sv);
    for (auto parameter : expressions[parameters]) {
        (void)out.write("'"sv, parameter.name.text(source),
            "': "sv);
        (void)out.write("'"sv, parameter.type.text(source), "' "sv);
    }
    (void)out.write("] "sv);
    expressions[block].dump(expressions, source, indent);
    (void)out.write(")"sv);
}

void PublicFunction::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.write("PublicFunction('"sv, name.text(source),
        "' '"sv, return_type.text(source), "' [ "sv);
    for (auto parameter : expressions[parameters]) {
        (void)out.write("'"sv, parameter.name.text(source),
            "': "sv);
        (void)out.write("'"sv, parameter.type.text(source), "' "sv);
    }
    (void)out.write("] "sv);
    expressions[block].dump(expressions, source, indent);
    (void)out.write(")"sv);
}

void PrivateCFunction::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.write("PrivateCFunction('"sv, name.text(source),
        "' '"sv, return_type.text(source), "' [ "sv);
    for (auto parameter : expressions[parameters]) {
        (void)out.write("'"sv, parameter.name.text(source),
            "': "sv);
        (void)out.write("'"sv, parameter.type.text(source), "' "sv);
    }
    (void)out.write("] "sv);
    expressions[block].dump(expressions, source, indent);
    (void)out.write(")"sv);
}

void PublicCFunction::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.write("PublicCFunction('"sv, name.text(source),
        "' '"sv, return_type.text(source), "' [ "sv);
    for (auto parameter : expressions[parameters]) {
        (void)out.write("'"sv, parameter.name.text(source),
            "': "sv);
        (void)out.write("'"sv, parameter.type.text(source), "' "sv);
    }
    (void)out.write("] "sv);
    expressions[block].dump(expressions, source, indent);
    (void)out.write(")"sv);
}

void FunctionCall::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.write("FunctionCall('"sv, name.text(source), "' ["sv);
    if (!expressions[arguments].is_empty()) {
        for (auto const& argument : expressions[arguments]) {
            (void)out.writeln();
            for (u32 i = 0; i < indent + 1; i++)
                (void)out.write(" "sv);
            argument.dump(expressions, source, indent + 1);
        }
        (void)out.writeln();
        for (u32 i = 0; i < indent; i++)
            (void)out.write(" "sv);
    }
    (void)out.write("])"sv);
}

void Block::dump(ParsedExpressions const& parsed_expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.writeln("{"sv);
    for (auto const& expression : parsed_expressions[expressions]) {
        for (u32 i = 0; i < indent + 1; i++)
            (void)out.write(" "sv);
        expression.dump(parsed_expressions, source, indent + 1);
        (void)out.writeln();
    }
    for (u32 i = 0; i < indent; i++)
        (void)out.write(" "sv);
    (void)out.write("}"sv);
}

void Return::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    (void)out.write("Return("sv);
    expressions[value].dump(expressions, source, indent);
    (void)out.write(")"sv);
}

void ImportC::dump(ParsedExpressions const&, StringView source,
    u32) const
{
    auto& out = Core::File::stderr();
    (void)out.write("ImportC("sv, filename.text(source), ")"sv);
}

void InlineC::dump(ParsedExpressions const&, StringView source,
    u32) const
{
    auto& out = Core::File::stderr();
    (void)out.write("InlineC('"sv, literal.text(source), "')"sv);
}

void Uninitialized::dump(ParsedExpressions const&, StringView, u32)
{
    auto& out = Core::File::stderr();
    (void)out.write("Uninitialized()"sv);
}

void Invalid::dump(ParsedExpressions const&, StringView, u32)
{
    auto& out = Core::File::stderr();
    (void)out.writeln("Trying to dump ExpressionType::Invalid"sv);
    (void)out.flush();
    __builtin_abort();
}

void ParsedExpressions::dump(StringView source) const
{
    auto& out = Core::File::stderr();

    for (auto import_c : import_cs) {
        import_c.dump(*this, source, 0);
        (void)out.writeln();
    }

    for (auto inline_c : top_level_inline_cs) {
        inline_c.dump(*this, source, 0);
        (void)out.writeln();
    }

    for (auto struct_ : struct_declarations) {
        struct_.dump(*this, source, 0);
        (void)out.writeln();
    }

    for (auto enum_ : enum_declarations) {
        enum_.dump(*this, source, 0);
        (void)out.writeln();
    }

    for (auto union_ : union_declarations) {
        union_.dump(*this, source, 0);
        (void)out.writeln();
    }

    for (auto variant : variant_declarations) {
        variant.dump(*this, source, 0);
        (void)out.writeln();
    }

    for (auto constant : top_level_private_constants) {
        constant.dump(*this, source, 0);
        (void)out.writeln();
    }

    for (auto constant : top_level_public_constants) {
        constant.dump(*this, source, 0);
        (void)out.writeln();
    }

    for (auto variable : top_level_private_variables) {
        variable.dump(*this, source, 0);
        (void)out.writeln();
    }

    for (auto variable : top_level_public_variables) {
        variable.dump(*this, source, 0);
        (void)out.writeln();
    }

    for (auto function : private_functions) {
        function.dump(*this, source, 0);
        (void)out.writeln();
    }

    for (auto function : public_functions) {
        function.dump(*this, source, 0);
        (void)out.writeln();
    }

    for (auto function : private_c_functions) {
        function.dump(*this, source, 0);
        (void)out.writeln();
    }

    for (auto function : public_c_functions) {
        function.dump(*this, source, 0);
        (void)out.writeln();
    }
}

}
