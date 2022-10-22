#include <Core/ErrorOr.h>
#include <Core/Try.h>
#include "Context.h"
#include "Expression.h"
#include "Parser.h"
#include "Token.h"
#include "Util.h"

namespace He {

namespace {

using ParseSingleItemResult = Core::ErrorOr<Expression, ParseError>;

#define FORWARD_DECLARE_PARSER(name)                          \
    ParseSingleItemResult parse_##name(                       \
        ParsedExpressions& expressions, Tokens const& tokens, \
        u32 start)

#define X(T, name, ...) FORWARD_DECLARE_PARSER(name);
EXPRESSIONS
#undef X

FORWARD_DECLARE_PARSER(root_item);
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
    auto expressions = TRY(ParsedExpressions::create());
    for (u32 start = 0; start < tokens.size();) {
        if (tokens[start].is(TokenType::NewLine))
            continue; // Ignore leading and trailing new lines.
        auto item
            = TRY(parse_root_item(expressions, tokens, start));
        start = item.end_token_index();
        TRY(expressions.expressions.append(item));
    }
    return expressions;
}

namespace {

[[maybe_unused]] ParseSingleItemResult parse_uninitialized(
    ParsedExpressions&, Tokens const& tokens, u32 start)
{
    auto uninitialized_index = start;
    auto uninitialized = tokens[uninitialized_index];
    if (uninitialized.is_not(TokenType::Uninitialized)) {
        return ParseError {
            "expected @uninitialized()",
            "this is probably a parser error",
            tokens[start],
        };
    }

    auto open_paren_index = uninitialized_index + 1;
    auto open_paren = tokens[open_paren_index];
    if (open_paren.is_not(TokenType::OpenParen)) {
        return ParseError {
            "expected '('",
            "function call need parenthesis",
            open_paren,
        };
    }

    auto close_paren_index = open_paren_index + 1;
    auto close_paren = tokens[close_paren_index];
    if (close_paren.is_not(TokenType::CloseParen)) {
        return ParseError {
            "expected ')'",
            "did you forget a parenthesis?",
            close_paren,
        };
    }

    auto end = close_paren_index;
    return Expression {
        ParsedExpressions::uninitialized_expression(),
        start,
        end + 1,
    };
}

ParseSingleItemResult parse_literal(ParsedExpressions& expressions,
    Tokens const& tokens, u32 start)
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

ParseSingleItemResult parse_lvalue(ParsedExpressions& expressions,
    Tokens const& tokens, u32 start)
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

ParseSingleItemResult parse_struct_initializer(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto type = tokens[start];

    auto open_curly_index = start + 1;
    auto open_curly = tokens[open_curly_index];
    if (open_curly.is_not(TokenType::OpenCurly)) {
        return ParseError {
            "expected '{'",
            nullptr,
            open_curly,
        };
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
            return ParseError {
                "expected '.'",
                "did you forget a dot before member name?",
                dot,
            };
        }

        auto name_index = dot_index + 1;
        auto name = tokens[name_index];
        if (name.is_not(TokenType::Identifier)) {
            return ParseError {
                "expected member name",
                nullptr,
                name,
            };
        }

        auto assign_index = name_index + 1;
        auto assign = tokens[assign_index];
        if (assign.is_not(TokenType::Assign)) {
            return ParseError {
                "expected '='",
                nullptr,
                assign,
            };
        }

        auto value_index = assign_index + 1;
        auto value
            = TRY(parse_irvalue(expressions, tokens, value_index));

        auto comma_index = value.end_token_index();
        auto comma = tokens[comma_index];
        if (comma.is_not(TokenType::Comma)) {
            return ParseError {
                "expected ','",
                nullptr,
                comma,
            };
        }
        end = comma_index + 1; // NOTE: Consume comma.

        TRY(initializers.append(Initializer {
            .name = name,
            .value = value.as_rvalue(),
        }));
    }

