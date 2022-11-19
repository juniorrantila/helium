#include "Parser.h"
#include "Context.h"
#include "Core/File.h"
#include "Expression.h"
#include "Token.h"
#include "Ty/Error.h"
#include "Util.h"
#include <Ty/ErrorOr.h>
#include <Ty/StringBuffer.h>
#include <Ty/Try.h>

namespace He {

namespace {

using ParseSingleItemResult = ErrorOr<Expression, ParseErrors>;

#define FORWARD_DECLARE_PARSER(name)                          \
    ParseSingleItemResult parse_##name(ParseErrors& errors,   \
        ParsedExpressions& expressions, Tokens const& tokens, \
        u32 start)

#define X(T, name, ...) FORWARD_DECLARE_PARSER(name);
EXPRESSIONS
#undef X

FORWARD_DECLARE_PARSER(top_level_constant_or_struct);
FORWARD_DECLARE_PARSER(if_rvalue);
FORWARD_DECLARE_PARSER(irvalue);
FORWARD_DECLARE_PARSER(prvalue);
FORWARD_DECLARE_PARSER(array_access_rvalue);
FORWARD_DECLARE_PARSER(pub_specifier);

#undef FORWARD_DECLARE_PARSER

}

ParseResult parse(Tokens const& tokens)
{
    auto errors = ParseErrors();

    auto expressions = TRY(ParsedExpressions::create());
    u32 last_error_index = 0;
    for (u32 start = 0; start < tokens.size();) {
        if (tokens[start].is(TokenType::NewLine))
            continue; // Ignore leading and trailing new lines.
        auto token = tokens[start];

        if (token.is(TokenType::Import)) {
            auto import_he = TRY(parse_import_he(errors,
                expressions, tokens, start));
            if (import_he.type() == ExpressionType::Invalid)
                last_error_index = import_he.end_token_index() - 1;
            start = import_he.end_token_index();
            continue;
        }

        if (token.is(TokenType::ImportC)) {
            auto import_c = TRY(
                parse_import_c(errors, expressions, tokens, start));
            if (import_c.type() == ExpressionType::Invalid)
                last_error_index = import_c.end_token_index() - 1;
            start = import_c.end_token_index();
            continue;
        }

        TokenType inline_cs[] {
            TokenType::InlineC,
            TokenType::InlineCBlock,
        };
        if (token.is_any_of(inline_cs)) {
            auto inline_c = TRY(
                parse_inline_c(errors, expressions, tokens, start));
            if (inline_c.type() == ExpressionType::Invalid)
                last_error_index = inline_c.end_token_index() - 1;
            start = inline_c.end_token_index();
            TRY(expressions.top_level_inline_cs.append(
                expressions[inline_c.as_inline_c()]));
            continue;
        }

        if (token.is(TokenType::Fn)) {
            auto expression = TRY(parse_private_function(errors,
                expressions, tokens, start));
            if (expression.type() == ExpressionType::Invalid)
                last_error_index = expression.end_token_index() - 1;
            start = expression.end_token_index();
            continue;
        }

        if (token.is(TokenType::CFn)) {
            auto expression = TRY(parse_private_c_function(errors,
                expressions, tokens, start));
            if (expression.type() == ExpressionType::Invalid)
                last_error_index = expression.end_token_index() - 1;
            start = expression.end_token_index();
            continue;
        }

        if (token.is(TokenType::Pub)) {
            auto pub = TRY(parse_pub_specifier(errors, expressions,
                tokens, start));
            if (pub.type() == ExpressionType::Invalid)
                last_error_index = pub.end_token_index() - 1;
            start = pub.end_token_index();
            if (pub.type()
                == ExpressionType::PublicConstantDeclaration) {
                auto constant = expressions
                    [pub.as_public_constant_declaration()];
                TRY(expressions.top_level_public_constants.append(
                    constant));
            }
            if (pub.type()
                == ExpressionType::PublicVariableDeclaration) {
                auto constant = expressions
                    [pub.as_public_constant_declaration()];
                TRY(expressions.top_level_public_constants.append(
                    constant));
            }
            continue;
        }

        if (token.is(TokenType::Let)) {
            auto expression
                = TRY(parse_top_level_constant_or_struct(errors,
                    expressions, tokens, start));
            if (expression.type() == ExpressionType::Invalid)
                last_error_index = expression.end_token_index() - 1;
            start = expression.end_token_index();
            if (expression.type()
                == ExpressionType::PrivateConstantDeclaration) {
                auto constant = expressions
                    [expression.as_private_constant_declaration()];
                TRY(expressions.top_level_private_constants.append(
                    constant));
            }
            continue;
        }

        if (token.is(TokenType::Var)) {
            auto expression
                = TRY(parse_private_variable_declaration(errors,
                    expressions, tokens, start));
            if (expression.type() == ExpressionType::Invalid)
                last_error_index = expression.end_token_index() - 1;
            start = expression.end_token_index();
            auto variable = expressions
                [expression.as_private_variable_declaration()];
            TRY(expressions.top_level_private_variables.append(
                variable));
            continue;
        }

        if (last_error_index + 1 != start) {
            // Likely false positive.
            TRY(errors.append_or_short({
                "unexpected token",
                "expected one of [import_c, inline_c, fn, "
                "c_fn, pub, let, var]",
                token,
            }));
        }
        last_error_index = start;
        start++;
    }

    if (errors.has_error())
        return errors;
    return expressions;
}

namespace {

[[maybe_unused]] ParseSingleItemResult parse_uninitialized(
    ParseErrors& errors, ParsedExpressions&, Tokens const& tokens,
    u32 start)
{
    auto uninitialized_index = start;
    auto uninitialized = tokens[uninitialized_index];
    if (uninitialized.is_not(TokenType::Uninitialized)) {
        TRY(errors.append_or_short({
            "expected @uninitialized()",
            "this is probably a parser error",
            tokens[start],
        }));
        return Expression::garbage(start, uninitialized_index);
    }

    auto open_paren_index = uninitialized_index + 1;
    auto open_paren = tokens[open_paren_index];
    if (open_paren.is_not(TokenType::OpenParen)) {
        TRY(errors.append_or_short({
            "expected '('",
            "function call need parenthesis",
            open_paren,
        }));
        return Expression::garbage(start, open_paren_index);
    }

    auto close_paren_index = open_paren_index + 1;
    auto close_paren = tokens[close_paren_index];
    if (close_paren.is_not(TokenType::CloseParen)) {
        TRY(errors.append_or_short({
            "expected ')'",
            "did you forget a closing parenthesis?",
            close_paren,
        }));
        return Expression::garbage(start, close_paren_index);
    }

    auto end = close_paren_index;
    return Expression {
        ParsedExpressions::uninitialized_expression(),
        start,
        end + 1,
    };
}

ParseSingleItemResult parse_literal(ParseErrors&,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto literal = TRY(expressions.append(Literal {
        tokens[start],
    }));
    return Expression {
        literal,
        start,
        start + 1,
    };
}

ParseSingleItemResult parse_lvalue(ParseErrors&,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto lvalue = TRY(expressions.append(LValue {
        tokens[start],
    }));

    return Expression {
        lvalue,
        start,
        start + 1,
    };
}

ParseSingleItemResult parse_struct_initializer(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto type = tokens[start];

    auto open_curly_index = start + 1;
    auto open_curly = tokens[open_curly_index];
    if (open_curly.is_not(TokenType::OpenCurly)) {
        TRY(errors.append_or_short({
            "expected '{'",
            nullptr,
            open_curly,
        }));
        return Expression::garbage(start, open_curly_index);
    }

    auto initializers_id
        = TRY(expressions.append(TRY(Initializers::create(8))));
    auto& initializers = expressions[initializers_id];
    u32 end = open_curly_index + 1;
    while (end < tokens.size()) {
        if (tokens[end].is(TokenType::CloseCurly))
            break;

        auto dot_index = end;
        auto dot = tokens[dot_index];
        if (dot.is_not(TokenType::Dot)) {
            TRY(errors.append_or_short({
                "expected '.'",
                "did you forget a dot before member name?",
                dot,
            }));
            return Expression::garbage(start, dot_index);
        }

        auto name_index = dot_index + 1;
        auto name = tokens[name_index];
        if (name.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected member name",
                nullptr,
                name,
            }));
            return Expression::garbage(start, name_index);
        }

        auto assign_index = name_index + 1;
        auto assign = tokens[assign_index];
        if (assign.is_not(TokenType::Assign)) {
            TRY(errors.append_or_short({
                "expected '='",
                nullptr,
                assign,
            }));
            return Expression::garbage(start, assign_index);
        }

        auto value_index = assign_index + 1;
        auto value = TRY(parse_irvalue(errors, expressions, tokens,
            value_index));

        auto comma_index = value.end_token_index();
        auto comma = tokens[comma_index];
        if (comma.is_not(TokenType::Comma)) {
            TRY(errors.append_or_short({
                "expected ','",
                nullptr,
                comma,
            }));
            return Expression::garbage(start, comma_index);
        }
        end = comma_index + 1; // NOTE: Consume comma.

        TRY(initializers.append(Initializer {
            .name = name,
            .value = value.as_rvalue(),
        }));
    }

    if (tokens[end].is_not(TokenType::CloseCurly)) {
        TRY(errors.append_or_short({
            "expected '}'",
            nullptr,
            tokens[end],
        }));
        return Expression::garbage(start, end);
    }

    auto struct_initializer_id
        = TRY(expressions.append(StructInitializer {
            .type = type,
            .initializers = initializers_id,
        }));

    // NOTE: Consume close curly.
    return Expression {
        struct_initializer_id,
        start,
        end + 1,
    };
}

ParseSingleItemResult parse_if_statement(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto condition = TRY(
        parse_if_rvalue(errors, expressions, tokens, start + 1));
    auto block_start_index = condition.end_token_index();
    auto block_start = tokens[block_start_index];
    if (block_start.is_not(TokenType::OpenCurly)) {
        TRY(errors.append_or_short({
            "expected '{'",
            "helium requires '{' after condition for if statements",
            block_start,
        }));
        return Expression::garbage(start, block_start_index);
    }
    auto block = TRY(parse_block(errors, expressions, tokens,
        block_start_index));

    auto end = block.end_token_index();
    auto if_statement = TRY(expressions.append(If {
        condition.release_as_rvalue(),
        block.release_as_block(),
    }));
    return Expression(if_statement, start, end);
}

