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
    out.write("PrivateVariable('"sv, name.text(source), "' '"sv,
           type.text(source), "' "sv)
        .ignore();
    expressions[value].dump(expressions, source, indent);
    out.write(")"sv).ignore();
}

void PublicVariableDeclaration::dump(
    ParsedExpressions const& expressions, StringView source,
    u32 indent) const
{
    auto& out = Core::File::stderr();
    out.write("PublicVariable('"sv, name.text(source), "' '"sv,
           type.text(source), "' "sv)
        .ignore();
    expressions[value].dump(expressions, source, indent);
    out.write(")"sv).ignore();
}

void PrivateConstantDeclaration::dump(
    ParsedExpressions const& expressions, StringView source,
    u32 indent) const
{
    auto& out = Core::File::stderr();
    out.write("PrivateConstant('"sv, name.text(source), "' '"sv,
           type.text(source), "' "sv)
        .ignore();
    expressions[value].dump(expressions, source, indent);
    out.write(")"sv).ignore();
}

void PublicConstantDeclaration::dump(
    ParsedExpressions const& expressions, StringView source,
    u32 indent) const
{
    auto& out = Core::File::stderr();
    out.write("PublicConstant('"sv, name.text(source), "' '"sv,
           type.text(source), "' "sv)
        .ignore();
    expressions[value].dump(expressions, source, indent);
    out.write(")"sv).ignore();
}

void VariableAssignment::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    out.write("VariableAssignment('"sv, name.text(source), "' "sv)
        .ignore();
    expressions[value].dump(expressions, source, indent);
    out.write(")"sv).ignore();
}

void MutableReference::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    out.write("MutableReference("sv).ignore();
    expressions[lvalue].dump(expressions, source, indent);
    out.write(")"sv).ignore();
}

void StructDeclaration::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    auto& out = Core::File::stderr();
    out.write("Struct('"sv, name.text(source), "' ["sv).ignore();
    for (auto member : expressions[members]) {
        auto type = member.type;
        auto name = member.name;
        out.write("'"sv, name.text(source), "' '"sv,
               type.text(source), "', "sv)
            .ignore();
    }
    out.write("\b\b ])"sv).ignore();
}

void EnumDeclaration::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    auto& out = Core::File::stderr();
    out.write("Enum('"sv, name.text(source), "' [ "sv).ignore();
    for (auto member : expressions[members]) {
        auto type = member.type;
        auto name = member.name;
        out.write("'"sv, name.text(source), "' '"sv,
               type.text(source), "', "sv)
            .ignore();
    }
    out.write("\b\b ])"sv).ignore();
}

void UnionDeclaration::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    auto& out = Core::File::stderr();
    out.write("Union('"sv, name.text(source), "' [ "sv).ignore();
    for (auto member : expressions[members]) {
        auto type = member.type;
        auto name = member.name;
        out.write("'"sv, name.text(source), "' '"sv,
               type.text(source), "', "sv)
            .ignore();
    }
    out.write("\b\b ])"sv).ignore();
}

void VariantDeclaration::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    auto& out = Core::File::stderr();
    out.write("Variant('"sv, name.text(source), "' [ "sv).ignore();
    for (auto member : expressions[members]) {
        auto type = member.type;
        auto name = member.name;
        out.write("'"sv, name.text(source), "' '"sv,
               type.text(source), "', "sv)
            .ignore();
    }
    out.write("\b\b ])"sv).ignore();
}

void StructInitializer::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    out.write("StructInitializer('"sv, type.text(source), "' ["sv)
        .ignore();
    for (auto member : expressions[initializers]) {
        auto name = member.name;
        out.write("\'"sv, name.text(source), "' = "sv).ignore();
        expressions[member.value].dump(expressions, source, indent);
        out.write(", "sv).ignore();
    }
    out.write("\b\b })"sv).ignore();
}

void MemberAccess::dump(ParsedExpressions const& expressions,
    StringView source, u32) const
{
    auto& out = Core::File::stderr();
    out.write("MemberAccess("sv).ignore();
    for (auto member : expressions[members]) {
        out.write("'"sv, member.text(source), "' "sv).ignore();
    }
    out.write("\b)"sv).ignore();
}