    if (tokens[end].is_not(TokenType::CloseCurly)) {
        return ParseError {
            "expected '}'",
            nullptr,
            tokens[end],
        };
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

ParseSingleItemResult parse_if_statement(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto condition
        = TRY(parse_if_rvalue(expressions, tokens, start + 1));
    auto block_start_index = condition.end_token_index();
    auto block_start = tokens[block_start_index];
    if (block_start.is_not(TokenType::OpenCurly)) {
        return ParseError {
            "expected '{'",
            "helium requires '{' after condition for if statements",
            block_start,
        };
    }
    auto block
        = TRY(parse_block(expressions, tokens, block_start_index));

    auto end = block.end_token_index();
    auto if_statement = TRY(expressions.append(If {
        condition.release_as_rvalue(),
        block.release_as_block(),
    }));
    return Expression(if_statement, start, end);
}

ParseSingleItemResult parse_while_statement(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto condition
        = TRY(parse_rvalue(expressions, tokens, start + 1));
    auto block_start_index = condition.end_token_index();
    auto block_start = tokens[block_start_index];
    if (block_start.is_not(TokenType::OpenCurly)) {
        return ParseError {
            "expected '{'",
            "helium requires '{' after condition for loops",
            block_start,
        };
    }
    auto block
        = TRY(parse_block(expressions, tokens, block_start_index));

    auto end = block.end_token_index();

    auto while_ = TRY(expressions.append(While {
        condition.release_as_rvalue(),
        block.release_as_block(),
    }));
    return Expression(while_, start, end);
}

ParseSingleItemResult parse_import_c(ParsedExpressions& expressions,
    Tokens const& tokens, u32 start)
{
    auto left_paren_index = start + 1;
    auto left_paren = tokens[left_paren_index];
    if (left_paren.is_not(TokenType::OpenParen)) {
        return ParseError {
            "expected '('",
            "did you forget a parenthesis?",
            left_paren,
        };
    }

    auto header_index = left_paren_index + 1;
    auto header = tokens[header_index];
    if (header.is_not(TokenType::Quoted)) {
        return ParseError {
            "expected quoted string",
            "system headers are also imported with '\"'"
            "quotes",
            header,
        };
    }

    auto right_paren_index = header_index + 1;
    auto right_paren = tokens[right_paren_index];
    if (right_paren.is_not(TokenType::CloseParen)) {
        return ParseError {
            "expected ')'",
            "did you forget a parenthesis?",
            left_paren,
        };
    }

    auto semicolon_index = right_paren_index + 1;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        return ParseError {
            "expected ';'",
            "did you forget a semicolon?",
            header,
        };
    }

    auto import_c = TRY(expressions.append(ImportC {
        header,
    }));

    // NOTE: Swallow semicolon.
    return Expression(import_c, start, semicolon_index + 1);
}

ParseSingleItemResult parse_pub_specifier(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto fn_index = start + 1;
    auto fn = tokens[fn_index];
    if (fn.is(TokenType::Fn))
        return parse_public_function(expressions, tokens, fn_index);
    if (fn.is(TokenType::CFn)) {
        return parse_public_c_function(expressions, tokens,
            fn_index);
    }
    if (fn.is(TokenType::Let)) {
        return parse_public_constant_declaration(expressions,
            tokens, fn_index);
    }
    if (fn.is(TokenType::Var)) {
        return parse_public_variable_declaration(expressions,
            tokens, fn_index);
    }
    return ParseError {
        "expected one of ['fn', 'c_fn', 'let', 'var']",
        nullptr,
        fn,
    };
}

ParseSingleItemResult parse_root_item(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto token = tokens[start];

    if (token.is(TokenType::ImportC))
        return parse_import_c(expressions, tokens, start);

    if (token.is(TokenType::InlineC))
        return parse_inline_c(expressions, tokens, start);

    if (token.is(TokenType::Fn))
        return parse_private_function(expressions, tokens, start);

    if (token.is(TokenType::CFn))
        return parse_private_c_function(expressions, tokens, start);

    if (token.is(TokenType::Pub))
        return parse_pub_specifier(expressions, tokens, start);

    if (token.is(TokenType::Let)) {
        return parse_top_level_constant_or_struct(expressions,
            tokens, start);
    }

    if (token.is(TokenType::Var)) {
        return parse_private_variable_declaration(expressions,
            tokens, start);
    }

    return ParseError {
        "unexpected token",
        nullptr,
        token,
    };
}

struct Function {
    Token name {};
    Token return_type {};
    Id<Parameters> parameters;
    Id<Block> block;
    u32 start_token_index { 0 };
    u32 end_token_index { 0 };
};

Core::ErrorOr<Function, ParseError> parse_function(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.is_not(TokenType::Identifier)) {
        return ParseError {
            "unexpected token",
            "expected function name",
            name,
        };
    }

    auto parameters_start = name_index + 1;
    auto starting_paren = tokens[parameters_start];
    if (starting_paren.is_not(TokenType::OpenParen)) {
        return ParseError {
            "unexpected token",
            "expected '('",
            starting_paren,
        };
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
            return ParseError {
                "expected parameter name",
                nullptr,
                name,
            };
        }

        auto colon_index = name_index + 1;
        auto colon = tokens[colon_index];
        if (colon.is_not(TokenType::Colon)) {
            return ParseError {
                "expected ':'",
                nullptr,
                colon,
            };
        }

        auto type_index = colon_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            return ParseError {
                "expected type name",
                nullptr,
                type_token,
            };
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

        return ParseError {
            "unexpected token",
            nullptr,
            comma_or_paren,
        };
    }
    // NOTE: Swallow paren.
    parameters_end++;

    auto arrow_index = parameters_end;
    auto arrow = tokens[arrow_index];
    if (arrow.is_not(TokenType::Arrow)) {
        return ParseError {
            "unexpected token",
            "expected '->'",
            arrow,
        };
    }

    auto return_type_index = arrow_index + 1;
    auto return_type = tokens[return_type_index];
    if (return_type.is_not(TokenType::Identifier)) {
        return ParseError {
            "unexpected token",
            "expected return type",
            return_type,
        };
    }

    auto block_start_index = return_type_index + 1;
    auto block_start = tokens[block_start_index];
    if (block_start.is_not(TokenType::OpenCurly)) {
        return ParseError {
            "unexpected token",
            "expected '{'",
            block_start,
        };
    }

    auto block
        = TRY(parse_block(expressions, tokens, block_start_index));
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