ParseSingleItemResult parse_while_statement(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto condition = TRY(
        parse_if_rvalue(errors, expressions, tokens, start + 1));
    auto block_start_index = condition.end_token_index();
    auto block_start = tokens[block_start_index];
    if (block_start.is_not(TokenType::OpenCurly)) {
        TRY(errors.append_or_short({
            "expected '{'",
            "helium requires '{' after condition for loops",
            block_start,
        }));
        return Expression::garbage(start, block_start_index);
    }
    auto block = TRY(parse_block(errors, expressions, tokens,
        block_start_index));

    auto end = block.end_token_index();

    auto while_ = TRY(expressions.append(While {
        condition.release_as_rvalue(),
        block.release_as_block(),
    }));
    return Expression(while_, start, end);
}

ParseSingleItemResult parse_import_he(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto left_paren_index = start + 1;
    auto left_paren = tokens[left_paren_index];
    if (left_paren.is_not(TokenType::OpenParen)) {
        TRY(errors.append_or_short({
            "expected '('",
            "did you forget a opening parenthesis?",
            left_paren,
        }));
        return Expression::garbage(start, left_paren_index);
    }

    auto header_index = left_paren_index + 1;
    auto header = tokens[header_index];
    if (header.is_not(TokenType::Quoted)) {
        TRY(errors.append_or_short({
            "expected quoted string",
            "did you forget the quotes around the module name?",
            header,
        }));
        return Expression::garbage(start, header_index);
    }

    auto right_paren_index = header_index + 1;
    auto right_paren = tokens[right_paren_index];
    if (right_paren.is_not(TokenType::CloseParen)) {
        TRY(errors.append_or_short({
            "expected ')'",
            "did you forget a closing parenthesis?",
            left_paren,
        }));
        return Expression::garbage(start, right_paren_index);
    }

    auto semicolon_index = right_paren_index + 1;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        TRY(errors.append_or_short({
            "expected ';'",
            "did you forget a semicolon?",
            header,
        }));
        return Expression::garbage(start, semicolon_index);
    }

    auto import_he = TRY(expressions.append(Import {
        header,
    }));

    // NOTE: Swallow semicolon.
    return Expression(import_he, start, semicolon_index + 1);
}

ParseSingleItemResult parse_import_c(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto left_paren_index = start + 1;
    auto left_paren = tokens[left_paren_index];
    if (left_paren.is_not(TokenType::OpenParen)) {
        TRY(errors.append_or_short({
            "expected '('",
            "did you forget a opening parenthesis?",
            left_paren,
        }));
        return Expression::garbage(start, left_paren_index);
    }

    auto header_index = left_paren_index + 1;
    auto header = tokens[header_index];
    if (header.is_not(TokenType::Quoted)) {
        TRY(errors.append_or_short({
            "expected quoted string",
            "system headers are also imported with quotes",
            header,
        }));
        return Expression::garbage(start, header_index);
    }

    auto right_paren_index = header_index + 1;
    auto right_paren = tokens[right_paren_index];
    if (right_paren.is_not(TokenType::CloseParen)) {
        TRY(errors.append_or_short({
            "expected ')'",
            "did you forget a closing parenthesis?",
            left_paren,
        }));
        return Expression::garbage(start, right_paren_index);
    }

    auto semicolon_index = right_paren_index + 1;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        TRY(errors.append_or_short({
            "expected ';'",
            "did you forget a semicolon?",
            header,
        }));
        return Expression::garbage(start, semicolon_index);
    }

    auto import_c = TRY(expressions.append(ImportC {
        header,
    }));

    // NOTE: Swallow semicolon.
    return Expression(import_c, start, semicolon_index + 1);
}

ParseSingleItemResult parse_pub_specifier(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto fn_index = start + 1;
    auto fn = tokens[fn_index];
    if (fn.is(TokenType::Fn))
        return parse_public_function(errors, expressions, tokens,
            fn_index);
    if (fn.is(TokenType::CFn)) {
        return parse_public_c_function(errors, expressions, tokens,
            fn_index);
    }
    if (fn.is(TokenType::Let)) {
        return parse_public_constant_declaration(errors,
            expressions, tokens, fn_index);
    }
    if (fn.is(TokenType::Var)) {
        return parse_public_variable_declaration(errors,
            expressions, tokens, fn_index);
    }
    TRY(errors.append_or_short({
        "expected one of [fn, c_fn, let, var]",
        nullptr,
        fn,
    }));
    return Expression::garbage(start, fn_index);
}

struct Function {
    Token name {};
    Token return_type {};
    Id<Parameters> parameters;
    Id<Block> block;
    u32 start_token_index { 0 };
    u32 end_token_index { 0 };

    constexpr bool is_garbage() const
    {
        return name.is(TokenType::Invalid);
    }

    static constexpr Function garbage(u32 start, u32 end)
    {
        return {
            .name = { TokenType::Invalid, 0 },
            .return_type = { TokenType::Invalid, 0 },
            .parameters = Id<Parameters>::invalid(),
            .block = Id<Block>::invalid(),
            .start_token_index = start,
            .end_token_index = end + 1,
        };
    }
};

ErrorOr<Function, ParseErrors> parse_function(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.is_not(TokenType::Identifier)) {
        TRY(errors.append_or_short({
            "unexpected token",
            "expected function name",
            name,
        }));
        return Function::garbage(start, name_index);
    }

    auto parameters_start = name_index + 1;
    auto starting_paren = tokens[parameters_start];
    if (starting_paren.is_not(TokenType::OpenParen)) {
        TRY(errors.append_or_short({
            "unexpected token",
            "expected '('",
            starting_paren,
        }));
        return Function::garbage(start, parameters_start);
    }

    auto parameters_id
        = TRY(expressions.append(TRY(Parameters::create(8))));
    auto& parameters = expressions[parameters_id];
    u32 parameters_end = parameters_start + 1;
    while (parameters_end < tokens.size()) {
        auto token = tokens[parameters_end];
        if (token.is(TokenType::CloseParen))
            break;

        auto name_index = parameters_end;
        auto name = tokens[name_index];
        if (name.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected parameter name",
                nullptr,
                name,
            }));
            return Function::garbage(start, name_index);
        }

        auto colon_index = name_index + 1;
        auto colon = tokens[colon_index];
        if (colon.is_not(TokenType::Colon)) {
            TRY(errors.append_or_short({
                "expected ':'",
                nullptr,
                colon,
            }));
            return Function::garbage(start, colon_index);
        }

        auto type_index = colon_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected type name",
                nullptr,
                type_token,
            }));
            return Function::garbage(start, type_index);
        }
        TRY(parameters.append({ name, type_token }));

        auto comma_or_paren_index = type_index + 1;
        auto comma_or_paren = tokens[comma_or_paren_index];
        if (comma_or_paren.is(TokenType::Comma)) {
            // NOTE: Swallow comma.
            parameters_end = comma_or_paren_index + 1;
            continue;
        }

        if (comma_or_paren.is(TokenType::CloseParen)) {
            parameters_end = comma_or_paren_index;
            break;
        }

        TRY(errors.append_or_short({
            "unexpected token",
            nullptr,
            comma_or_paren,
        }));
        return Function::garbage(start, comma_or_paren_index);
    }
    // NOTE: Swallow paren.
    parameters_end++;

    auto arrow_index = parameters_end;
    auto arrow = tokens[arrow_index];
    if (arrow.is_not(TokenType::Arrow)) {
        TRY(errors.append_or_short({
            "unexpected token",
            "expected '->'",
            arrow,
        }));
        return Function::garbage(start, arrow_index);
    }

    auto return_type_index = arrow_index + 1;
    auto return_type = tokens[return_type_index];
    if (return_type.is_not(TokenType::Identifier)) {
        TRY(errors.append_or_short({
            "unexpected token",
            "expected return type",
            return_type,
        }));
        return Function::garbage(start, return_type_index);
    }

    auto block_start_index = return_type_index + 1;
    auto block_start = tokens[block_start_index];
    if (block_start.is_not(TokenType::OpenCurly)) {
        TRY(errors.append_or_short({
            "unexpected token",
            "expected '{'",
            block_start,
        }));
        return Function::garbage(start, block_start_index);
    }

    auto block = TRY(parse_block(errors, expressions, tokens,
        block_start_index));
    auto block_end_index = block.end_token_index();

    return Function {
        name,
        return_type,
        parameters_id,
        block.release_as_block(),
        start,
        block_end_index,
    };
}

ParseSingleItemResult parse_public_function(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto function
        = TRY(parse_function(errors, expressions, tokens, start));
    if (function.is_garbage()) {
        return Expression::garbage(function.start_token_index,
            function.end_token_index - 1);
    }

    auto function_id = TRY(expressions.append(PublicFunction {
        .name = function.name,
        .return_type = function.return_type,
        .parameters = function.parameters,
        .block = function.block,
    }));
    return Expression {
        function_id,
        function.start_token_index,
        function.end_token_index,
    };
}

ParseSingleItemResult parse_public_c_function(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto function
        = TRY(parse_function(errors, expressions, tokens, start));
    if (function.is_garbage()) {
        return Expression::garbage(function.start_token_index,
            function.end_token_index - 1);
    }
    auto function_id = TRY(expressions.append(PublicCFunction {
        .name = function.name,
        .return_type = function.return_type,
        .parameters = function.parameters,
        .block = function.block,
    }));
    return Expression {
        function_id,
        function.start_token_index,
        function.end_token_index,
    };
}

ParseSingleItemResult parse_private_function(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto function
        = TRY(parse_function(errors, expressions, tokens, start));
    if (function.is_garbage()) {
        return Expression::garbage(function.start_token_index,
            function.end_token_index - 1);
    }
    auto function_id = TRY(expressions.append(PrivateFunction {
        .name = function.name,
        .return_type = function.return_type,
        .parameters = function.parameters,
        .block = function.block,
    }));
    return Expression {
        function_id,
        function.start_token_index,
        function.end_token_index,
    };
}

