#include "He/Expression.h"
#include <Core/ErrorOr.h>
#include <He/Codegen.h>
#include <He/Context.h>
#include <He/Parser.h>
#include <He/Typecheck.h>
#include <SourceFile.h>

namespace He {

Core::ErrorOr<void> TypecheckError::show(Context const&) const
{
    return Core::Error::from_string_literal("unimplemented");
}

TypecheckResult typecheck(Context& context)
{
    auto output = Codegen();
    auto& expressions = context.expressions;
    for (auto& expression : expressions.expressions) {
        if (expression.type() == ExpressionType::ImportC) {
            auto import_c = expression.release_as_import_c();
            output.import_c_filenames.push_back(import_c.filename);
        }

        if (expression.type()
            == ExpressionType::StructDeclaration) {
            auto const& declaration = context.expressions[expression.as_struct_declaration()];
            auto name = declaration.name;
            output.struct_forward_declarations.push_back({ name });
        }

        if (expression.type() == ExpressionType::InlineC) {
            auto inline_c = expression.release_as_inline_c();
            output.inline_c_expressions.push_back(inline_c.literal);
        }

        if (expression.type() == ExpressionType::PrivateFunction) {
            auto const& function
                = expressions[expression.as_private_function()];
            auto name = function.name;
            auto return_type = function.return_type;
            auto decl = FunctionForwardDeclaration { name,
                return_type, function.parameters };
            output.private_function_forward_declarations.push_back(
                decl);
            continue;
        }

        if (expression.type() == ExpressionType::PublicFunction) {
            auto const& function = expressions[expression.as_public_function()];
            auto name = function.name;
            auto return_type = function.return_type;
            auto decl = FunctionForwardDeclaration { name,
                return_type, function.parameters };
            output.public_function_forward_declarations.push_back(
                decl);
            continue;
        }
    }
    return output;
}

}