ParseSingleItemResult parse_public_function(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto function = TRY(parse_function(expressions, tokens, start));
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

ParseSingleItemResult parse_public_c_function(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto function = TRY(parse_function(expressions, tokens, start));
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

ParseSingleItemResult parse_private_function(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto function = TRY(parse_function(expressions, tokens, start));
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

ParseSingleItemResult parse_private_c_function(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto function = TRY(parse_function(expressions, tokens, start));
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

ParseSingleItemResult parse_function_call(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto function_name = tokens[start];

    auto left_paren_index = start + 1;
    auto left_paren = tokens[left_paren_index];
    if (left_paren.is_not(TokenType::OpenParen)) {
        return ParseError {
            "expected left parenthesis",
            "did you mean to do a function call?",
            left_paren,
        };
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
        auto argument = TRY(
            parse_prvalue(expressions, tokens, argument_index));
        right_paren_index = argument.end_token_index();
        TRY(expressions[call.arguments].append(argument));
    }

    auto right_paren = tokens[right_paren_index];
    if (right_paren.is_not(TokenType::CloseParen)) {
        return ParseError {
            "expected right parenthesis",
            "did you mean to do a function call?",
            right_paren,
        };
    }

    // NOTE: Swallow right parenthesis
    return Expression(call_id, start, right_paren_index + 1);
}

ParseSingleItemResult parse_return_statement(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.is(TokenType::Identifier)) {
        auto open_curly_index = name_index + 1;
        auto open_curly = tokens[open_curly_index];
        if (open_curly.is(TokenType::OpenCurly)) {
            auto value = TRY(parse_struct_initializer(expressions,
                tokens, name_index));
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

    auto value = TRY(parse_rvalue(expressions, tokens, start + 1));
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

ParseSingleItemResult parse_inline_c(ParsedExpressions& expressions,
    Tokens const& tokens, u32 start)
{
    i32 level = 0;
    auto block_start_index = start + 1;
    auto end = block_start_index;
    if (tokens[block_start_index].is(TokenType::OpenCurly)) {
        for (; end < tokens.size(); end++) {
            auto token = tokens[end];
            if (token.is(TokenType::OpenCurly))
                level++;
            if (token.is(TokenType::CloseCurly))
                level--;
            if (level == 0)
                break;
        }
        auto token = tokens[start + 1];
        auto last_token = tokens[end];
        token.start_index++;
        token.set_end_index(last_token.end_index() - 1);
        auto inline_c = TRY(expressions.append(InlineC {
            token,
        }));
        return Expression(inline_c, start, end + 1);
    }

    for (; end < tokens.size(); end++) {
        auto token = tokens[end];
        if (token.is(TokenType::OpenCurly))
            level++;
        if (token.is(TokenType::CloseCurly))
            level--;
        if (level == 0 && token.is(TokenType::Semicolon))
            break;
        if (level < 0) {
            return ParseError {
                "suspicious curly brace",
                "did you forget a semicolon after inline_c?",
                token,
            };
        }
    }
    auto token = tokens[start + 1];
    auto last_token = tokens[end];
    token.set_end_index(last_token.end_index());

    auto inline_c = TRY(expressions.append(InlineC {
        token,
    }));

    return Expression(inline_c, start, end + 1);
}

ParseSingleItemResult parse_block(ParsedExpressions& expressions,
    Tokens const& tokens, u32 start)
{
    auto block = TRY(expressions.create_block());
    auto end = start + 1;
    for (; end < tokens.size();) {
        if (tokens[end].is(TokenType::CloseCurly))
            break;

        if (tokens[end].is(TokenType::InlineC)) {
            auto inline_c
                = TRY(parse_inline_c(expressions, tokens, end));
            end = inline_c.end_token_index();
            TRY(expressions[block.expressions].append(inline_c));
            continue;
        }

        if (tokens[end].is(TokenType::OpenCurly)) {
            auto sub_block
                = TRY(parse_block(expressions, tokens, end));
            end = sub_block.end_token_index();
            TRY(expressions[block.expressions].append(sub_block));
            continue;
        }

        if (tokens[end].is(TokenType::Let)) {
            auto variable = TRY(parse_public_constant_declaration(
                expressions, tokens, end));
            end = variable.end_token_index();
            TRY(expressions[block.expressions].append(variable));
            continue;
        }

        if (tokens[end].is(TokenType::Var)) {
            auto variable = TRY(parse_public_variable_declaration(
                expressions, tokens, end));
            end = variable.end_token_index();
            TRY(expressions[block.expressions].append(variable));
            continue;
        }

        if (tokens[end].is(TokenType::Return)) {
            auto return_expression = TRY(
                parse_return_statement(expressions, tokens, end));
            end = return_expression.end_token_index();

            if (tokens[end].is_not(TokenType::Semicolon)) {
                return ParseError {
                    "expected ';'",
                    "did you forget a semicolon?",
                    tokens[end],
                };
            }
            end++; // NOTE: Swallow semicolon.

            TRY(expressions[block.expressions].append(
                return_expression));
            continue;
        }

        if (tokens[end].is(TokenType::Identifier)) {
            if (tokens[end + 1].is(TokenType::Assign)) {
                auto assignment = TRY(parse_variable_assignment(
                    expressions, tokens, end));
                TRY(expressions[block.expressions].append(
                    assignment));
                end = assignment.end_token_index();
                continue;
            }

            auto call = TRY(
                parse_function_call(expressions, tokens, end));
            end = call.end_token_index();
            TRY(expressions[block.expressions].append(call));

            if (tokens[end].is_not(TokenType::Semicolon)) {
                return ParseError {
                    "expected ';'",
                    "did you forget a semicolon?",
                    tokens[end],
                };
            }
            end++; // NOTE: Swallow semicolon.
            continue;
        }

        if (tokens[end].is(TokenType::If)) {
            auto if_
                = TRY(parse_if_statement(expressions, tokens, end));
            end = if_.end_token_index();
            TRY(expressions[block.expressions].append(if_));
            continue;
        }

        if (tokens[end].is(TokenType::While)) {
            auto while_ = TRY(
                parse_while_statement(expressions, tokens, end));
            end = while_.end_token_index();
            TRY(expressions[block.expressions].append(while_));
            continue;
        }

        return ParseError {
            "unexpected token",
            nullptr,
            tokens[end],
        };
    }
    auto block_id = TRY(expressions.append(block));
    // NOTE: Swallow close curly
    return Expression(block_id, start, end + 1);
}

ParseSingleItemResult parse_irvalue(ParsedExpressions& expressions,
    Tokens const& tokens, u32 start)
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
                    expressions, tokens, end));
                end = initializer.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    initializer));
                continue;
            }
        }

        if (tokens[end].is(TokenType::InlineC)) {
            auto inline_c
                = TRY(parse_inline_c(expressions, tokens, end));
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
            auto uninitialized = TRY(
                parse_uninitialized(expressions, tokens, end));
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
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
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
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Quoted)) {
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Identifier)) {
            if (tokens[end + 1].is(TokenType::OpenParen)) {
                auto call = TRY(
                    parse_function_call(expressions, tokens, end));
                end = call.end_token_index();
                TRY(expressions[rvalue.expressions].append(call));
                continue;
            }
            if (tokens[end + 1].is(TokenType::OpenCurly)) {
                auto initializer = TRY(parse_struct_initializer(
                    expressions, tokens, start));
                end = initializer.end_token_index() + 1;
                TRY(expressions[rvalue.expressions].append(
                    initializer));
                continue;
            }
            if (tokens[end + 1].is(TokenType::Dot)) {
                auto member_access = TRY(
                    parse_member_access(expressions, tokens, end));
                end = member_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    member_access));
                continue;
            }

            auto lvalue
                = TRY(parse_lvalue(expressions, tokens, end));
            end = lvalue.end_token_index();
            TRY(expressions[rvalue.expressions].append(lvalue));
            continue;
        }

        return ParseError {
            "expected ',' or '{'",
            "did you forget a comma?",
            tokens[end],
        };
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

    return ParseError {
        "expected ',' or '{'",
        "did you forget a comma?",
        tokens[end],
    };
}