ParseSingleItemResult parse_private_c_function(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto function
        = TRY(parse_function(errors, expressions, tokens, start));
    if (function.is_garbage()) {
        return Expression::garbage(function.start_token_index,
            function.end_token_index - 1);
    }
    auto function_id = TRY(expressions.append(PrivateCFunction {
        .name = function.name,
        .return_type = function.return_type,
        .parameters = function.parameters,
        .block = function.block,
    }));
    return Expression {
        function_id,
        function.start_token_index,
        function.end_token_index,
    };
}

ParseSingleItemResult parse_function_call(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto function_name = tokens[start];

    auto left_paren_index = start + 1;
    auto left_paren = tokens[left_paren_index];
    if (left_paren.is_not(TokenType::OpenParen)) {
        TRY(errors.append_or_short({
            "expected left parenthesis",
            "did you mean to do a function call?",
            left_paren,
        }));
        return Expression::garbage(start, left_paren_index);
    }

    auto arguments_id
        = TRY(expressions.append(TRY(Expressions::create(8))));
    auto call_id = TRY(expressions.append(FunctionCall {
        .name = function_name,
        .arguments = arguments_id,
    }));
    auto& call = expressions[call_id];
    auto right_paren_index = left_paren_index + 1;
    if (tokens[right_paren_index].is(TokenType::CloseParen)) {
        // NOTE: Swallow right parenthesis
        return Expression(call_id, start, right_paren_index + 1);
    }

    right_paren_index = left_paren_index;
    for (; right_paren_index < tokens.size();) {
        if (tokens[right_paren_index].is(TokenType::CloseParen))
            break;
        auto argument_index = right_paren_index + 1;
        auto argument = TRY(parse_prvalue(errors, expressions,
            tokens, argument_index));
        right_paren_index = argument.end_token_index();
        TRY(expressions[call.arguments].append(argument));
    }

    auto right_paren = tokens[right_paren_index];
    if (right_paren.is_not(TokenType::CloseParen)) {
        TRY(errors.append_or_short({
            "expected right parenthesis",
            "did you mean to do a function call?",
            right_paren,
        }));
        return Expression::garbage(start, right_paren_index);
    }

    // NOTE: Swallow right parenthesis
    return Expression(call_id, start, right_paren_index + 1);
}

ParseSingleItemResult parse_return_statement(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.is(TokenType::Identifier)) {
        auto open_curly_index = name_index + 1;
        auto open_curly = tokens[open_curly_index];
        if (open_curly.is(TokenType::OpenCurly)) {
            auto value = TRY(parse_struct_initializer(errors,
                expressions, tokens, name_index));
            auto value_id = TRY(expressions.append(value));
            auto return_id = TRY(expressions.append(Return {
                value_id,
            }));
            return Expression {
                return_id,
                name_index,
                value.end_token_index(),
            };
        }
    }

    auto value
        = TRY(parse_rvalue(errors, expressions, tokens, start + 1));
    auto value_id = TRY(expressions.append(value));
    auto end = value.end_token_index();

    auto return_id = TRY(expressions.append(Return {
        value_id,
    }));
    return Expression {
        return_id,
        start,
        end,
    };
}

ParseSingleItemResult parse_inline_c(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto end = start + 1;
    auto inline_c = TRY(expressions.append(InlineC {
        tokens[start],
    }));
    if (tokens[end].is(TokenType::CloseCurly)) {
        end++;
    }
    if (tokens[end].is_not(TokenType::Semicolon)) {
        TRY(errors.append_or_short(ParseError {
            "unexpected token",
            "did you forget a semicolon?",
            tokens[end],
        }));
        return Expression::garbage(start, end);
    }
    // NOTE: Swallow semicolon.
    return Expression(inline_c, start, end + 1);
}

ParseSingleItemResult parse_block(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto block = TRY(expressions.create_block());
    auto end = start + 1;
    for (; end < tokens.size();) {
        if (tokens[end].is(TokenType::CloseCurly))
            break;

        if (tokens[end].is(TokenType::InlineC)) {
            auto inline_c = TRY(
                parse_inline_c(errors, expressions, tokens, end));
            end = inline_c.end_token_index();
            TRY(expressions[block.expressions].append(inline_c));
            continue;
        }

        if (tokens[end].is(TokenType::OpenCurly)) {
            auto sub_block = TRY(
                parse_block(errors, expressions, tokens, end));
            end = sub_block.end_token_index();
            TRY(expressions[block.expressions].append(sub_block));
            continue;
        }

        if (tokens[end].is(TokenType::Let)) {
            auto variable = TRY(parse_public_constant_declaration(
                errors, expressions, tokens, end));
            end = variable.end_token_index();
            TRY(expressions[block.expressions].append(variable));
            continue;
        }

        if (tokens[end].is(TokenType::Var)) {
            auto variable = TRY(parse_public_variable_declaration(
                errors, expressions, tokens, end));
            end = variable.end_token_index();
            TRY(expressions[block.expressions].append(variable));
            continue;
        }

        if (tokens[end].is(TokenType::Return)) {
            auto return_expression = TRY(parse_return_statement(
                errors, expressions, tokens, end));
            end = return_expression.end_token_index();

            if (tokens[end].is_not(TokenType::Semicolon)) {
                TRY(errors.append_or_short({
                    "expected ';'",
                    "did you forget a semicolon?",
                    tokens[end],
                }));
                return Expression::garbage(start, end);
            }
            end++; // NOTE: Swallow semicolon.

            TRY(expressions[block.expressions].append(
                return_expression));
            continue;
        }

        if (tokens[end].is(TokenType::Identifier)) {
            if (tokens[end + 1].is(TokenType::Assign)) {
                auto assignment = TRY(parse_variable_assignment(
                    errors, expressions, tokens, end));
                TRY(expressions[block.expressions].append(
                    assignment));
                end = assignment.end_token_index();
                continue;
            }

            auto call = TRY(parse_function_call(errors, expressions,
                tokens, end));
            end = call.end_token_index();
            TRY(expressions[block.expressions].append(call));

            if (tokens[end].is_not(TokenType::Semicolon)) {
                TRY(errors.append_or_short({
                    "expected ';'",
                    "did you forget a semicolon?",
                    tokens[end],
                }));
                return Expression::garbage(start, end);
            }
            end++; // NOTE: Swallow semicolon.
            continue;
        }

        if (tokens[end].is(TokenType::If)) {
            auto if_ = TRY(parse_if_statement(errors, expressions,
                tokens, end));
            end = if_.end_token_index();
            TRY(expressions[block.expressions].append(if_));
            continue;
        }

        if (tokens[end].is(TokenType::While)) {
            auto while_ = TRY(parse_while_statement(errors,
                expressions, tokens, end));
            end = while_.end_token_index();
            TRY(expressions[block.expressions].append(while_));
            continue;
        }

        TRY(errors.append_or_short({
            "unexpected token",
            nullptr,
            tokens[end],
        }));
        return Expression::garbage(start, end);
    }
    auto block_id = TRY(expressions.append(block));
    // NOTE: Swallow close curly
    return Expression(block_id, start, end + 1);
}

ParseSingleItemResult parse_irvalue(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto rvalue = TRY(expressions.create_rvalue());

    auto end = start;
    for (; end < tokens.size();) {
        if (tokens[end].is(TokenType::Comma))
            break;
        if (tokens[end].is(TokenType::OpenCurly))
            break;

        if (tokens[end].is(TokenType::Identifier)) {
            if (tokens[end + 1].is(TokenType::OpenCurly)) {
                auto initializer = TRY(parse_struct_initializer(
                    errors, expressions, tokens, end));
                end = initializer.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    initializer));
                continue;
            }
        }

        if (tokens[end].is(TokenType::InlineC)) {
            auto inline_c = TRY(
                parse_inline_c(errors, expressions, tokens, end));
            end = inline_c.end_token_index();
            TRY(expressions[rvalue.expressions].append(inline_c));
            auto rvalue_id = TRY(expressions.append(rvalue));
            // NOTE: Unconsume ';'
            return Expression {
                rvalue_id,
                start,
                end - 1,
            };
        }

        if (tokens[end].is(TokenType::Uninitialized)) {
            auto uninitialized = TRY(parse_uninitialized(errors,
                expressions, tokens, end));
            auto start = end;
            end = uninitialized.end_token_index();
            TRY(expressions[rvalue.expressions].append({
                uninitialized.as_uninitialized(),
                start,
                end,
            }));
            continue;
        }

        if (tokens[end].is(TokenType::Number)) {
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        TokenType binary_operators[] = {
            TokenType::Plus,
            TokenType::Minus,
            TokenType::Star,

            TokenType::Equals,
            TokenType::LessThan,
            TokenType::LessThanOrEqual,
            TokenType::GreaterThan,
            TokenType::GreaterThanOrEqual,
        };
        if (tokens[end].is_any_of(binary_operators)) {
            // FIXME: Create parse_binary_operator.
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Quoted)) {
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Identifier)) {
            if (tokens[end + 1].is(TokenType::OpenParen)) {
                auto call = TRY(parse_function_call(errors,
                    expressions, tokens, end));
                end = call.end_token_index();
                TRY(expressions[rvalue.expressions].append(call));
                continue;
            }
            if (tokens[end + 1].is(TokenType::OpenCurly)) {
                auto initializer = TRY(parse_struct_initializer(
                    errors, expressions, tokens, start));
                end = initializer.end_token_index() + 1;
                TRY(expressions[rvalue.expressions].append(
                    initializer));
                continue;
            }
            if (tokens[end + 1].is(TokenType::Dot)) {
                auto member_access = TRY(parse_member_access(errors,
                    expressions, tokens, end));
                end = member_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    member_access));
                continue;
            }

            auto lvalue = TRY(
                parse_lvalue(errors, expressions, tokens, end));
            end = lvalue.end_token_index();
            TRY(expressions[rvalue.expressions].append(lvalue));
            continue;
        }

        TRY(errors.append_or_short({
            "expected ',' or '{'",
            "did you forget a comma?",
            tokens[end],
        }));
        return Expression::garbage(start, end);
    }

    if (tokens[end].is(TokenType::Comma)) {
        auto rvalue_id = TRY(expressions.append(rvalue));
        return Expression {
            rvalue_id,
            start,
            end,
        };
    }

    if (tokens[end].is(TokenType::OpenCurly)) {
        auto rvalue_id = TRY(expressions.append(rvalue));
        return Expression {
            rvalue_id,
            start,
            end,
        };
    }

    TRY(errors.append_or_short({
        "expected ',' or '{'",
        "did you forget a comma?",
        tokens[end],
    }));
    return Expression::garbage(start, end);
}

