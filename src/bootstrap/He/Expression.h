#pragma once
#include <Core/Vector.h>
#include <He/Token.h>
#include <stdio.h>
#include <string_view>
#include <vector>

namespace He {

struct ParsedExpressions;

#define EXPRESSIONS                                             \
    X(CompilerProvidedU64, compiler_provided_u64)               \
    X(Literal, literal)                                         \
                                                                \
    X(PrivateConstantDeclaration, private_constant_declaration) \
    X(PrivateVariableDeclaration, private_variable_declaration) \
    X(PublicConstantDeclaration, public_constant_declaration)   \
    X(PublicVariableDeclaration, public_variable_declaration)   \
                                                                \
    X(StructDeclaration, struct_declaration)                    \
    X(StructInitializer, struct_initializer)                    \
                                                                \
    X(LValue, lvalue)                                           \
    X(RValue, rvalue)                                           \
                                                                \
    X(If, if_statement)                                         \
    X(Return, return_statement)                                 \
    X(While, while_statement)                                   \
                                                                \
    X(Block, block)                                             \
                                                                \
    X(FunctionCall, function_call)                              \
    X(PrivateCFunction, private_c_function)                     \
    X(PrivateFunction, private_function)                        \
    X(PublicCFunction, public_c_function)                       \
    X(PublicFunction, public_function)                          \
                                                                \
    X(ImportC, import_c)                                        \
    X(InlineC, inline_c)                                        \
                                                                \
    X(Moved, moved_value)                                       \
    X(Invalid, invalid)

enum class ExpressionType : u8 {
#define X(T, ...) T,
    EXPRESSIONS
#undef X
};

#define FORWARD_DECLARE(T) struct T
#define X(T, ...) FORWARD_DECLARE(T);
EXPRESSIONS
#undef FORWARD_DECLARE
#undef X

constexpr std::string_view expression_type_string(
    ExpressionType type)
{
    switch (type) {
#define X(T, ...) \
    case ExpressionType::T: return #T;
        EXPRESSIONS
#undef X
    }
}

struct Expression;
using Expressions = Core::Vector<Expression>;

struct Literal {
    Token token {};

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

// FIXME: Fix memory leak.
struct [[gnu::packed]] Block {
    Expressions expressions;

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct CompilerProvidedU64 {
    u64 number;

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] Variable {
    Token name {};
    Token type {};
};
using Parameters = std::vector<Variable>;
using Variables = std::vector<Variable>;
using Members = std::vector<Variable>;
using Member = Variable;

struct [[gnu::packed]] StructDeclaration {
    Token name {};
    Members members {};

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct Initializer {
    Token name {};
    Id<RValue> value {};
};
using Initializers = std::vector<Initializer>;

struct [[gnu::packed]] StructInitializer {
    Initializers initializers {};
    Token type {};

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] PrivateFunction {
    Parameters parameters {};
    Token name {};
    Token return_type {};
    Id<Block> block;

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] PublicFunction {
    Parameters parameters {};
    Token name {};
    Token return_type {};
    Id<Block> block;

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] PrivateCFunction {
    Parameters parameters {};
    Token name {};
    Token return_type {};
    Id<Block> block;

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] PublicCFunction {
    Parameters parameters {};
    Token name {};
    Token return_type {};
    Id<Block> block;

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] LValue {
    Token token {};

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] RValue {
    std::vector<Expression> expressions;

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] PrivateVariableDeclaration {
    Token name {};
    Token type {};
    Id<Expression> value;

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] PublicVariableDeclaration {
    Token name {};
    Token type {};
    Id<Expression> value;

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] PrivateConstantDeclaration {
    Token name {};
    Token type {};
    Id<Expression> value;

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] PublicConstantDeclaration {
    Token name {};
    Token type {};
    Id<Expression> value;

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] If {
    Id<RValue> condition;
    Id<Block> block;

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] While {
    Id<RValue> condition;
    Id<Block> block;

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] Return {
    Id<Expression> value;

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] FunctionCall {
    Expressions arguments;
    Token name {};

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] ImportC {
    Token filename {};

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct [[gnu::packed]] InlineC {
    Token literal {};

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent) const;
};

struct Moved {
    static void dump(ParsedExpressions const&, std::string_view,
        u32)
    {
    }
};

struct Invalid {
    static void dump(ParsedExpressions const&, std::string_view,
        u32);
};

struct Expression {
#define VARIANT(T, name)                                      \
    constexpr Expression(Id<T> value, u32 start_index,        \
        u32 end_index)                                        \
        : name(value)                                         \
        , start_token_index(start_index)                      \
        , end_token_offset(end_index - start_index)           \
        , m_type(ExpressionType::T)                           \
    {                                                         \
    }                                                         \
    constexpr Id<T> const& as_##name() const { return name; } \
    constexpr Id<T> release_as_##name()                       \
    {                                                         \
        auto value = name;                                    \
        m_type = ExpressionType::Moved;                       \
        return value;                                         \
    }

#define X(T, name, ...) VARIANT(T, name);
    EXPRESSIONS
#undef X
#undef VARIANT

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent = 0) const;

private:
    union {
#define X(T, name, ...) Id<T> name;
        EXPRESSIONS
#undef X
    };

public:
    u32 start_token_index { 0 };
    u32 end_token_offset : 24 { 0 };

    constexpr u32 end_token_index() const
    {
        return start_token_index + end_token_offset;
    }

    constexpr ExpressionType type() const { return m_type; }

private:
    ExpressionType m_type { ExpressionType::Invalid };
};

struct ParsedExpressions {
private:
    constexpr ParsedExpressions(Expressions expressions)
        : expressions(expressions)
    {
    }

public:
    static Core::ErrorOr<ParsedExpressions> create()
    {
        return { (TRY(Expressions::create())) };
    }

    void destroy() const {
        for (auto function_call : function_calls)
            function_call.arguments.destroy();
        for (auto block : blocks)
            block.expressions.destroy();
        expressions.destroy();
    }

#define SOA_MEMBER(T, name)                                      \
    constexpr T& operator[](Id<T> id) { return name[id.raw()]; } \
    constexpr T const& operator[](Id<T> id) const                \
    {                                                            \
        return name[id.raw()];                                   \
    }                                                            \
    constexpr Id<T> append(T&& value)                            \
    {                                                            \
        auto id = Id<T>(name.size());                            \
        name.push_back(value);                                   \
        return id;                                               \
    }                                                            \
    constexpr Id<T> append(T const& value)                       \
    {                                                            \
        auto id = Id<T>(name.size());                            \
        name.push_back(value);                                   \
        return id;                                               \
    }                                                            \
    std::vector<T> name { }

#define X(T, name, ...) SOA_MEMBER(T, name##s);
    EXPRESSIONS
#undef X

    SOA_MEMBER(Expression, late_expressions);

#undef SOA_MEMBER
    Expressions expressions;
};

}