ParseSingleItemResult parse_if_rvalue(
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
            auto inline_c
                = TRY(parse_inline_c(expressions, tokens, end));
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
            auto uninitialized = TRY(
                parse_uninitialized(expressions, tokens, end));
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
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
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
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Quoted)) {
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Identifier)) {
            if (tokens[end + 1].is(TokenType::OpenParen)) {
                auto call = TRY(
                    parse_function_call(expressions, tokens, end));
                end = call.end_token_index();
                TRY(expressions[rvalue.expressions].append(call));
                continue;
            }
            if (tokens[end + 1].is(TokenType::Dot)) {
                auto member_access = TRY(
                    parse_member_access(expressions, tokens, end));
                end = member_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    member_access));
                continue;
            }

            auto lvalue
                = TRY(parse_lvalue(expressions, tokens, end));
            end = lvalue.end_token_index();
            TRY(expressions[rvalue.expressions].append(lvalue));
            continue;
        }

        return ParseError {
            "expected ';' or '{'",
            "did you forget a semicolon?",
            tokens[end],
        };
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

    return ParseError {
        "expected ';' or '{'",
        "did you forget a semicolon?",
        tokens[end],
    };
}

ParseSingleItemResult parse_rvalue(ParsedExpressions& expressions,
    Tokens const& tokens, u32 start)
{
    auto rvalue = TRY(expressions.create_rvalue());

    auto end = start;
    for (; end < tokens.size();) {
        if (tokens[end].is(TokenType::Semicolon))
            break;
        if (tokens[end].is(TokenType::OpenCurly))
            break;

        if (tokens[end].is(TokenType::InlineC)) {
            auto inline_c
                = TRY(parse_inline_c(expressions, tokens, end));
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
            auto uninitialized = TRY(
                parse_uninitialized(expressions, tokens, end));
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
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
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
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Quoted)) {
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Identifier)) {
            if (tokens[end + 1].is(TokenType::OpenParen)) {
                auto call = TRY(
                    parse_function_call(expressions, tokens, end));
                end = call.end_token_index();
                TRY(expressions[rvalue.expressions].append(call));
                continue;
            }
            if (tokens[end + 1].is(TokenType::OpenCurly)) {
                auto initializer = TRY(parse_struct_initializer(
                    expressions, tokens, end));
                end = initializer.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    initializer));
                continue;
            }
            if (tokens[end + 1].is(TokenType::Dot)) {
                auto member_access = TRY(
                    parse_member_access(expressions, tokens, end));
                end = member_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    member_access));
                continue;
            }

            if (tokens[end + 1].is(TokenType::OpenBracket)) {
                auto array_access = TRY(
                    parse_array_access(expressions, tokens, end));
                end = array_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    array_access));
                continue;
            }

            auto lvalue
                = TRY(parse_lvalue(expressions, tokens, end));
            end = lvalue.end_token_index();
            TRY(expressions[rvalue.expressions].append(lvalue));
            continue;
        }

        return ParseError {
            "expected ';' or '{'",
            "did you forget a semicolon?",
            tokens[end],
        };
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

    return ParseError {
        "expected ';' or '{'",
        "did you forget a semicolon?",
        tokens[end],
    };
}

ParseSingleItemResult parse_array_access_rvalue(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto rvalue = TRY(expressions.create_rvalue());

    auto end = start;
    for (; end < tokens.size();) {
        if (tokens[end].is(TokenType::CloseBracket))
            break;

        if (tokens[end].is(TokenType::Number)) {
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
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
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Quoted)) {
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Identifier)) {
            if (tokens[end + 1].is(TokenType::OpenParen)) {
                auto call = TRY(
                    parse_function_call(expressions, tokens, end));
                end = call.end_token_index();
                TRY(expressions[rvalue.expressions].append(call));
                continue;
            }
            if (tokens[end + 1].is(TokenType::OpenCurly)) {
                auto initializer = TRY(parse_struct_initializer(
                    expressions, tokens, end));
                end = initializer.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    initializer));
                continue;
            }
            if (tokens[end + 1].is(TokenType::Dot)) {
                auto member_access = TRY(
                    parse_member_access(expressions, tokens, end));
                end = member_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    member_access));
                continue;
            }

            if (tokens[end + 1].is(TokenType::OpenBracket)) {
                auto array_access = TRY(
                    parse_array_access(expressions, tokens, start));
                end = array_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    array_access));
                continue;
            }

            auto lvalue
                = TRY(parse_lvalue(expressions, tokens, end));
            end = lvalue.end_token_index();
            TRY(expressions[rvalue.expressions].append(lvalue));
            continue;
        }

        return ParseError {
            "expected ']'",
            nullptr,
            tokens[end],
        };
    }

    if (tokens[end].is_not(TokenType::CloseBracket)) {
        return ParseError {
            "expected ']'",
            nullptr,
            tokens[end],
        };
    }

    auto rvalue_id = TRY(expressions.append(rvalue));
    return Expression {
        rvalue_id,
        start,
        end,
    };
}