ParseSingleItemResult parse_if_rvalue(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto rvalue = TRY(expressions.create_rvalue());

    auto end = start;
    for (; end < tokens.size();) {
        if (tokens[end].is(TokenType::Semicolon))
            break;
        if (tokens[end].is(TokenType::OpenCurly))
            break;

        if (tokens[end].is(TokenType::InlineC)) {
            auto inline_c = TRY(
                parse_inline_c(errors, expressions, tokens, end));
            end = inline_c.end_token_index();
            TRY(expressions[rvalue.expressions].append(inline_c));
            auto rvalue_id = TRY(expressions.append(rvalue));
            // NOTE: Unconsume ';'
            return Expression {
                rvalue_id,
                start,
                end - 1,
            };
        }

        if (tokens[end].is(TokenType::Uninitialized)) {
            auto uninitialized = TRY(parse_uninitialized(errors,
                expressions, tokens, end));
            auto start = end;
            end = uninitialized.end_token_index();
            TRY(expressions[rvalue.expressions].append({
                uninitialized.as_uninitialized(),
                start,
                end,
            }));
            continue;
        }

        if (tokens[end].is(TokenType::Number)) {
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        TokenType binary_operators[] = {
            TokenType::Plus,
            TokenType::Minus,
            TokenType::Star,

            TokenType::Equals,
            TokenType::LessThan,
            TokenType::LessThanOrEqual,
            TokenType::GreaterThan,
            TokenType::GreaterThanOrEqual,
        };
        if (tokens[end].is_any_of(binary_operators)) {
            // FIXME: Create parse_binary_operator.
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Quoted)) {
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Identifier)) {
            if (tokens[end + 1].is(TokenType::OpenParen)) {
                auto call = TRY(parse_function_call(errors,
                    expressions, tokens, end));
                end = call.end_token_index();
                TRY(expressions[rvalue.expressions].append(call));
                continue;
            }
            if (tokens[end + 1].is(TokenType::Dot)) {
                auto member_access = TRY(parse_member_access(errors,
                    expressions, tokens, end));
                end = member_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    member_access));
                continue;
            }

            auto lvalue = TRY(
                parse_lvalue(errors, expressions, tokens, end));
            end = lvalue.end_token_index();
            TRY(expressions[rvalue.expressions].append(lvalue));
            continue;
        }

        TRY(errors.append_or_short({
            "expected ';' or '{'",
            "did you forget a semicolon?",
            tokens[end],
        }));
        return Expression::garbage(start, end);
    }

    if (tokens[end].is(TokenType::Semicolon)) {
        auto rvalue_id = TRY(expressions.append(rvalue));
        return Expression {
            rvalue_id,
            start,
            end,
        };
    }

    if (tokens[end].is(TokenType::OpenCurly)) {
        auto rvalue_id = TRY(expressions.append(rvalue));
        return Expression {
            rvalue_id,
            start,
            end,
        };
    }

    TRY(errors.append_or_short({
        "expected ';' or '{'",
        "did you forget a semicolon?",
        tokens[end],
    }));
    return Expression::garbage(start, end);
}

ParseSingleItemResult parse_rvalue(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto rvalue = TRY(expressions.create_rvalue());

    auto end = start;
    for (; end < tokens.size();) {
        if (tokens[end].is(TokenType::Semicolon))
            break;

        if (tokens[end].is(TokenType::InlineC)) {
            auto inline_c = TRY(
                parse_inline_c(errors, expressions, tokens, end));
            end = inline_c.end_token_index();
            TRY(expressions[rvalue.expressions].append(inline_c));
            auto rvalue_id = TRY(expressions.append(rvalue));
            // NOTE: Unconsume ';'
            return Expression {
                rvalue_id,
                start,
                end - 1,
            };
        }

        if (tokens[end].is(TokenType::Uninitialized)) {
            auto uninitialized = TRY(parse_uninitialized(errors,
                expressions, tokens, end));
            auto start = end;
            end = uninitialized.end_token_index();
            TRY(expressions[rvalue.expressions].append({
                uninitialized.as_uninitialized(),
                start,
                end,
            }));
            continue;
        }

        if (tokens[end].is(TokenType::Number)) {
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        TokenType binary_operators[] = {
            TokenType::Plus,
            TokenType::Minus,
            TokenType::Star,

            TokenType::Equals,
            TokenType::LessThan,
            TokenType::LessThanOrEqual,
            TokenType::GreaterThan,
            TokenType::GreaterThanOrEqual,
        };
        if (tokens[end].is_any_of(binary_operators)) {
            // FIXME: Create parse_binary_operator.
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Quoted)) {
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Identifier)) {
            if (tokens[end + 1].is(TokenType::OpenParen)) {
                auto call = TRY(parse_function_call(errors,
                    expressions, tokens, end));
                end = call.end_token_index();
                TRY(expressions[rvalue.expressions].append(call));
                continue;
            }
            if (tokens[end + 1].is(TokenType::OpenCurly)) {
                auto initializer = TRY(parse_struct_initializer(
                    errors, expressions, tokens, end));
                end = initializer.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    initializer));
                continue;
            }
            if (tokens[end + 1].is(TokenType::Dot)) {
                auto member_access = TRY(parse_member_access(errors,
                    expressions, tokens, end));
                end = member_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    member_access));
                continue;
            }

            if (tokens[end + 1].is(TokenType::OpenBracket)) {
                auto array_access = TRY(parse_array_access(errors,
                    expressions, tokens, end));
                end = array_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    array_access));
                continue;
            }

            auto lvalue = TRY(
                parse_lvalue(errors, expressions, tokens, end));
            end = lvalue.end_token_index();
            TRY(expressions[rvalue.expressions].append(lvalue));
            continue;
        }

        if (expressions[rvalue.expressions].is_empty()) {
            TRY(errors.append_or_short({
                "expected a value",
                nullptr,
                tokens[end],
            }));
            return Expression::garbage(start, end);
        }
        TRY(errors.append_or_short({
            "expected ';'",
            "did you forget a semicolon?",
            tokens[end],
        }));
        return Expression::garbage(start, end);
    }

    if (tokens[end].is(TokenType::Semicolon)) {
        auto rvalue_id = TRY(expressions.append(rvalue));
        return Expression {
            rvalue_id,
            start,
            end,
        };
    }

    if (tokens[end].is(TokenType::OpenCurly)) {
        auto rvalue_id = TRY(expressions.append(rvalue));
        return Expression {
            rvalue_id,
            start,
            end,
        };
    }

    TRY(errors.append_or_short({
        "expected ';' or '{'",
        "did you forget a semicolon?",
        tokens[end],
    }));
    return Expression::garbage(start, end);
}

ParseSingleItemResult parse_array_access_rvalue(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto rvalue = TRY(expressions.create_rvalue());

    auto end = start;
    for (; end < tokens.size();) {
        if (tokens[end].is(TokenType::CloseBracket))
            break;

        if (tokens[end].is(TokenType::Number)) {
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        TokenType binary_operators[] = {
            TokenType::Plus,
            TokenType::Minus,
            TokenType::Star,

            TokenType::Equals,
            TokenType::LessThan,
            TokenType::LessThanOrEqual,
            TokenType::GreaterThan,
            TokenType::GreaterThanOrEqual,
        };
        if (tokens[end].is_any_of(binary_operators)) {
            // FIXME: Create parse_binary_operator.
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Quoted)) {
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Identifier)) {
            if (tokens[end + 1].is(TokenType::OpenParen)) {
                auto call = TRY(parse_function_call(errors,
                    expressions, tokens, end));
                end = call.end_token_index();
                TRY(expressions[rvalue.expressions].append(call));
                continue;
            }
            if (tokens[end + 1].is(TokenType::OpenCurly)) {
                auto initializer = TRY(parse_struct_initializer(
                    errors, expressions, tokens, end));
                end = initializer.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    initializer));
                continue;
            }
            if (tokens[end + 1].is(TokenType::Dot)) {
                auto member_access = TRY(parse_member_access(errors,
                    expressions, tokens, end));
                end = member_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    member_access));
                continue;
            }

            if (tokens[end + 1].is(TokenType::OpenBracket)) {
                auto array_access = TRY(parse_array_access(errors,
                    expressions, tokens, start));
                end = array_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    array_access));
                continue;
            }

            auto lvalue = TRY(
                parse_lvalue(errors, expressions, tokens, end));
            end = lvalue.end_token_index();
            TRY(expressions[rvalue.expressions].append(lvalue));
            continue;
        }

        TRY(errors.append_or_short({
            "expected ']'",
            nullptr,
            tokens[end],
        }));
        return Expression::garbage(start, end);
    }

    if (tokens[end].is_not(TokenType::CloseBracket)) {
        TRY(errors.append_or_short({
            "expected ']'",
            nullptr,
            tokens[end],
        }));
        return Expression::garbage(start, end);
    }

    auto rvalue_id = TRY(expressions.append(rvalue));
    return Expression {
        rvalue_id,
        start,
        end,
    };
}