void ArrayAccess::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    out.write("ArrayAccess('"sv, name.text(source), "' "sv)
        .ignore();
    expressions[index].dump(expressions, source, indent);
    out.write(")"sv).ignore();
}

void Literal::dump(ParsedExpressions const&, StringView source,
    u32) const
{
    auto& out = Core::File::stderr();
    out.write("Literal("sv, token.text(source), ")"sv).ignore();
}

void LValue::dump(ParsedExpressions const&, StringView source,
    u32) const
{
    auto& out = Core::File::stderr();
    out.write("LValue('"sv, token.text(source), "')"sv).ignore();
}

void RValue::dump(ParsedExpressions const& parsed_expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    out.write("RValue("sv).ignore();
    for (auto const& expression : parsed_expressions[expressions]) {
        expression.dump(parsed_expressions, source, indent);
        out.write(" "sv).ignore();
    }
    out.write("\b)"sv).ignore();
}

void If::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    out.writeln("If("sv).ignore();
    for (u32 i = 0; i < indent + 1; i++)
        out.write(" "sv).ignore();
    expressions[condition].dump(expressions, source, indent + 1);
    out.writeln().ignore();
    for (u32 i = 0; i < indent + 1; i++)
        out.write(" "sv).ignore();
    expressions[block].dump(expressions, source, indent + 1);
    out.writeln().ignore();
    for (u32 i = 0; i < indent; i++)
        out.write(" "sv).ignore();
    out.write(")"sv).ignore();
}

void While::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    out.writeln("While("sv).ignore();
    for (u32 i = 0; i < indent + 1; i++)
        out.write(" "sv).ignore();
    expressions[condition].dump(expressions, source, indent + 1);
    out.writeln().ignore();
    for (u32 i = 0; i < indent + 1; i++)
        out.write(" "sv).ignore();
    expressions[block].dump(expressions, source, indent + 1);
    out.writeln().ignore();
    for (u32 i = 0; i < indent; i++)
        out.write(" "sv).ignore();
    out.write(")"sv).ignore();
}

void PrivateFunction::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    out.write("PrivateFunction('"sv, name.text(source), "' "sv,
           return_type.text(source), " ["sv)
        .ignore();
    for (auto parameter : expressions[parameters]) {
        out.write("'"sv, parameter.name.text(source), "': "sv)
            .ignore();
        out.write("'"sv, parameter.type.text(source), "' "sv)
            .ignore();
    }
    out.write("] "sv).ignore();
    expressions[block].dump(expressions, source, indent);
    out.write(")"sv).ignore();
}

void PublicFunction::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    out.write("PublicFunction('"sv, name.text(source), "' '"sv,
           return_type.text(source), "' [ "sv)
        .ignore();
    for (auto parameter : expressions[parameters]) {
        out.write("'"sv, parameter.name.text(source), "': "sv)
            .ignore();
        out.write("'"sv, parameter.type.text(source), "' "sv)
            .ignore();
    }
    out.write("] "sv).ignore();
    expressions[block].dump(expressions, source, indent);
    out.write(")"sv).ignore();
}

void PrivateCFunction::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    out.write("PrivateCFunction('"sv, name.text(source), "' '"sv,
           return_type.text(source), "' [ "sv)
        .ignore();
    for (auto parameter : expressions[parameters]) {
        out.write("'"sv, parameter.name.text(source), "': "sv)
            .ignore();
        out.write("'"sv, parameter.type.text(source), "' "sv)
            .ignore();
    }
    out.write("] "sv).ignore();
    expressions[block].dump(expressions, source, indent);
    out.write(")"sv).ignore();
}

void PublicCFunction::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    out.write("PublicCFunction('"sv, name.text(source), "' '"sv,
           return_type.text(source), "' [ "sv)
        .ignore();
    for (auto parameter : expressions[parameters]) {
        out.write("'"sv, parameter.name.text(source), "': "sv)
            .ignore();
        out.write("'"sv, parameter.type.text(source), "' "sv)
            .ignore();
    }
    out.write("] "sv).ignore();
    expressions[block].dump(expressions, source, indent);
    out.write(")"sv).ignore();
}