ParseSingleItemResult parse_prvalue(ParsedExpressions& expressions,
    Tokens const& tokens, u32 start)
{
    auto rvalue = TRY(expressions.create_rvalue());

    auto end = start;
    for (; end < tokens.size();) {
        if (tokens[end].is(TokenType::Comma))
            break;
        if (tokens[end].is(TokenType::CloseParen))
            break;

        if (tokens[end].is(TokenType::Number)) {
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::RefMut)) {
            auto token = tokens[end];
            token.set_end_index(token.start_index + 1);
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

        if (tokens[end].is(TokenType::Ampersand)) {
            auto token = tokens[end];
            token.set_end_index(token.start_index + 1);
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
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Quoted)) {
            auto literal
                = TRY(parse_literal(expressions, tokens, end));
            TRY(expressions[rvalue.expressions].append(literal));
            end = literal.end_token_index();
            continue;
        }

        if (tokens[end].is(TokenType::Identifier)) {
            if (tokens[end + 1].is(TokenType::OpenParen)) {
                auto function_call = TRY(
                    parse_function_call(expressions, tokens, end));
                end = function_call.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    function_call));
                continue;
            }
            if (tokens[end + 1].is(TokenType::Dot)) {
                auto member_access = TRY(
                    parse_member_access(expressions, tokens, end));
                end = member_access.end_token_index();
                TRY(expressions[rvalue.expressions].append(
                    member_access));
                continue;
            }

            auto lvalue
                = TRY(parse_lvalue(expressions, tokens, end));
            end = lvalue.end_token_index();
            TRY(expressions[rvalue.expressions].append(lvalue));
            continue;
        }

        return ParseError {
            "expected ',' or ')'",
            "did you forget a comma?",
            tokens[end],
        };
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

    return ParseError {
        "expected ',' or ')'",
        "did you forget a comma?",
        tokens[end],
    };
}

ParseSingleItemResult parse_struct_declaration(
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
        return ParseError {
            "expected '='",
            hint,
            assign,
        };
    }

    auto struct_token_index = assign_index + 1;
    auto struct_token = tokens[struct_token_index];
    if (struct_token.is_not(TokenType::Struct)) {
        return ParseError {
            "expected 'struct'",
            nullptr,
            struct_token,
        };
    }

    auto block_start_index = struct_token_index + 1;
    auto block_start = tokens[block_start_index];
    if (block_start.is_not(TokenType::OpenCurly)) {
        return ParseError {
            "expected '{'",
            nullptr,
            block_start,
        };
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
            return ParseError {
                "expected name of member",
                nullptr,
                member_name,
            };
        }

        auto colon_index = member_name_index + 1;
        auto colon = tokens[colon_index];
        if (colon.is_not(TokenType::Colon)) {
            return ParseError {
                "expected ':'",
                nullptr,
                colon,
            };
        }

        auto type_index = colon_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            return ParseError {
                "expected type name",
                nullptr,
                type_token,
            };
        }

        auto comma_index = type_index + 1;
        auto comma = tokens[comma_index];
        if (comma.is_not(TokenType::Comma)) {
            return ParseError {
                "expected ','",
                "did you forget a comma?",
                comma,
            };
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
        return ParseError {
            "expected '}'",
            nullptr,
            block_end,
        };
    }

    auto semicolon_index = block_end_index + 1;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        return ParseError {
            "expected ';'",
            "did you forget a semicolon?",
            semicolon,
        };
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

ParseSingleItemResult parse_enum_declaration(
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
        return ParseError {
            "expected '='",
            hint,
            assign,
        };
    }

    auto enum_token_index = assign_index + 1;
    auto enum_token = tokens[enum_token_index];
    if (enum_token.is_not(TokenType::Enum)) {
        return ParseError {
            "expected 'enum'",
            nullptr,
            enum_token,
        };
    }

    auto block_start_index = enum_token_index + 1;
    auto block_start = tokens[block_start_index];
    if (block_start.is_not(TokenType::OpenCurly)) {
        return ParseError {
            "expected '{'",
            nullptr,
            block_start,
        };
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
            return ParseError {
                "expected name of member",
                nullptr,
                member_name,
            };
        }

        auto comma_index = member_name_index + 1;
        auto comma = tokens[comma_index];
        if (comma.is_not(TokenType::Comma)) {
            return ParseError {
                "expected ','",
                "did you forget a comma?",
                comma,
            };
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
        return ParseError {
            "expected '}'",
            nullptr,
            block_end,
        };
    }

    auto semicolon_index = block_end_index + 1;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        return ParseError {
            "expected ';'",
            "did you forget a semicolon?",
            semicolon,
        };
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