ParseSingleItemResult parse_prvalue(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto rvalue = TRY(expressions.create_rvalue());

    auto end = start;
    for (; end < tokens.size();) {
        if (tokens[end].is(TokenType::Comma))
            break;
        if (tokens[end].is(TokenType::CloseParen))
            break;

        if (tokens[end].is(TokenType::Number)) {
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::RefMut)) {
            auto refmut = TRY(parse_mutable_reference(errors,
                expressions, tokens, end + 1));
            TRY(expressions[rvalue.expressions].append(refmut));
            end = refmut.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Ampersand)) {
            auto token = tokens[end];
            auto literal = TRY(expressions.append(Literal {
                token,
            }));
            TRY(expressions[rvalue.expressions].append({
                literal,
                end,
                end + 1,
            }));
            end = end + 1;
            continue;
        }

        TokenType binary_operators[] = {
            TokenType::Plus,
            TokenType::Minus,
            TokenType::Star,

            TokenType::Equals,
            TokenType::LessThan,
            TokenType::LessThanOrEqual,
            TokenType::GreaterThan,
            TokenType::GreaterThanOrEqual,
        };
        if (tokens[end].is_any_of(binary_operators)) {
            // FIXME: Create parse_binary_operator.
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Quoted)) {
            auto literal = TRY(
                parse_literal(errors, expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Identifier)) {
            if (tokens[end + 1].is(TokenType::OpenParen)) {
                auto function_call = TRY(parse_function_call(errors,
                    expressions, tokens, end));
                end = function_call.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    function_call));
                continue;
            }
            if (tokens[end + 1].is(TokenType::Dot)) {
                auto member_access = TRY(parse_member_access(errors,
                    expressions, tokens, end));
                end = member_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    member_access));
                continue;
            }
            if (tokens[end + 1].is(TokenType::OpenBracket)) {
                auto array_access = TRY(parse_array_access(errors,
                    expressions, tokens, end));
                end = array_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    array_access));
                continue;
            }

            auto lvalue = TRY(
                parse_lvalue(errors, expressions, tokens, end));
            end = lvalue.end_token_index();
            TRY(expressions[rvalue.expressions].append(lvalue));
            continue;
        }

        TRY(errors.append_or_short({
            "expected ',' or ')'",
            "did you forget a comma?",
            tokens[end],
        }));
        return Expression::garbage(start, end);
    }

    if (tokens[end].is(TokenType::Comma)) {
        auto rvalue_id = TRY(expressions.append(rvalue));
        return Expression {
            rvalue_id,
            start,
            end,
        };
    }
    if (tokens[end].is(TokenType::CloseParen)) {
        auto rvalue_id = TRY(expressions.append(rvalue));
        return Expression {
            rvalue_id,
            start,
            end,
        };
    }

    TRY(errors.append_or_short({
        "expected ',' or ')'",
        "did you forget a comma?",
        tokens[end],
    }));
    return Expression::garbage(start, end);
}

ParseSingleItemResult parse_mutable_reference(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto lvalue
        = TRY(parse_lvalue(errors, expressions, tokens, start));
    auto ref_id = TRY(expressions.append(MutableReference {
        lvalue.as_lvalue(),
    }));
    return Expression(ref_id, start, lvalue.end_token_index());
}

ParseSingleItemResult parse_struct_declaration(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto name = tokens[start];

    auto assign_index = start + 1;
    auto assign = tokens[assign_index];
    if (assign.is_not(TokenType::Assign)) {
        auto const* hint = "struct declarations can't have colon "
                           "in this position";
        if (assign.is_not(TokenType::Colon))
            hint = nullptr;
        TRY(errors.append_or_short({
            "expected '='",
            hint,
            assign,
        }));
        return Expression::garbage(start, assign_index);
    }

    auto struct_token_index = assign_index + 1;
    auto struct_token = tokens[struct_token_index];
    if (struct_token.is_not(TokenType::Struct)) {
        TRY(errors.append_or_short({
            "expected 'struct'",
            nullptr,
            struct_token,
        }));
        return Expression::garbage(start, struct_token_index);
    }

    auto block_start_index = struct_token_index + 1;
    auto block_start = tokens[block_start_index];
    if (block_start.is_not(TokenType::OpenCurly)) {
        TRY(errors.append_or_short({
            "expected '{'",
            nullptr,
            block_start,
        }));
        return Expression::garbage(start, block_start_index);
    }

    auto members_id
        = TRY(expressions.append(TRY(Members::create(8))));
    auto& members = expressions[members_id];

    auto block_end_index = block_start_index + 1;
    while (block_end_index < tokens.size()) {
        auto block_end = tokens[block_end_index];
        if (block_end.is(TokenType::CloseCurly))
            break;

        auto member_name_index = block_end_index;
        auto member_name = tokens[member_name_index];
        if (member_name.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected name of member",
                nullptr,
                member_name,
            }));
            return Expression::garbage(start, member_name_index);
        }

        auto colon_index = member_name_index + 1;
        auto colon = tokens[colon_index];
        if (colon.is_not(TokenType::Colon)) {
            TRY(errors.append_or_short({
                "expected ':'",
                nullptr,
                colon,
            }));
            return Expression::garbage(start, colon_index);
        }

        auto type_index = colon_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected type name",
                nullptr,
                type_token,
            }));
            return Expression::garbage(start, type_index);
        }

        auto comma_index = type_index + 1;
        auto comma = tokens[comma_index];
        if (comma.is_not(TokenType::Comma)) {
            TRY(errors.append_or_short({
                "expected ','",
                "did you forget a comma?",
                comma,
            }));
            return Expression::garbage(start, comma_index);
        }

        auto member = Member {
            .name = member_name,
            .type = type_token,
        };
        TRY(members.append(member));
        block_end_index = comma_index + 1;
    }

    auto block_end = tokens[block_end_index];
    if (block_end.is_not(TokenType::CloseCurly)) {
        TRY(errors.append_or_short({
            "expected '}'",
            nullptr,
            block_end,
        }));
        return Expression::garbage(start, block_end_index);
    }

    auto semicolon_index = block_end_index + 1;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        TRY(errors.append_or_short({
            "expected ';'",
            "did you forget a semicolon?",
            semicolon,
        }));
        return Expression::garbage(start, semicolon_index);
    }

    auto struct_declaration = StructDeclaration {
        .name = name,
        .members = members_id,
    };
    // NOTE: Swallow semicolon.
    auto end = semicolon_index + 1;
    auto struct_id = TRY(expressions.append(struct_declaration));
    return Expression(struct_id, start, end);
}

ParseSingleItemResult parse_enum_declaration(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto name = tokens[start];

    auto assign_index = start + 1;
    auto assign = tokens[assign_index];
    if (assign.is_not(TokenType::Assign)) {
        auto const* hint = "struct declarations can't have colon "
                           "in this position";
        if (assign.is_not(TokenType::Colon))
            hint = nullptr;
        TRY(errors.append_or_short({
            "expected '='",
            hint,
            assign,
        }));
        return Expression::garbage(start, assign_index);
    }

    auto enum_token_index = assign_index + 1;
    auto enum_token = tokens[enum_token_index];
    if (enum_token.is_not(TokenType::Enum)) {
        TRY(errors.append_or_short({
            "expected 'enum'",
            nullptr,
            enum_token,
        }));
        return Expression::garbage(start, enum_token_index);
    }

    auto block_start_index = enum_token_index + 1;
    auto block_start = tokens[block_start_index];
    if (block_start.is_not(TokenType::OpenCurly)) {
        TRY(errors.append_or_short({
            "expected '{'",
            nullptr,
            block_start,
        }));
        return Expression::garbage(start, block_start_index);
    }

    auto members_id
        = TRY(expressions.append(TRY(Members::create(8))));
    auto& members = expressions[members_id];

    auto block_end_index = block_start_index + 1;
    while (block_end_index < tokens.size()) {
        auto block_end = tokens[block_end_index];
        if (block_end.is(TokenType::CloseCurly))
            break;

        auto member_name_index = block_end_index;
        auto member_name = tokens[member_name_index];
        if (member_name.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected name of member",
                nullptr,
                member_name,
            }));
            return Expression::garbage(start, member_name_index);
        }

        auto comma_index = member_name_index + 1;
        auto comma = tokens[comma_index];
        if (comma.is_not(TokenType::Comma)) {
            TRY(errors.append_or_short({
                "expected ','",
                "did you forget a comma?",
                comma,
            }));
            return Expression::garbage(start, comma_index);
        }

        auto member = Member {
            .name = member_name,
            .type = Token(), // FIXME
        };
        TRY(members.append(member));
        block_end_index = comma_index + 1;
    }

    auto block_end = tokens[block_end_index];
    if (block_end.is_not(TokenType::CloseCurly)) {
        TRY(errors.append_or_short({
            "expected '}'",
            nullptr,
            block_end,
        }));
        return Expression::garbage(start, block_end_index);
    }

    auto semicolon_index = block_end_index + 1;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        TRY(errors.append_or_short({
            "expected ';'",
            "did you forget a semicolon?",
            semicolon,
        }));
        return Expression::garbage(start, semicolon_index);
    }

    auto enum_declaration = EnumDeclaration {
        .name = name,
        .underlying_type = Token(), // FIXME
        .members = members_id,
    };
    // NOTE: Swallow semicolon.
    auto end = semicolon_index + 1;
    auto enum_id = TRY(expressions.append(enum_declaration));
    return Expression(enum_id, start, end);
}