void FunctionCall::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    out.write("FunctionCall('"sv, name.text(source), "' ["sv)
        .ignore();
    if (!expressions[arguments].is_empty()) {
        for (auto const& argument : expressions[arguments]) {
            out.writeln().ignore();
            for (u32 i = 0; i < indent + 1; i++)
                out.write(" "sv).ignore();
            argument.dump(expressions, source, indent + 1);
        }
        out.writeln().ignore();
        for (u32 i = 0; i < indent; i++)
            out.write(" "sv).ignore();
    }
    out.write("])"sv).ignore();
}

void Block::dump(ParsedExpressions const& parsed_expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    out.writeln("{"sv).ignore();
    for (auto const& expression : parsed_expressions[expressions]) {
        for (u32 i = 0; i < indent + 1; i++)
            out.write(" "sv).ignore();
        expression.dump(parsed_expressions, source, indent + 1);
        out.writeln().ignore();
    }
    for (u32 i = 0; i < indent; i++)
        out.write(" "sv).ignore();
    out.write("}"sv).ignore();
}

void Return::dump(ParsedExpressions const& expressions,
    StringView source, u32 indent) const
{
    auto& out = Core::File::stderr();
    out.write("Return("sv).ignore();
    expressions[value].dump(expressions, source, indent);
    out.write(")"sv).ignore();
}

void Import::dump(ParsedExpressions const&, StringView source,
    u32) const
{
    auto& out = Core::File::stderr();
    out.write("Import("sv, filename.text(source), ")"sv).ignore();
}

void ImportC::dump(ParsedExpressions const&, StringView source,
    u32) const
{
    auto& out = Core::File::stderr();
    out.write("ImportC("sv, filename.text(source), ")"sv).ignore();
}

void InlineC::dump(ParsedExpressions const&, StringView source,
    u32) const
{
    auto& out = Core::File::stderr();
    out.write("InlineC('"sv, literal.text(source), "')"sv).ignore();
}

void Uninitialized::dump(ParsedExpressions const&, StringView, u32)
{
    auto& out = Core::File::stderr();
    out.write("Uninitialized()"sv).ignore();
}

void Invalid::dump(ParsedExpressions const&, StringView, u32)
{
    auto& out = Core::File::stderr();
    out.writeln("Trying to dump ExpressionType::Invalid"sv)
        .ignore();
    out.flush().ignore();
    __builtin_abort();
}

void ParsedExpressions::dump(StringView source) const
{
    auto& out = Core::File::stderr();

    for (auto import_c : import_cs) {
        import_c.dump(*this, source, 0);
        out.writeln().ignore();
    }

    for (auto inline_c : top_level_inline_cs) {
        inline_c.dump(*this, source, 0);
        out.writeln().ignore();
    }

    for (auto struct_ : struct_declarations) {
        struct_.dump(*this, source, 0);
        out.writeln().ignore();
    }

    for (auto enum_ : enum_declarations) {
        enum_.dump(*this, source, 0);
        out.writeln().ignore();
    }

    for (auto union_ : union_declarations) {
        union_.dump(*this, source, 0);
        out.writeln().ignore();
    }

    for (auto variant : variant_declarations) {
        variant.dump(*this, source, 0);
        out.writeln().ignore();
    }

    for (auto constant : top_level_private_constants) {
        constant.dump(*this, source, 0);
        out.writeln().ignore();
    }

    for (auto constant : top_level_public_constants) {
        constant.dump(*this, source, 0);
        out.writeln().ignore();
    }

    for (auto variable : top_level_private_variables) {
        variable.dump(*this, source, 0);
        out.writeln().ignore();
    }

    for (auto variable : top_level_public_variables) {
        variable.dump(*this, source, 0);
        out.writeln().ignore();
    }

    for (auto function : private_functions) {
        function.dump(*this, source, 0);
        out.writeln().ignore();
    }

    for (auto function : public_functions) {
        function.dump(*this, source, 0);
        out.writeln().ignore();
    }

    for (auto function : private_c_functions) {
        function.dump(*this, source, 0);
        out.writeln().ignore();
    }

    for (auto function : public_c_functions) {
        function.dump(*this, source, 0);
        out.writeln().ignore();
    }
}

}