ParseSingleItemResult parse_union_declaration(
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
        return ParseError {
            "expected '='",
            hint,
            assign,
        };
    }

    auto union_token_index = assign_index + 1;
    auto union_token = tokens[union_token_index];
    if (union_token.is_not(TokenType::Union)) {
        return ParseError {
            "expected 'union'",
            nullptr,
            union_token,
        };
    }

    auto block_start_index = union_token_index + 1;
    auto block_start = tokens[block_start_index];
    if (block_start.is_not(TokenType::OpenCurly)) {
        return ParseError {
            "expected '{'",
            nullptr,
            block_start,
        };
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
            return ParseError {
                "expected name of member",
                nullptr,
                member_name,
            };
        }

        auto colon_index = member_name_index + 1;
        auto colon = tokens[colon_index];
        if (colon.is_not(TokenType::Colon)) {
            return ParseError {
                "expected ':'",
                nullptr,
                colon,
            };
        }

        auto type_index = colon_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            return ParseError {
                "expected type name",
                nullptr,
                type_token,
            };
        }

        auto comma_index = type_index + 1;
        auto comma = tokens[comma_index];
        if (comma.is_not(TokenType::Comma)) {
            return ParseError {
                "expected ','",
                "did you forget a comma?",
                comma,
            };
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
        return ParseError {
            "expected '}'",
            nullptr,
            block_end,
        };
    }

    auto semicolon_index = block_end_index + 1;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        return ParseError {
            "expected ';'",
            "did you forget a semicolon?",
            semicolon,
        };
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