ParseSingleItemResult parse_union_declaration(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto name = tokens[start];

    auto assign_index = start + 1;
    auto assign = tokens[assign_index];
    if (assign.is_not(TokenType::Assign)) {
        auto const* hint = "struct declarations can't have colon "
                           "in this position";
        if (assign.is_not(TokenType::Colon))
            hint = nullptr;
        TRY(errors.append_or_short({
            "expected '='",
            hint,
            assign,
        }));
        return Expression::garbage(start, assign_index);
    }

    auto union_token_index = assign_index + 1;
    auto union_token = tokens[union_token_index];
    if (union_token.is_not(TokenType::Union)) {
        TRY(errors.append_or_short({
            "expected 'union'",
            nullptr,
            union_token,
        }));
        return Expression::garbage(start, union_token_index);
    }

    auto block_start_index = union_token_index + 1;
    auto block_start = tokens[block_start_index];
    if (block_start.is_not(TokenType::OpenCurly)) {
        TRY(errors.append_or_short({
            "expected '{'",
            nullptr,
            block_start,
        }));
        return Expression::garbage(start, block_start_index);
    }

    auto members_id
        = TRY(expressions.append(TRY(Members::create(8))));
    auto& members = expressions[members_id];

    auto block_end_index = block_start_index + 1;
    while (block_end_index < tokens.size()) {
        auto block_end = tokens[block_end_index];
        if (block_end.is(TokenType::CloseCurly))
            break;

        auto member_name_index = block_end_index;
        auto member_name = tokens[member_name_index];
        if (member_name.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected name of member",
                nullptr,
                member_name,
            }));
            return Expression::garbage(start, member_name_index);
        }

        auto colon_index = member_name_index + 1;
        auto colon = tokens[colon_index];
        if (colon.is_not(TokenType::Colon)) {
            TRY(errors.append_or_short({
                "expected ':'",
                nullptr,
                colon,
            }));
            return Expression::garbage(start, colon_index);
        }

        auto type_index = colon_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected type name",
                nullptr,
                type_token,
            }));
            return Expression::garbage(start, type_index);
        }

        auto comma_index = type_index + 1;
        auto comma = tokens[comma_index];
        if (comma.is_not(TokenType::Comma)) {
            TRY(errors.append_or_short({
                "expected ','",
                "did you forget a comma?",
                comma,
            }));
            return Expression::garbage(start, comma_index);
        }

        auto member = Member {
            .name = member_name,
            .type = type_token,
        };
        TRY(members.append(member));
        block_end_index = comma_index + 1;
    }

    auto block_end = tokens[block_end_index];
    if (block_end.is_not(TokenType::CloseCurly)) {
        TRY(errors.append_or_short({
            "expected '}'",
            nullptr,
            block_end,
        }));
        return Expression::garbage(start, block_end_index);
    }

    auto semicolon_index = block_end_index + 1;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        TRY(errors.append_or_short({
            "expected ';'",
            "did you forget a semicolon?",
            semicolon,
        }));
        return Expression::garbage(start, semicolon_index);
    }

    auto union_declaration = UnionDeclaration {
        .name = name,
        .members = members_id,
    };
    // NOTE: Swallow semicolon.
    auto end = semicolon_index + 1;
    auto union_id = TRY(expressions.append(union_declaration));
    return Expression(union_id, start, end);
}

ParseSingleItemResult parse_variant_declaration(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto name = tokens[start];

    auto assign_index = start + 1;
    auto assign = tokens[assign_index];
    if (assign.is_not(TokenType::Assign)) {
        auto const* hint = "struct declarations can't have colon "
                           "in this position";
        if (assign.is_not(TokenType::Colon))
            hint = nullptr;
        TRY(errors.append_or_short({
            "expected '='",
            hint,
            assign,
        }));
        return Expression::garbage(start, assign_index);
    }

    auto variant_token_index = assign_index + 1;
    auto variant_token = tokens[variant_token_index];
    if (variant_token.is_not(TokenType::Variant)) {
        TRY(errors.append_or_short({
            "expected 'variant'",
            nullptr,
            variant_token,
        }));
        return Expression::garbage(start, variant_token_index);
    }

    auto block_start_index = variant_token_index + 1;
    auto block_start = tokens[block_start_index];
    if (block_start.is_not(TokenType::OpenCurly)) {
        TRY(errors.append_or_short({
            "expected '{'",
            nullptr,
            block_start,
        }));
        return Expression::garbage(start, block_start_index);
    }

    auto members_id
        = TRY(expressions.append(TRY(Members::create(8))));
    auto& members = expressions[members_id];

    auto block_end_index = block_start_index + 1;
    while (block_end_index < tokens.size()) {
        auto block_end = tokens[block_end_index];
        if (block_end.is(TokenType::CloseCurly))
            break;

        auto member_name_index = block_end_index;
        auto member_name = tokens[member_name_index];
        if (member_name.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected name of member",
                nullptr,
                member_name,
            }));
            return Expression::garbage(start, member_name_index);
        }

        auto colon_index = member_name_index + 1;
        auto colon = tokens[colon_index];
        if (colon.is_not(TokenType::Colon)) {
            TRY(errors.append_or_short({
                "expected ':'",
                nullptr,
                colon,
            }));
            return Expression::garbage(start, colon_index);
        }

        auto type_index = colon_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected type name",
                nullptr,
                type_token,
            }));
            return Expression::garbage(start, type_index);
        }

        auto comma_index = type_index + 1;
        auto comma = tokens[comma_index];
        if (comma.is_not(TokenType::Comma)) {
            TRY(errors.append_or_short({
                "expected ','",
                "did you forget a comma?",
                comma,
            }));
            return Expression::garbage(start, comma_index);
        }

        auto member = Member {
            .name = member_name,
            .type = type_token,
        };
        TRY(members.append(member));
        block_end_index = comma_index + 1;
    }

    auto block_end = tokens[block_end_index];
    if (block_end.is_not(TokenType::CloseCurly)) {
        TRY(errors.append_or_short({
            "expected '}'",
            nullptr,
            block_end,
        }));
        return Expression::garbage(start, block_end_index);
    }

    auto semicolon_index = block_end_index + 1;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        TRY(errors.append_or_short({
            "expected ';'",
            "did you forget a semicolon?",
            semicolon,
        }));
        return Expression::garbage(start, semicolon_index);
    }

    auto variant_declaration = VariantDeclaration {
        .name = name,
        .members = members_id,
    };
    // NOTE: Swallow semicolon.
    auto end = semicolon_index + 1;
    auto variant_id = TRY(expressions.append(variant_declaration));
    return Expression(variant_id, start, end);
}

ParseSingleItemResult parse_private_variable_declaration(
    ParseErrors& errors, ParsedExpressions& expressions,
    Tokens const& tokens, u32 start)
{
    auto type = tokens[start];
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.is_not(TokenType::Identifier)) {
        TRY(errors.append_or_short({
            "expected variable name",
            "did you forget to name your variable?",
            name,
        }));
        return Expression::garbage(start, name_index);
    }

    auto colon_or_assign_index = name_index + 1;
    auto rvalue_start_index = colon_or_assign_index + 1;
    auto colon_or_assign = tokens[colon_or_assign_index];
    if (colon_or_assign.is(TokenType::Colon)) {
        auto type_index = colon_or_assign_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected type name",
                nullptr,
                type_token,
            }));
            return Expression::garbage(start, type_index);
        }
        type = type_token;

        auto assign_index = type_index + 1;
        auto assign = tokens[assign_index];
        if (assign.is_not(TokenType::Assign)) {
            TRY(errors.append_or_short({
                "expected '='",
                nullptr,
                assign,
            }));
            return Expression::garbage(start, assign_index);
        }
        rvalue_start_index = assign_index + 1;
    } else if (colon_or_assign.is_not(TokenType::Assign)) {
        TRY(errors.append_or_short({
            "expected ':', or '='",
            nullptr,
            colon_or_assign,
        }));
        return Expression::garbage(start, colon_or_assign_index);
    }

    auto struct_name_index = rvalue_start_index;
    auto struct_name = tokens[struct_name_index];
    if (struct_name.is(TokenType::Identifier)) {
        auto open_curly_index = struct_name_index + 1;
        auto open_curly = tokens[open_curly_index];
        if (open_curly.is(TokenType::OpenCurly)) {
            auto value = TRY(parse_struct_initializer(errors,
                expressions, tokens, struct_name_index));

            auto semicolon_index = value.end_token_index();
            auto semicolon = tokens[semicolon_index];
            if (semicolon.is_not(TokenType::Semicolon)) {
                TRY(errors.append_or_short({
                    "expected ';' after struct initializer",
                    "did you forget a semicolon?",
                    semicolon,
                }));
                return Expression::garbage(start, semicolon_index);
            }
            // NOTE: Swallow semicolon;
            auto end = semicolon_index + 1;

            auto value_id = TRY(expressions.append(value));
            auto variable = TRY(
                expressions.append(PrivateVariableDeclaration {
                    .name = name,
                    .type = type,
                    .value = value_id,
                }));

            return Expression(variable, start, end);
        }
    }

    auto value = TRY(parse_rvalue(errors, expressions, tokens,
        rvalue_start_index));
    auto rvalue_end_index = value.end_token_index();
    auto value_id = TRY(expressions.append(value));

    auto semicolon_index = rvalue_end_index;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        auto rvalue = tokens[rvalue_end_index];
        TRY(errors.append_or_short({
            "expected ';' after rvalue",
            "did you forget a semicolon?",
            rvalue,
        }));
        return Expression::garbage(start, semicolon_index);
    }
    // NOTE: Swallow semicolon;
    auto end = semicolon_index + 1;

    auto variable
        = TRY(expressions.append(PrivateVariableDeclaration {
            .name = name,
            .type = type,
            .value = value_id,
        }));
    return Expression(variable, start, end);
}

ParseSingleItemResult parse_variable_assignment(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto name_index = start;
    auto name = tokens[name_index];

    auto assign_index = start + 1;
    auto assign = tokens[assign_index];
    if (assign.is_not(TokenType::Assign)) {
        TRY(errors.append_or_short({
            "expected '='",
            nullptr,
            assign,
        }));
        return Expression::garbage(start, assign_index);
    }

    auto rvalue_index = assign_index + 1;
    auto rvalue = TRY(
        parse_rvalue(errors, expressions, tokens, rvalue_index));

    auto semicolon_index = rvalue.end_token_index();
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        TRY(errors.append_or_short({
            "expected ';'",
            "did you forget a semicolon?",
            semicolon,
        }));
        return Expression::garbage(start, semicolon_index);
    }
    auto end = semicolon_index;

    auto variable_assignment
        = TRY(expressions.append(VariableAssignment {
            .name = name,
            .value = rvalue.as_rvalue(),
        }));

    return Expression {
        variable_assignment,
        start,
        end + 1,
    };
}

