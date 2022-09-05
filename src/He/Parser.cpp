#include <Core/ErrorOr.h>
#include <Core/Try.h>
#include <He/Context.h>
#include <He/Parser.h>
#include <He/Token.h>
#include <Util.h>
#include <iostream>

namespace He {

std::string_view expression_type_string(ExpressionType type)
{
    switch (type) {
#define CASE_RETURN(variant) \
    case ExpressionType::variant: return #variant
        CASE_RETURN(Literal);
        CASE_RETURN(VariableDeclaration);

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

using ParseSingleItemResult = Core::ErrorOr<Expression, ParseError>;
static ParseSingleItemResult parse_root_item(Tokens const&,
    u32 start);

static ParseSingleItemResult parse_variable(Tokens const&,
    u32 start);

static ParseSingleItemResult parse_rvalue(Tokens const&, u32 start);

static ParseSingleItemResult parse_return(Tokens const&, u32 start);

static ParseSingleItemResult parse_public_function(Tokens const&,
    u32 start);

static ParseSingleItemResult parse_private_function(Tokens const&,
    u32 start);

static ParseSingleItemResult parse_import_c(Tokens const&,
    u32 start);

static ParseSingleItemResult parse_inline_c(Tokens const&,
    u32 start);

static ParseSingleItemResult parse_block(Tokens const&, u32 start);

static ParseSingleItemResult parse_function_call(Tokens const&,
    u32 start);

static ParseSingleItemResult parse_prvalue(Tokens const&,
    u32 start);

static ParseSingleItemResult parse_if(Tokens const&, u32 start);

static ParseSingleItemResult parse_while(Tokens const&, u32 start);

static ParseSingleItemResult parse_pub_specifier(
    Tokens const& tokens, u32 start);

ParseResult parse(Tokens const& tokens)
{
    Expressions items;
    for (u32 start = 0; start < tokens.size();) {
        if (tokens[start].type == TokenType::NewLine)
            continue; // Ignore leading and trailing new lines.
        auto item = TRY(parse_root_item(tokens, start));
        start = item.end_token_index;
        items.push_back(std::move(item));
    }
    return items;
}

static ParseSingleItemResult parse_if(Tokens const& tokens,
    u32 start)
{
    auto condition = TRY(parse_rvalue(tokens, start + 1));
    auto block_start_index = condition.end_token_index;
    auto block_start = tokens[block_start_index];
    if (block_start.type != TokenType::OpenCurly) {
        return ParseError {
            "expected '{'",
            "helium requires '{' after condition for if statements",
            block_start,
        };
    }
    auto block = TRY(parse_block(tokens, block_start_index));

    auto end = block.end_token_index;
    auto if_statement = If {
        condition.release_as_rvalue(),
        block.release_as_block(),
    };
    return Expression(std::move(if_statement), start, end);
}

static ParseSingleItemResult parse_while(Tokens const& tokens,
    u32 start)
{
    auto condition = TRY(parse_rvalue(tokens, start + 1));
    auto block_start_index = condition.end_token_index;
    auto block_start = tokens[block_start_index];
    if (block_start.type != TokenType::OpenCurly) {
        return ParseError {
            "expected '{'",
            "helium requires '{' after condition for loops",
            block_start,
        };
    }
    auto block = TRY(parse_block(tokens, block_start_index));

    auto end = block.end_token_index;
    auto while_ = While { condition.release_as_rvalue(),
        block.release_as_block() };
    return Expression(std::move(while_), start, end);
}

static ParseSingleItemResult parse_import_c(Tokens const& tokens,
    u32 start)
{
    auto left_paren_index = start + 1;
    auto left_paren = tokens[left_paren_index];
    if (left_paren.type != TokenType::OpenParen) {
        return ParseError {
            "expected '('",
            "did you forget a parenthesis?",
            left_paren,
        };
    }

    auto header_index = left_paren_index + 1;
    auto header = tokens[header_index];
    if (header.type != TokenType::Quoted) {
        return ParseError {
            "expected quoted string",
            "system headers are also imported with '\"'"
            "quotes",
            header,
        };
    }

    auto right_paren_index = header_index + 1;
    auto right_paren = tokens[right_paren_index];
    if (right_paren.type != TokenType::CloseParen) {
        return ParseError {
            "expected ')'",
            "did you forget a parenthesis?",
            left_paren,
        };
    }

    auto semicolon_index = right_paren_index + 1;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.type != TokenType::Semicolon) {
        return ParseError {
            "expected ';'",
            "did you forget a semicolon?",
            header,
        };
    }

    auto import_c = ImportC { header };

    // NOTE: Swallow semicolon.
    return Expression(import_c, start, semicolon_index + 1);
}

static ParseSingleItemResult parse_pub_specifier(
    Tokens const& tokens, u32 start)
{
    auto fn_index = start + 1;
    auto fn = tokens[fn_index];
    if (fn.type != TokenType::Fn) {
        return ParseError {
            "expected fn",
            nullptr,
            fn,
        };
    }
    return parse_public_function(tokens, fn_index);
}

static ParseSingleItemResult parse_root_item(Tokens const& tokens,
    u32 start)
{
    auto token = tokens[start];

    if (token.type == TokenType::ImportC)
        return parse_import_c(tokens, start);

    if (token.type == TokenType::InlineC)
        return parse_inline_c(tokens, start);

    if (token.type == TokenType::Fn)
        return parse_private_function(tokens, start);

    if (token.type == TokenType::Pub)
        return parse_pub_specifier(tokens, start);

    return ParseError {
        "unexpected token",
        nullptr,
        token,
    };
}

struct Function {
    Token name {};
    Token return_type {};
    Parameters parameters {};
    Block block;
    u32 start_token_index { 0 };
    u32 end_token_index { 0 };
};

static Core::ErrorOr<Function, ParseError> parse_function(
    Tokens const& tokens, u32 start)
{
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.type != TokenType::Identifier) {
        return ParseError {
            "unexpected token",
            "expected function name",
            name,
        };
    }

    auto parameters_start = name_index + 1;
    auto starting_paren = tokens[parameters_start];
    if (starting_paren.type != TokenType::OpenParen) {
        return ParseError {
            "unexpected token",
            "expected '('",
            starting_paren,
        };
    }

    auto parameters = Parameters();
    u32 parameters_end = parameters_start + 1;
    while (parameters_end < tokens.size()) {
        auto token = tokens[parameters_end];
        if (token.type == TokenType::CloseParen)
            break;

        auto name_index = parameters_end;
        auto name = tokens[name_index];
        if (name.type != TokenType::Identifier) {
            return ParseError {
                "expected parameter name",
                nullptr,
                name,
            };
        }

        auto colon_index = name_index + 1;
        auto colon = tokens[colon_index];
        if (colon.type != TokenType::Colon) {
            return ParseError {
                "expected ':'",
                nullptr,
                colon,
            };
        }

        auto type_index = colon_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.type != TokenType::Identifier) {
            return ParseError {
                "expected type name",
                nullptr,
                type_token,
            };
        }
        parameters.push_back({ name, type_token });

        auto comma_or_paren_index = type_index + 1;
        auto comma_or_paren = tokens[comma_or_paren_index];
        if (comma_or_paren.type == TokenType::Comma) {
            // NOTE: Swallow comma.
            parameters_end = comma_or_paren_index + 1;
            continue;
        }

        if (comma_or_paren.type == TokenType::CloseParen) {
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
    if (arrow.type != TokenType::Arrow) {
        return ParseError {
            "unexpected token",
            "expected '->'",
            arrow,
        };
    }

    auto return_type_index = arrow_index + 1;
    auto return_type = tokens[return_type_index];
    if (return_type.type != TokenType::Identifier) {
        return ParseError {
            "unexpected token",
            "expected return type",
            return_type,
        };
    }

    auto block_start_index = return_type_index + 1;
    auto block_start = tokens[block_start_index];
    if (block_start.type != TokenType::OpenCurly) {
        return ParseError {
            "unexpected token",
            "expected '{'",
            block_start,
        };
    }

    auto block = TRY(parse_block(tokens, block_start_index));
    auto block_end_index = block.end_token_index;

    return Function {
        name,
        return_type,
        std::move(parameters),
        block.release_as_block(),
        start,
        block_end_index,
    };
}

static ParseSingleItemResult parse_public_function(
    Tokens const& tokens, u32 start)
{
    auto function = TRY(parse_function(tokens, start));
    auto public_function = PublicFunction {
        .name = function.name,
        .return_type = function.return_type,
        .parameters = function.parameters,
        .block = std::move(function.block),
    };
    return Expression {
        std::move(public_function),
        function.start_token_index,
        function.end_token_index,
    };
}

static ParseSingleItemResult parse_private_function(
    Tokens const& tokens, u32 start)
{
    auto function = TRY(parse_function(tokens, start));
    auto private_function = PrivateFunction {
        .name = function.name,
        .return_type = function.return_type,
        .parameters = function.parameters,
        .block = std::move(function.block),
    };
    return Expression {
        std::move(private_function),
        function.start_token_index,
        function.end_token_index,
    };
}

static ParseSingleItemResult parse_function_call(
    Tokens const& tokens, u32 start)
{
    auto function_name = tokens[start];

    auto left_paren_index = start + 1;
    auto left_paren = tokens[left_paren_index];
    if (left_paren.type != TokenType::OpenParen) {
        return ParseError {
            "expected left parenthesis",
            "did you mean to do a function call?",
            left_paren,
        };
    }

    auto call = FunctionCall { function_name, Expressions() };
    auto right_paren_index = left_paren_index + 1;
    if (tokens[right_paren_index].type == TokenType::CloseParen) {
        // NOTE: Swallow right parenthesis
        return Expression(call, start, right_paren_index + 1);
    }

    right_paren_index = left_paren_index;
    for (; right_paren_index < tokens.size();) {
        if (tokens[right_paren_index].type == TokenType::CloseParen)
            break;
        auto argument_index = right_paren_index + 1;
        auto argument = TRY(parse_prvalue(tokens, argument_index));
        right_paren_index = argument.end_token_index;
        call.arguments.push_back(std::move(argument));
    }

    auto right_paren = tokens[right_paren_index];
    if (right_paren.type != TokenType::CloseParen) {
        return ParseError {
            "expected right parenthesis",
            "did you mean to do a function call?",
            right_paren,
        };
    }

    // NOTE: Swallow right parenthesis
    return Expression(call, start, right_paren_index + 1);
}

static ParseSingleItemResult parse_return(Tokens const& tokens,
    u32 start)
{
    auto generic_rvalue = TRY(parse_rvalue(tokens, start + 1));
    auto end = generic_rvalue.end_token_index;
    auto rvalue = generic_rvalue.release_as_rvalue();

    auto return_expression = Return { std::move(rvalue) };

    return Expression(std::move(return_expression), start, end);
}

static ParseSingleItemResult parse_inline_c(Tokens const& tokens,
    u32 start)
{
    i32 level = 0;
    auto block_start_index = start + 1;
    auto end = block_start_index;
    if (tokens[block_start_index].type == TokenType::OpenCurly) {
        for (; end < tokens.size(); end++) {
            auto token = tokens[end];
            if (token.type == TokenType::OpenCurly)
                level++;
            if (token.type == TokenType::CloseCurly)
                level--;
            if (level == 0)
                break;
        }
        auto token = tokens[start + 1];
        auto last_token = tokens[end];
        token.start_index++;
        token.end_index = last_token.end_index - 1;
        return Expression(InlineC { token }, start, end + 1);
    }

    for (; end < tokens.size(); end++) {
        auto token = tokens[end];
        if (token.type == TokenType::OpenCurly)
            level++;
        if (token.type == TokenType::CloseCurly)
            level--;
        if (level == 0 && token.type == TokenType::Semicolon)
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
    token.end_index = last_token.end_index;
    return Expression(InlineC { token }, start, end + 1);
}

static ParseSingleItemResult parse_block(Tokens const& tokens,
    u32 start)
{
    auto block = Block();
    auto end = start + 1;
    for (; end < tokens.size();) {
        if (tokens[end].type == TokenType::CloseCurly)
            break;

        if (tokens[end].type == TokenType::InlineC) {
            auto inline_c = TRY(parse_inline_c(tokens, end));
            end = inline_c.end_token_index;
            block.expressions.push_back(std::move(inline_c));
            continue;
        }

        if (tokens[end].type == TokenType::OpenCurly) {
            auto sub_block = TRY(parse_block(tokens, end));
            end = sub_block.end_token_index + 1;
            block.expressions.push_back(std::move(sub_block));
            continue;
        }

        if (tokens[end].type == TokenType::Let) {
            auto variable = TRY(parse_variable(tokens, end));
            end = variable.end_token_index;
            block.expressions.push_back(std::move(variable));
            continue;
        }

        if (tokens[end].type == TokenType::Var) {
            auto variable = TRY(parse_variable(tokens, end));
            end = variable.end_token_index;
            block.expressions.push_back(std::move(variable));
            continue;
        }

        if (tokens[end].type == TokenType::Return) {
            auto return_expression = TRY(parse_return(tokens, end));
            end = return_expression.end_token_index;

            if (tokens[end].type != TokenType::Semicolon) {
                return ParseError {
                    "expected ';'",
                    "did you forget a semicolon?",
                    tokens[end],
                };
            }
            end++; // NOTE: Swallow semicolon.

            block.expressions.push_back(
                std::move(return_expression));
            continue;
        }

        if (tokens[end].type == TokenType::Identifier) {
            if (tokens[end + 1].type == TokenType::Assign) {
                auto rvalue = TRY(parse_rvalue(tokens, end + 2));
                // Note: Swallow semicolon.
                end = rvalue.end_token_index + 1;
                block.expressions.push_back(std::move(rvalue));
                continue;
            }

            auto call = TRY(parse_function_call(tokens, end));
            end = call.end_token_index;
            block.expressions.push_back(std::move(call));

            if (tokens[end].type != TokenType::Semicolon) {
                return ParseError {
                    "expected ';'",
                    "did you forget a semicolon?",
                    tokens[end],
                };
            }
            end++; // NOTE: Swallow semicolon.
            continue;
        }

        if (tokens[end].type == TokenType::If) {
            auto if_ = TRY(parse_if(tokens, end));
            end = if_.end_token_index;
            block.expressions.push_back(std::move(if_));
            continue;
        }

        if (tokens[end].type == TokenType::While) {
            auto while_ = TRY(parse_while(tokens, end));
            end = while_.end_token_index;
            block.expressions.push_back(std::move(while_));
            continue;
        }

        return ParseError {
            "unexpected token",
            nullptr,
            tokens[end],
        };
    }
    end++; // NOTE: Swallow close curly
    return Expression(std::move(block), start, end);
}

static ParseSingleItemResult parse_rvalue(Tokens const& tokens,
    u32 start)
{
    auto rvalue = RValue();

    auto end = start;
    for (; end < tokens.size();) {
        if (tokens[end].type == TokenType::Semicolon)
            break;
        if (tokens[end].type == TokenType::OpenCurly)
            break;

        if (tokens[end].type == TokenType::InlineC) {
            auto inline_c = TRY(parse_inline_c(tokens, end));
            end = inline_c.end_token_index;
            rvalue.expressions.push_back(inline_c);
            // NOTE: Unconsume ';'
            return Expression { std::move(rvalue), start, end - 1 };
        }

        if (tokens[end].type == TokenType::Uninitialized) {
            auto open_paren_index = end + 1;
            auto open_paren = tokens[open_paren_index];
            if (open_paren.type != TokenType::OpenParen) {
                return ParseError {
                    "expected '('",
                    nullptr,
                    open_paren,
                };
            }

            auto close_paren_index = open_paren_index + 1;
            auto close_paren = tokens[close_paren_index];
            if (close_paren.type != TokenType::CloseParen) {
                return ParseError {
                    "expected ')'",
                    nullptr,
                    close_paren,
                };
            }
            end = close_paren_index;
            rvalue.expressions.emplace_back(Block {}, end, end + 1);
            end = close_paren_index + 1;
            Block block {};
            continue;
        }

        if (tokens[end].type == TokenType::Number) {
            auto literal = Literal { tokens[end] };
            rvalue.expressions.push_back({ literal, end, end + 1 });
            end = end + 1;
            continue;
        }

        if (tokens[end].type == TokenType::Plus) {
            // FIXME: Make this a unary operation.
            auto literal = Literal { tokens[end] };
            rvalue.expressions.push_back({ literal, end, end + 1 });
            end = end + 1;
            continue;
        }

        if (tokens[end].type == TokenType::Minus) {
            // FIXME: Make this a unary operation.
            auto literal = Literal { tokens[end] };
            rvalue.expressions.push_back({ literal, end, end + 1 });
            end = end + 1;
            continue;
        }

        if (tokens[end].type == TokenType::LessThanOrEqual) {
            // FIXME: Make this a unary operation.
            auto literal = Literal { tokens[end] };
            rvalue.expressions.push_back({ literal, end, end + 1 });
            end = end + 1;
            continue;
        }

        if (tokens[end].type == TokenType::GreaterThan) {
            // FIXME: Make this a unary operation.
            auto literal = Literal { tokens[end] };
            rvalue.expressions.push_back({ literal, end, end + 1 });
            end = end + 1;
            continue;
        }

        if (tokens[end].type == TokenType::Equals) {
            // FIXME: Make this a unary operation.
            auto literal = Literal { tokens[end] };
            rvalue.expressions.push_back({ literal, end, end + 1 });
            end = end + 1;
            continue;
        }

        if (tokens[end].type == TokenType::Quoted) {
            auto literal = Literal { tokens[end] };
            rvalue.expressions.push_back({ literal, end, end + 1 });
            end = end + 1;
            continue;
        }

        if (tokens[end].type == TokenType::Identifier) {
            if (tokens[end + 1].type == TokenType::OpenParen) {
                auto call = TRY(parse_function_call(tokens, end));
                end = call.end_token_index;
                rvalue.expressions.push_back(std::move(call));
            } else {
                auto value = LValue { tokens[end] };
                auto expression = Expression(value, end, end + 1);
                end = expression.end_token_index;
                rvalue.expressions.push_back(std::move(expression));
            }
            continue;
        }

        auto parse_error = ParseError { "foo", "bar", tokens[end] };
        return ParseError {
            "expected ';' or '{'",
            "did you forget a semicolon?",
            tokens[end],
        };
    }

    if (tokens[end].type == TokenType::Semicolon)
        return Expression { std::move(rvalue), start, end };

    if (tokens[end].type == TokenType::OpenCurly)
        return Expression { std::move(rvalue), start, end };

    return ParseError {
        "expected ';' or '{'",
        "did you forget a semicolon?",
        tokens[end],
    };
}

static ParseSingleItemResult parse_prvalue(Tokens const& tokens,
    u32 start)
{
    auto rvalue = RValue();

    auto end = start;
    for (; end < tokens.size();) {
        if (tokens[end].type == TokenType::Comma)
            break;
        if (tokens[end].type == TokenType::CloseParen)
            break;

        if (tokens[end].type == TokenType::Number) {
            auto literal = Literal { tokens[end] };
            rvalue.expressions.push_back({ literal, end, end + 1 });
            end = end + 1;
            continue;
        }

        if (tokens[end].type == TokenType::RefMut) {
            auto token = tokens[end];
            token.end_index = token.start_index + 1;
            auto literal = Literal { token };
            rvalue.expressions.push_back({ literal, end, end + 1 });
            end = end + 1;
            continue;
        }

        if (tokens[end].type == TokenType::Ampersand) {
            auto token = tokens[end];
            token.end_index = token.start_index + 1;
            auto literal = Literal { token };
            rvalue.expressions.push_back({ literal, end, end + 1 });
            end = end + 1;
            continue;
        }

        if (tokens[end].type == TokenType::Plus) {
            // FIXME: Make this a unary operation.
            auto literal = Literal { tokens[end] };
            rvalue.expressions.push_back({ literal, end, end + 1 });
            end = end + 1;
            continue;
        }

        if (tokens[end].type == TokenType::Minus) {
            // FIXME: Make this a unary operation.
            auto literal = Literal { tokens[end] };
            rvalue.expressions.push_back({ literal, end, end + 1 });
            end = end + 1;
            continue;
        }

        if (tokens[end].type == TokenType::Quoted) {
            auto literal = Literal { tokens[end] };
            rvalue.expressions.push_back({ literal, end, end + 1 });
            end = end + 1;
            continue;
        }

        if (tokens[end].type == TokenType::Identifier) {
            if (tokens[end + 1].type == TokenType::OpenParen) {
                auto function_call
                    = TRY(parse_function_call(tokens, end));
                end = function_call.end_token_index;
                rvalue.expressions.push_back(
                    std::move(function_call));
            } else {
                auto value = LValue { tokens[end] };
                auto expression = Expression(value, end, end + 1);
                end = expression.end_token_index;
                rvalue.expressions.push_back(std::move(expression));
            }
            continue;
        }

        return ParseError {
            "expected ',' or ')'",
            "did you forget a comma?",
            tokens[end],
        };
    }

    if (tokens[end].type == TokenType::Comma)
        return Expression { std::move(rvalue), start, end };
    if (tokens[end].type == TokenType::CloseParen)
        return Expression { std::move(rvalue), start, end };

    return ParseError {
        "expected ',' or ')'",
        "did you forget a comma?",
        tokens[end],
    };
}

static ParseSingleItemResult parse_variable(Tokens const& tokens,
    u32 start)
{
    auto type = tokens[start];
    auto name_index = start + 1;
    auto name = tokens[name_index];
    if (name.type != TokenType::Identifier) {
        return ParseError {
            "unexpected token",
            "expected variable name",
            name,
        };
    }

    auto colon_or_assign_index = name_index + 1;
    auto rvalue_start_index = colon_or_assign_index + 1;
    auto colon_or_assign = tokens[colon_or_assign_index];
    if (colon_or_assign.type == TokenType::Colon) {
        auto type_index = colon_or_assign_index + 1;
        auto type_token = tokens[type_index];
        if (type_token.type != TokenType::Identifier) {
            return ParseError {
                "expected type name",
                nullptr,
                type_token,
            };
        }
        type = type_token;

        auto assign_index = type_index + 1;
        auto assign = tokens[assign_index];
        if (assign.type != TokenType::Assign) {
            return ParseError {
                "expected '='",
                nullptr,
                assign,
            };
        }
        rvalue_start_index = assign_index + 1;
    } else if (colon_or_assign.type != TokenType::Assign) {
        return ParseError {
            "expected ':', or '='",
            nullptr,
            colon_or_assign,
        };
    }

    auto value = TRY(parse_rvalue(tokens, rvalue_start_index));
    auto rvalue_end_index = value.end_token_index;
    auto rvalue = value.release_as_rvalue();

    auto semicolon_index = rvalue_end_index;
    auto semicolon = tokens[semicolon_index];
    if (semicolon.type != TokenType::Semicolon) {
        auto rvalue = tokens[rvalue_end_index];
        return ParseError {
            "expected ';' after rvalue",
            "did you forget a semicolon?",
            rvalue,
        };
    }
    // NOTE: Swallow semicolon;
    auto end = semicolon_index + 1;

    auto variable = VariableDeclaration {
        name,
        type,
        std::move(rvalue),
    };
    return Expression(std::move(variable), start, end);
}

Core::ErrorOr<void> ParseError::show(SourceFile source) const
{
    auto start = *Util::line_and_column_for(source.text,
        offending_token.start_index);
    auto end = *Util::line_and_column_for(source.text,
        offending_token.end_index);
    auto line = Util::fetch_line(source.text, start.line);

    std::cerr << "Parse error @ " << parser_function << ": "
              << message << " "
              << "[" << source.file_name << ':' << start.line << ':'
              << start.column << "]" << '\n';
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