ParseSingleItemResult parse_variant_declaration(
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
        return ParseError {
            "expected '='",
            hint,
            assign,
        };
    }

    auto variant_token_index = assign_index + 1;
    auto variant_token = tokens[variant_token_index];
    if (variant_token.is_not(TokenType::Variant)) {
        return ParseError {
            "expected 'variant'",
            nullptr,
            variant_token,
        };
    }

    auto block_start_index = variant_token_index + 1;
    auto block_start = tokens[block_start_index];
    if (block_start.is_not(TokenType::OpenCurly)) {
        return ParseError {
            "expected '{'",
            nullptr,
            block_start,
        };
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
            return ParseError {
                "expected name of member",
                nullptr,
                member_name,
            };
        }

        auto colon_index = member_name_index + 1;
        auto colon = tokens[colon_index];
        if (colon.is_not(TokenType::Colon)) {
            return ParseError {
                "expected ':'",
                nullptr,
                colon,
            };
        }

        auto type_index = colon_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            return ParseError {
                "expected type name",
                nullptr,
                type_token,
            };
        }

        auto comma_index = type_index + 1;
        auto comma = tokens[comma_index];
        if (comma.is_not(TokenType::Comma)) {
            return ParseError {
                "expected ','",
                "did you forget a comma?",
                comma,
            };
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
        return ParseError {
            "expected '}'",
            nullptr,
            block_end,
        };
    }

    auto semicolon_index = block_end_index + 1;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        return ParseError {
            "expected ';'",
            "did you forget a semicolon?",
            semicolon,
        };
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
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto type = tokens[start];
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.is_not(TokenType::Identifier)) {
        return ParseError {
            "expected variable name",
            "did you forget to name your variable?",
            name,
        };
    }

    auto colon_or_assign_index = name_index + 1;
    auto rvalue_start_index = colon_or_assign_index + 1;
    auto colon_or_assign = tokens[colon_or_assign_index];
    if (colon_or_assign.is(TokenType::Colon)) {
        auto type_index = colon_or_assign_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            return ParseError {
                "expected type name",
                nullptr,
                type_token,
            };
        }
        type = type_token;

        auto assign_index = type_index + 1;
        auto assign = tokens[assign_index];
        if (assign.is_not(TokenType::Assign)) {
            return ParseError {
                "expected '='",
                nullptr,
                assign,
            };
        }
        rvalue_start_index = assign_index + 1;
    } else if (colon_or_assign.is_not(TokenType::Assign)) {
        return ParseError {
            "expected ':', or '='",
            nullptr,
            colon_or_assign,
        };
    }

    auto struct_name_index = rvalue_start_index;
    auto struct_name = tokens[struct_name_index];
    if (struct_name.is(TokenType::Identifier)) {
        auto open_curly_index = struct_name_index + 1;
        auto open_curly = tokens[open_curly_index];
        if (open_curly.is(TokenType::OpenCurly)) {
            auto value = TRY(parse_struct_initializer(expressions,
                tokens, struct_name_index));

            auto semicolon_index = value.end_token_index();
            auto semicolon = tokens[semicolon_index];
            if (semicolon.is_not(TokenType::Semicolon)) {
                return ParseError {
                    "expected ';' after struct initializer",
                    "did you forget a semicolon?",
                    semicolon,
                };
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

    auto value = TRY(
        parse_rvalue(expressions, tokens, rvalue_start_index));
    auto rvalue_end_index = value.end_token_index();
    auto value_id = TRY(expressions.append(value));

    auto semicolon_index = rvalue_end_index;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        auto rvalue = tokens[rvalue_end_index];
        return ParseError {
            "expected ';' after rvalue",
            "did you forget a semicolon?",
            rvalue,
        };
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

ParseSingleItemResult parse_variable_assignment(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto name_index = start;
    auto name = tokens[name_index];

    auto assign_index = start + 1;
    auto assign = tokens[assign_index];
    if (assign.is_not(TokenType::Assign)) {
        return ParseError {
            "expected '='",
            nullptr,
            assign,
        };
    }

    auto rvalue_index = assign_index + 1;
    auto rvalue
        = TRY(parse_rvalue(expressions, tokens, rvalue_index));

    auto semicolon_index = rvalue.end_token_index();
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        return ParseError {
            "expected ';'",
            "did you forget a semicolon?",
            semicolon,
        };
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

ParseSingleItemResult parse_member_access(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto member_access = TRY(expressions.create_member_access());
    auto& access_expressions = expressions[member_access.members];

    auto end = start;
    while (end < tokens.size()) {
        auto name_index = end;
        auto name = tokens[name_index];
        if (name.is_not(TokenType::Identifier)) {
            return ParseError {
                "expected member name",
                nullptr,
                name,
            };
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

ParseSingleItemResult parse_array_access(
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto name_index = start;
    auto name = tokens[name_index];
    if (name.is_not(TokenType::Identifier)) {
        return ParseError {
            "expected array name",
            nullptr,
            name,
        };
    }

    auto open_bracket_index = name_index + 1;
    auto open_bracket = tokens[open_bracket_index];
    if (open_bracket.is_not(TokenType::OpenBracket)) {
        return ParseError {
            "expected '['",
            nullptr,
            open_bracket,
        };
    }

    auto index_start = open_bracket_index + 1;
    auto index = TRY(parse_array_access_rvalue(expressions, tokens,
        index_start));

    auto close_bracket_index = index.end_token_index();
    auto close_bracket = tokens[close_bracket_index];
    if (close_bracket.is_not(TokenType::CloseBracket)) {
        return ParseError {
            "expected ']'",
            nullptr,
            close_bracket,
        };
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
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto type = tokens[start];
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.is_not(TokenType::Identifier)) {
        return ParseError {
            "expected variable name",
            "did you forget to name your variable?",
            name,
        };
    }

    auto colon_or_assign_index = name_index + 1;
    auto rvalue_start_index = colon_or_assign_index + 1;
    auto colon_or_assign = tokens[colon_or_assign_index];
    if (colon_or_assign.is(TokenType::Colon)) {
        auto type_index = colon_or_assign_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            return ParseError {
                "expected type name",
                nullptr,
                type_token,
            };
        }
        type = type_token;

        auto assign_index = type_index + 1;
        auto assign = tokens[assign_index];
        if (assign.is_not(TokenType::Assign)) {
            return ParseError {
                "expected '='",
                nullptr,
                assign,
            };
        }
        rvalue_start_index = assign_index + 1;
    } else if (colon_or_assign.is_not(TokenType::Assign)) {
        return ParseError {
            "expected ':', or '='",
            nullptr,
            colon_or_assign,
        };
    }

    auto struct_name_index = rvalue_start_index;
    auto struct_name = tokens[struct_name_index];
    if (struct_name.is(TokenType::Identifier)) {
        auto open_curly_index = struct_name_index + 1;
        auto open_curly = tokens[open_curly_index];
        if (open_curly.is(TokenType::OpenCurly)) {
            auto value = TRY(parse_struct_initializer(expressions,
                tokens, struct_name_index));

            auto semicolon_index = value.end_token_index();
            auto semicolon = tokens[semicolon_index];
            if (semicolon.is_not(TokenType::Semicolon)) {
                return ParseError {
                    "expected ';' after struct initializer",
                    "did you forget a semicolon?",
                    semicolon,
                };
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

    auto value = TRY(
        parse_rvalue(expressions, tokens, rvalue_start_index));
    auto rvalue_end_index = value.end_token_index();
    auto value_id = TRY(expressions.append(value));

    auto semicolon_index = rvalue_end_index;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        auto rvalue = tokens[rvalue_end_index];
        return ParseError {
            "expected ';' after rvalue",
            "did you forget a semicolon?",
            rvalue,
        };
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
parse_private_constant_declaration(ParsedExpressions& expressions,
    Tokens const& tokens, u32 start)
{
    auto type = tokens[start];
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.is_not(TokenType::Identifier)) {
        return ParseError {
            "expected variable name",
            "did you forget to name your variable?",
            name,
        };
    }

    auto colon_or_assign_index = name_index + 1;
    auto rvalue_start_index = colon_or_assign_index + 1;
    auto colon_or_assign = tokens[colon_or_assign_index];
    if (colon_or_assign.is(TokenType::Colon)) {
        auto type_index = colon_or_assign_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            return ParseError {
                "expected type name",
                nullptr,
                type_token,
            };
        }
        type = type_token;

        auto assign_index = type_index + 1;
        auto assign = tokens[assign_index];
        if (assign.is_not(TokenType::Assign)) {
            return ParseError {
                "expected '='",
                nullptr,
                assign,
            };
        }
        rvalue_start_index = assign_index + 1;
    } else if (colon_or_assign.is_not(TokenType::Assign)) {
        return ParseError {
            "expected ':', or '='",
            nullptr,
            colon_or_assign,
        };
    }

    auto struct_name_index = rvalue_start_index;
    auto struct_name = tokens[struct_name_index];
    if (struct_name.is(TokenType::Identifier)) {
        auto open_curly_index = struct_name_index + 1;
        auto open_curly = tokens[open_curly_index];
        if (open_curly.is(TokenType::OpenCurly)) {
            auto value = TRY(parse_struct_initializer(expressions,
                tokens, struct_name_index));

            auto semicolon_index = value.end_token_index();
            auto semicolon = tokens[semicolon_index];
            if (semicolon.is_not(TokenType::Semicolon)) {
                return ParseError {
                    "expected ';' after struct initializer",
                    "did you forget a semicolon?",
                    semicolon,
                };
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

    auto value = TRY(
        parse_rvalue(expressions, tokens, rvalue_start_index));
    auto rvalue_end_index = value.end_token_index();
    auto value_id = TRY(expressions.append(value));

    auto semicolon_index = rvalue_end_index;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        auto rvalue = tokens[rvalue_end_index];
        return ParseError {
            "expected ';' after rvalue",
            "did you forget a semicolon?",
            rvalue,
        };
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
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto type = tokens[start];
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.is_not(TokenType::Identifier)) {
        return ParseError {
            "expected variable name",
            "did you forget to name your variable?",
            name,
        };
    }

    auto colon_or_assign_index = name_index + 1;
    auto rvalue_start_index = colon_or_assign_index + 1;
    auto colon_or_assign = tokens[colon_or_assign_index];
    if (colon_or_assign.is(TokenType::Colon)) {
        auto type_index = colon_or_assign_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            return ParseError {
                "expected type name",
                nullptr,
                type_token,
            };
        }
        type = type_token;

        auto assign_index = type_index + 1;
        auto assign = tokens[assign_index];
        if (assign.is_not(TokenType::Assign)) {
            return ParseError {
                "expected '='",
                nullptr,
                assign,
            };
        }
        rvalue_start_index = assign_index + 1;
    } else if (colon_or_assign.is_not(TokenType::Assign)) {
        return ParseError {
            "expected ':', or '='",
            nullptr,
            colon_or_assign,
        };
    }

    auto struct_name_index = rvalue_start_index;
    auto struct_name = tokens[struct_name_index];
    if (struct_name.is(TokenType::Identifier)) {
        auto open_curly_index = struct_name_index + 1;
        auto open_curly = tokens[open_curly_index];
        if (open_curly.is(TokenType::OpenCurly)) {
            auto value = TRY(parse_struct_initializer(expressions,
                tokens, struct_name_index));

            auto semicolon_index = value.end_token_index();
            auto semicolon = tokens[semicolon_index];
            if (semicolon.is_not(TokenType::Semicolon)) {
                return ParseError {
                    "expected ';' after struct initializer",
                    "did you forget a semicolon?",
                    semicolon,
                };
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

    auto value = TRY(
        parse_rvalue(expressions, tokens, rvalue_start_index));
    auto rvalue_end_index = value.end_token_index();

    auto semicolon_index = rvalue_end_index;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        auto rvalue = tokens[rvalue_end_index];
        return ParseError {
            "expected ';' after rvalue",
            "did you forget a semicolon?",
            rvalue,
        };
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
    ParsedExpressions& expressions, Tokens const& tokens, u32 start)
{
    auto type = tokens[start];
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.is_not(TokenType::Identifier)) {
        return ParseError {
            "expected variable name",
            "did you forget to name your variable or struct?",
            name,
        };
    }

    auto colon_or_assign_index = name_index + 1;
    auto rvalue_start_index = colon_or_assign_index + 1;
    auto colon_or_assign = tokens[colon_or_assign_index];
    if (colon_or_assign.is(TokenType::Colon)) {
        auto type_index = colon_or_assign_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.is_not(TokenType::Identifier)) {
            return ParseError {
                "expected type name",
                nullptr,
                type_token,
            };
        }
        type = type_token;

        auto assign_index = type_index + 1;
        auto assign = tokens[assign_index];
        if (assign.is_not(TokenType::Assign)) {
            return ParseError {
                "expected '='",
                nullptr,
                assign,
            };
        }
        rvalue_start_index = assign_index + 1;
    } else if (colon_or_assign.is_not(TokenType::Assign)) {
        return ParseError {
            "expected ':', or '='",
            nullptr,
            colon_or_assign,
        };
    }

    auto struct_name_index = rvalue_start_index;
    auto struct_name = tokens[struct_name_index];
    if (struct_name.is(TokenType::Identifier)) {
        auto open_curly_index = struct_name_index + 1;
        auto open_curly = tokens[open_curly_index];
        if (open_curly.is(TokenType::OpenCurly)) {
            auto value = TRY(parse_struct_initializer(expressions,
                tokens, struct_name_index));

            auto semicolon_index = value.end_token_index();
            auto semicolon = tokens[semicolon_index];
            if (semicolon.is_not(TokenType::Semicolon)) {
                return ParseError {
                    "expected ';' after struct initializer",
                    "did you forget a semicolon?",
                    semicolon,
                };
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
        return TRY(parse_struct_declaration(expressions, tokens,
            name_index));

    auto enum_token_index = struct_token_index;
    auto enum_token = tokens[enum_token_index];
    if (enum_token.is(TokenType::Enum))
        return TRY(parse_enum_declaration(expressions, tokens,
            name_index));

    auto union_token_index = struct_token_index;
    auto union_token = tokens[union_token_index];
    if (union_token.is(TokenType::Union))
        return TRY(parse_union_declaration(expressions, tokens,
            name_index));

    auto variant_token_index = union_token_index;
    auto variant_token = tokens[variant_token_index];
    if (variant_token.is(TokenType::Variant))
        return TRY(parse_variant_declaration(expressions, tokens,
            name_index));

    auto value = TRY(
        parse_rvalue(expressions, tokens, rvalue_start_index));
    auto rvalue_end_index = value.end_token_index();
    auto value_id = TRY(expressions.append(value));

    auto semicolon_index = rvalue_end_index;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.is_not(TokenType::Semicolon)) {
        auto rvalue = tokens[rvalue_end_index];
        return ParseError {
            "expected ';' after rvalue",
            "did you forget a semicolon?",
            rvalue,
        };
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
parse_invalid(ParsedExpressions&, Tokens const& tokens, u32 start)
{
    return ParseError {
        "trying to parse invalid",
        nullptr,
        tokens[start],
    };
}

[[deprecated("can't parse moved value")]] //
[[maybe_unused]] ParseSingleItemResult
parse_moved_value(ParsedExpressions&, Tokens const& tokens,
    u32 start)
{
    return ParseError {
        "trying to parse moved",
        nullptr,
        tokens[start],
    };
}

}

Core::ErrorOr<void> ParseError::show(SourceFile source) const
{
    auto start = *Util::line_and_column_for(source.text,
        offending_token.start_index);
    auto end = *Util::line_and_column_for(source.text,
        offending_token.end_index());
    auto line = Util::fetch_line(source.text, start.line);

    std::cerr << parser_function << " @ [" << parser_file << ":"
              << line_in_parser_file << "]:\n\n"
              << message << " "
              << "[" << source.file_name << ':' << start.line << ':'
              << start.column << "]\n";
    std::cerr << line << '\n';

    for (u32 i = 0; i < start.column; i++)
        std::cerr << ' ';
    for (u32 i = start.column; i < end.column; i++)
        std::cerr << '^';
    std::cerr << '\n';

    if (hint)
        std::cerr << "Hint: " << hint << '\n';

    return {};
}

}