ParseSingleItemResult parse_member_access(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto member_access = TRY(expressions.create_member_access());
    auto& access_expressions = expressions[member_access.members];

    auto end = start;
    while (end < tokens.size()) {
        auto name_index = end;
        auto name = tokens[name_index];
        if (name.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected member name",
                nullptr,
                name,
            }));
            return Expression::garbage(start, name_index);
        }

        TRY(access_expressions.append(name));

        auto dot_index = name_index + 1;
        auto dot = tokens[dot_index];
        if (dot.is_not(TokenType::Dot))
            break;

        end = dot_index + 1;
    }

    auto member_access_id = TRY(expressions.append(member_access));
    return Expression {
        member_access_id,
        start,
        end + 1,
    };
}

ParseSingleItemResult parse_array_access(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto name_index = start;
    auto name = tokens[name_index];
    if (name.is_not(TokenType::Identifier)) {
        TRY(errors.append_or_short({
            "expected array name",
            nullptr,
            name,
        }));
        return Expression::garbage(start, name_index);
    }

    auto open_bracket_index = name_index + 1;
    auto open_bracket = tokens[open_bracket_index];
    if (open_bracket.is_not(TokenType::OpenBracket)) {
        TRY(errors.append_or_short({
            "expected '['",
            nullptr,
            open_bracket,
        }));
        return Expression::garbage(start, open_bracket_index);
    }

    auto index_start = open_bracket_index + 1;
    auto index = TRY(parse_array_access_rvalue(errors, expressions,
        tokens, index_start));

    auto close_bracket_index = index.end_token_index();
    auto close_bracket = tokens[close_bracket_index];
    if (close_bracket.is_not(TokenType::CloseBracket)) {
        TRY(errors.append_or_short({
            "expected ']'",
            nullptr,
            close_bracket,
        }));
        return Expression::garbage(start, close_bracket_index);
    }

    auto array_access = TRY(expressions.append(ArrayAccess {
        .name = name,
        .index = index.as_rvalue(),
    }));

    auto end = close_bracket_index;
    return Expression {
        array_access,
        start,
        end + 1,
    };
}

ParseSingleItemResult parse_public_variable_declaration(
    ParseErrors& errors, ParsedExpressions& expressions,
    Tokens const& tokens, u32 start)
{
    auto type = tokens[start];
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.is_not(TokenType::Identifier)) {
        TRY(errors.append_or_short({
            "expected variable name",
            "did you forget to name your variable?",
            name,
        }));
        return Expression::garbage(start, name_index);
    }

    auto colon_or_assign_index = name_index + 1;
    auto rvalue_start_index = colon_or_assign_index + 1;
    auto colon_or_assign = tokens[colon_or_assign_index];
    if (colon_or_assign.is(TokenType::Colon)) {
        auto type_index = colon_or_assign_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected type name",
                nullptr,
                type_token,
            }));
            return Expression::garbage(start, type_index);
        }
        type = type_token;

        auto assign_index = type_index + 1;
        auto assign = tokens[assign_index];
        if (assign.is_not(TokenType::Assign)) {
            TRY(errors.append_or_short({
                "expected '='",
                nullptr,
                assign,
            }));
            return Expression::garbage(start, assign_index);
        }
        rvalue_start_index = assign_index + 1;
    } else if (colon_or_assign.is_not(TokenType::Assign)) {
        TRY(errors.append_or_short({
            "expected ':', or '='",
            nullptr,
            colon_or_assign,
        }));
        return Expression::garbage(start, colon_or_assign_index);
    }

    auto struct_name_index = rvalue_start_index;
    auto struct_name = tokens[struct_name_index];
    if (struct_name.is(TokenType::Identifier)) {
        auto open_curly_index = struct_name_index + 1;
        auto open_curly = tokens[open_curly_index];
        if (open_curly.is(TokenType::OpenCurly)) {
            auto value = TRY(parse_struct_initializer(errors,
                expressions, tokens, struct_name_index));

            auto semicolon_index = value.end_token_index();
            auto semicolon = tokens[semicolon_index];
            if (semicolon.is_not(TokenType::Semicolon)) {
                TRY(errors.append_or_short({
                    "expected ';' after struct initializer",
                    "did you forget a semicolon?",
                    semicolon,
                }));
                return Expression::garbage(start, semicolon_index);
            }
            // NOTE: Swallow semicolon;
            auto end = semicolon_index + 1;

            auto value_id = TRY(expressions.append(value));
            auto variable_id
                = TRY(expressions.append(PublicVariableDeclaration {
                    .name = name,
                    .type = type,
                    .value = value_id,
                }));

            return Expression(variable_id, start, end);
        }
    }

    auto value = TRY(parse_rvalue(errors, expressions, tokens,
        rvalue_start_index));
    auto rvalue_end_index = value.end_token_index();
    auto value_id = TRY(expressions.append(value));

    auto semicolon_index = rvalue_end_index;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        auto rvalue = tokens[rvalue_end_index];
        TRY(errors.append_or_short({
            "expected ';' after rvalue",
            "did you forget a semicolon?",
            rvalue,
        }));
        return Expression::garbage(start, semicolon_index);
    }
    // NOTE: Swallow semicolon;
    auto end = semicolon_index + 1;

    auto variable
        = TRY(expressions.append(PublicVariableDeclaration {
            .name = name,
            .type = type,
            .value = value_id,
        }));
    return Expression(variable, start, end);
}

[[maybe_unused]] ParseSingleItemResult
parse_private_constant_declaration(ParseErrors& errors,
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto type = tokens[start];
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.is_not(TokenType::Identifier)) {
        TRY(errors.append_or_short({
            "expected variable name",
            "did you forget to name your variable?",
            name,
        }));
        return Expression::garbage(start, name_index);
    }

    auto colon_or_assign_index = name_index + 1;
    auto rvalue_start_index = colon_or_assign_index + 1;
    auto colon_or_assign = tokens[colon_or_assign_index];
    if (colon_or_assign.is(TokenType::Colon)) {
        auto type_index = colon_or_assign_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected type name",
                nullptr,
                type_token,
            }));
            return Expression::garbage(start, type_index);
        }
        type = type_token;

        auto assign_index = type_index + 1;
        auto assign = tokens[assign_index];
        if (assign.is_not(TokenType::Assign)) {
            TRY(errors.append_or_short({
                "expected '='",
                nullptr,
                assign,
            }));
            return Expression::garbage(start, assign_index);
        }
        rvalue_start_index = assign_index + 1;
    } else if (colon_or_assign.is_not(TokenType::Assign)) {
        TRY(errors.append_or_short({
            "expected ':', or '='",
            nullptr,
            colon_or_assign,
        }));
        return Expression::garbage(start, colon_or_assign_index);
    }

    auto struct_name_index = rvalue_start_index;
    auto struct_name = tokens[struct_name_index];
    if (struct_name.is(TokenType::Identifier)) {
        auto open_curly_index = struct_name_index + 1;
        auto open_curly = tokens[open_curly_index];
        if (open_curly.is(TokenType::OpenCurly)) {
            auto value = TRY(parse_struct_initializer(errors,
                expressions, tokens, struct_name_index));

            auto semicolon_index = value.end_token_index();
            auto semicolon = tokens[semicolon_index];
            if (semicolon.is_not(TokenType::Semicolon)) {
                TRY(errors.append_or_short({
                    "expected ';' after struct initializer",
                    "did you forget a semicolon?",
                    semicolon,
                }));
                return Expression::garbage(start, semicolon_index);
            }
            // NOTE: Swallow semicolon;
            auto end = semicolon_index + 1;

            auto value_id = TRY(expressions.append(value));
            auto constant = TRY(
                expressions.append(PrivateConstantDeclaration {
                    .name = name,
                    .type = type,
                    .value = value_id,
                }));

            return Expression(constant, start, end);
        }
    }

    auto value = TRY(parse_rvalue(errors, expressions, tokens,
        rvalue_start_index));
    auto rvalue_end_index = value.end_token_index();
    auto value_id = TRY(expressions.append(value));

    auto semicolon_index = rvalue_end_index;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        auto rvalue = tokens[rvalue_end_index];
        TRY(errors.append_or_short({
            "expected ';' after rvalue",
            "did you forget a semicolon?",
            rvalue,
        }));
        return Expression::garbage(start, semicolon_index);
    }
    // NOTE: Swallow semicolon;
    auto end = semicolon_index + 1;

    auto constant
        = TRY(expressions.append(PrivateConstantDeclaration {
            .name = name,
            .type = type,
            .value = value_id,
        }));
    return Expression(constant, start, end);
}

ParseSingleItemResult parse_public_constant_declaration(
    ParseErrors& errors, ParsedExpressions& expressions,
    Tokens const& tokens, u32 start)
{
    auto type = tokens[start];
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.is_not(TokenType::Identifier)) {
        TRY(errors.append_or_short({
            "expected variable name",
            "did you forget to name your variable?",
            name,
        }));
        return Expression::garbage(start, name_index);
    }

    auto colon_or_assign_index = name_index + 1;
    auto rvalue_start_index = colon_or_assign_index + 1;
    auto colon_or_assign = tokens[colon_or_assign_index];
    if (colon_or_assign.is(TokenType::Colon)) {
        auto type_index = colon_or_assign_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected type name",
                nullptr,
                type_token,
            }));
            return Expression::garbage(start, type_index);
        }
        type = type_token;

        auto assign_index = type_index + 1;
        auto assign = tokens[assign_index];
        if (assign.is_not(TokenType::Assign)) {
            TRY(errors.append_or_short({
                "expected '='",
                nullptr,
                assign,
            }));
            return Expression::garbage(start, assign_index);
        }
        rvalue_start_index = assign_index + 1;
    } else if (colon_or_assign.is_not(TokenType::Assign)) {
        TRY(errors.append_or_short({
            "expected ':', or '='",
            nullptr,
            colon_or_assign,
        }));
        return Expression::garbage(start, colon_or_assign_index);
    }

    auto struct_name_index = rvalue_start_index;
    auto struct_name = tokens[struct_name_index];
    if (struct_name.is(TokenType::Identifier)) {
        auto open_curly_index = struct_name_index + 1;
        auto open_curly = tokens[open_curly_index];
        if (open_curly.is(TokenType::OpenCurly)) {
            auto value = TRY(parse_struct_initializer(errors,
                expressions, tokens, struct_name_index));

            auto semicolon_index = value.end_token_index();
            auto semicolon = tokens[semicolon_index];
            if (semicolon.is_not(TokenType::Semicolon)) {
                TRY(errors.append_or_short({
                    "expected ';' after struct initializer",
                    "did you forget a semicolon?",
                    semicolon,
                }));
                return Expression::garbage(start, semicolon_index);
            }
            // NOTE: Swallow semicolon;
            auto end = semicolon_index + 1;

            auto value_id = TRY(expressions.append(value));
            auto constant
                = TRY(expressions.append(PublicConstantDeclaration {
                    .name = name,
                    .type = type,
                    .value = value_id,
                }));

            return Expression(constant, start, end);
        }
    }

    auto value = TRY(parse_rvalue(errors, expressions, tokens,
        rvalue_start_index));
    auto rvalue_end_index = value.end_token_index();

    auto semicolon_index = rvalue_end_index;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        auto rvalue = tokens[rvalue_end_index];
        TRY(errors.append_or_short({
            "expected ';' after rvalue",
            "did you forget a semicolon?",
            rvalue,
        }));
        return Expression::garbage(start, semicolon_index);
    }
    // NOTE: Swallow semicolon;
    auto end = semicolon_index + 1;

    auto value_id = TRY(expressions.append(value));
    auto constant
        = TRY(expressions.append(PublicConstantDeclaration {
            .name = name,
            .type = type,
            .value = value_id,
        }));
    return Expression(constant, start, end);
}

ParseSingleItemResult parse_top_level_constant_or_struct(
    ParseErrors& errors, ParsedExpressions& expressions,
    Tokens const& tokens, u32 start)
{
    auto type = tokens[start];
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.is_not(TokenType::Identifier)) {
        TRY(errors.append_or_short({
            "expected variable name",
            "did you forget to name your variable or struct?",
            name,
        }));
        return Expression::garbage(start, name_index);
    }

    auto colon_or_assign_index = name_index + 1;
    auto rvalue_start_index = colon_or_assign_index + 1;
    auto colon_or_assign = tokens[colon_or_assign_index];
    if (colon_or_assign.is(TokenType::Colon)) {
        auto type_index = colon_or_assign_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            TRY(errors.append_or_short({
                "expected type name",
                nullptr,
                type_token,
            }));
            return Expression::garbage(start, type_index);
        }
        type = type_token;

        auto assign_index = type_index + 1;
        auto assign = tokens[assign_index];
        if (assign.is_not(TokenType::Assign)) {
            TRY(errors.append_or_short({
                "expected '='",
                nullptr,
                assign,
            }));
            return Expression::garbage(start, assign_index);
        }
        rvalue_start_index = assign_index + 1;
    } else if (colon_or_assign.is_not(TokenType::Assign)) {
        TRY(errors.append_or_short({
            "expected ':', or '='",
            nullptr,
            colon_or_assign,
        }));
        return Expression::garbage(start, colon_or_assign_index);
    }

    auto struct_name_index = rvalue_start_index;
    auto struct_name = tokens[struct_name_index];
    if (struct_name.is(TokenType::Identifier)) {
        auto open_curly_index = struct_name_index + 1;
        auto open_curly = tokens[open_curly_index];
        if (open_curly.is(TokenType::OpenCurly)) {
            auto value = TRY(parse_struct_initializer(errors,
                expressions, tokens, struct_name_index));

            auto semicolon_index = value.end_token_index();
            auto semicolon = tokens[semicolon_index];
            if (semicolon.is_not(TokenType::Semicolon)) {
                TRY(errors.append_or_short({
                    "expected ';' after struct initializer",
                    "did you forget a semicolon?",
                    semicolon,
                }));
                return Expression::garbage(start, semicolon_index);
            }
            // NOTE: Swallow semicolon;
            auto end = semicolon_index + 1;

            auto value_id = TRY(expressions.append(value));
            auto constant = TRY(
                expressions.append(PrivateConstantDeclaration {
                    .name = name,
                    .type = type,
                    .value = value_id,
                }));

            return Expression(constant, start, end);
        }
    }

    auto struct_token_index = colon_or_assign_index + 1;
    auto struct_token = tokens[struct_token_index];
    if (struct_token.is(TokenType::Struct))
        return TRY(parse_struct_declaration(errors, expressions,
            tokens, name_index));

    auto enum_token_index = struct_token_index;
    auto enum_token = tokens[enum_token_index];
    if (enum_token.is(TokenType::Enum))
        return TRY(parse_enum_declaration(errors, expressions,
            tokens, name_index));

    auto union_token_index = struct_token_index;
    auto union_token = tokens[union_token_index];
    if (union_token.is(TokenType::Union))
        return TRY(parse_union_declaration(errors, expressions,
            tokens, name_index));

    auto variant_token_index = union_token_index;
    auto variant_token = tokens[variant_token_index];
    if (variant_token.is(TokenType::Variant))
        return TRY(parse_variant_declaration(errors, expressions,
            tokens, name_index));

    auto value = TRY(parse_rvalue(errors, expressions, tokens,
        rvalue_start_index));
    auto rvalue_end_index = value.end_token_index();
    auto value_id = TRY(expressions.append(value));

    auto semicolon_index = rvalue_end_index;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        auto rvalue = tokens[rvalue_end_index];
        TRY(errors.append_or_short({
            "expected ';' after rvalue",
            "did you forget a semicolon?",
            rvalue,
        }));
        return Expression::garbage(start, semicolon_index);
    }
    // NOTE: Swallow semicolon;
    auto end = semicolon_index + 1;

    auto constant
        = TRY(expressions.append(PrivateConstantDeclaration {
            .name = name,
            .type = type,
            .value = value_id,
        }));
    return Expression(constant, start, end);
}

[[deprecated("can't parse invalid")]] //
[[maybe_unused]] ParseSingleItemResult
parse_invalid(ParseErrors& errors, ParsedExpressions&,
    Tokens const& tokens, u32 start)
{
    TRY(errors.append_or_short({
        "trying to parse invalid",
        nullptr,
        tokens[start],
    }));
    return Expression::garbage(start, start);
}

[[deprecated("can't parse moved value")]] //
[[maybe_unused]] ParseSingleItemResult
parse_moved_value(ParseErrors& errors, ParsedExpressions&,
    Tokens const& tokens, u32 start)
{
    TRY(errors.append_or_short({
        "trying to parse moved",
        nullptr,
        tokens[start],
    }));
    return Expression::garbage(start, start);
}

}

ErrorOr<void> ParseError::show(SourceFile source) const
{
    auto start_index = m_offending_token.start_index;
    auto end_index = m_offending_token.end_index(source.text);

    auto start = TRY(
        Util::line_and_column_for(source.text, start_index)
            .or_else([] {
                return Core::File::stderr()
                    .writeln(
                        "Could not fetch line and column for error"sv)
                    .on_success(Util::LineAndColumn {
                        .line = 0,
                        .column = 0,
                    });
            }));

    auto end = TRY(
        Util::line_and_column_for(source.text, end_index).or_else([] {
            return Core::File::stderr()
                .writeln(
                    "Could not fetch line and column for error"sv)
                .on_success(Util::LineAndColumn {
                    .line = 0,
                    .column = 0,
                });
        }));

    auto normal = "\033[0;0m"sv;
    auto red = "\033[1;31m"sv;
    auto yellow = "\033[1;33m"sv;
    auto cyan = "\033[1;36m"sv;
    auto blue = "\033[0;34m"sv;
    auto green = "\033[1;32m"sv;

    auto type = token_type_string(m_offending_token.type);

    auto& out = Core::File::stderr();
    TRY(out.writeln(green, "From:  "sv, normal, parser_function(),
        " ["sv, parser_file(), normal, ":"sv, line_in_parser_file(),
        "]"sv));
    TRY(out.writeln(red, "Error: "sv, normal, message(), normal,
        " ("sv, "got "sv, type, ") ["sv, blue, source.file_name,
        normal, ":"sv, start.line + 1, ":"sv, start.column + 1,
        "]"sv));
    if (m_hint)
        TRY(out.writeln(cyan, "Hint:  "sv, normal, hint()));

    if (start.line - end.line == 0) {
        if (start.line > 0 && start.column == 0) {
            auto line
                = Util::fetch_line(source.text, start.line - 1);
            TRY(out.writeln(line));
        }

        auto line = Util::fetch_line(source.text, start.line);
        TRY(out.writeln(line));

        for (u32 i = 0; i < start.column; i++)
            TRY(out.write(" "sv));
        TRY(out.write(yellow));
        for (u32 i = start.column; i < end.column; i++)
            TRY(out.write("^"sv));
        TRY(out.writeln(" "sv, message(), normal, " ("sv, "got "sv,
            type, ")"sv));

        if (m_hint) {
            for (u32 i = 0; i < start.column; i++)
                TRY(out.write(" "sv));
            TRY(out.write(cyan));
            for (u32 i = start.column; i < end.column; i++)
                TRY(out.write("^"sv));
            TRY(out.writeln(" hint: "sv, normal, hint()));
        }

        return {};
    }

    for (u32 i = start.line; i < end.line; i++) {
        auto line = Util::fetch_line(source.text, i);
        TRY(out.writeln(line));
    }
    for (u32 i = 0; i < end.column; i++)
        TRY(out.write(" "sv));
    TRY(out.write(yellow, "^"sv));
    TRY(out.writeln(" "sv, message(), normal, " ("sv, "got "sv,
        type, ")"sv));

    if (m_hint) {
        for (u32 i = 0; i < end.column; i++)
            TRY(out.write(" "sv));
        TRY(out.write(cyan, "^"sv));
        TRY(out.writeln(" hint: "sv, normal, hint()));
    }

    return {};
}

ErrorOr<void> ParseErrors::show(SourceFile source) const
{
    if (parse_errors.is_empty()) {
        TRY(Core::File::stderr().writeln(basic_error));
        return {};
    }

    for (auto const& error : parse_errors.in_reverse()) {
        TRY(error.show(source));
        TRY(Core::File::stderr().write("\n"sv));
    }
    return {};
}

}
