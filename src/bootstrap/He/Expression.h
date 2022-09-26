#pragma once
#include <Core/Vector.h>
#include <He/Token.h>
#include <stdio.h>

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

constexpr StringView expression_type_string(
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

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct Block {
    Id<Expressions> expressions;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct CompilerProvidedU64 {
    u64 number;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct Variable {
    Token name {};
    Token type {};
};

struct Parameter {
    Token name {};
    Token type {};
};
using Parameters = Core::Vector<Parameter>;

struct Member {
    Token name {};
    Token type {};
};
using Members = Core::Vector<Member>;

struct StructDeclaration {
    Token name {};
    Id<Members> members;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct Initializer {
    Token name {};
    Id<RValue> value {};
};
using Initializers = Core::Vector<Initializer>;

struct StructInitializer {
    Token type {};
    Id<Initializers> initializers;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct PrivateFunction {
    Token name {};
    Token return_type {};
    Id<Parameters> parameters;
    Id<Block> block;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct PublicFunction {
    Token name {};
    Token return_type {};
    Id<Parameters> parameters;
    Id<Block> block;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct PrivateCFunction {
    Token name {};
    Token return_type {};
    Id<Parameters> parameters;
    Id<Block> block;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct PublicCFunction {
    Token name {};
    Token return_type {};
    Id<Parameters> parameters;
    Id<Block> block;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct LValue {
    Token token {};

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct RValue {
    Id<Expressions> expressions;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct PrivateVariableDeclaration {
    Token name {};
    Token type {};
    Id<Expression> value;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct PublicVariableDeclaration {
    Token name {};
    Token type {};
    Id<Expression> value;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct PrivateConstantDeclaration {
    Token name {};
    Token type {};
    Id<Expression> value;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct PublicConstantDeclaration {
    Token name {};
    Token type {};
    Id<Expression> value;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct If {
    Id<RValue> condition;
    Id<Block> block;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct While {
    Id<RValue> condition;
    Id<Block> block;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct Return {
    Id<Expression> value;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct FunctionCall {
    Token name {};
    Id<Expressions> arguments;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct ImportC {
    Token filename {};

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct InlineC {
    Token literal {};

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct Moved {
    static void dump(ParsedExpressions const&, StringView,
        u32)
    {
    }
};

struct Invalid {
    static void dump(ParsedExpressions const&, StringView,
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

    void dump(ParsedExpressions const&, StringView source,
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
public:
    static Core::ErrorOr<ParsedExpressions> create()
    {
#define X(T, name, ...) .name##s = TRY(Core::Vector<T>::create()),
        // clang-format off
        return ParsedExpressions {
            EXPRESSIONS
            .late_expressions = TRY(Expressions::create()),
            .block_data = TRY(Core::Vector<Expressions>::create()),
            .memberss = TRY(Core::Vector<Members>::create()),
            .initializerss = TRY(Core::Vector<Initializers>::create()),
            .parameterss = TRY(Core::Vector<Parameters>::create()),
            .expressions = TRY(Expressions::create()),
        };
        // clang-format on
#undef X
    }

#define SOA_MEMBER(T, name)                                    \
    constexpr T& operator[](Id<T> id) { return name[id]; }     \
    constexpr T const& operator[](Id<T> id) const              \
    {                                                          \
        return name[id];                                       \
    }                                                          \
    constexpr Core::ErrorOr<Id<T>> append(T&& value) requires( \
        !std::is_trivially_copyable_v<T>)                      \
    {                                                          \
        return name.append(std::move(value));                  \
    }                                                          \
    constexpr Core::ErrorOr<Id<T>> append(T value) requires(   \
        std::is_trivially_copyable_v<T>)                       \
    {                                                          \
        return name.append(std::move(value));                  \
    }                                                          \
    Core::Vector<T> name

#define X(T, name, ...) SOA_MEMBER(T, name##s);
    EXPRESSIONS
#undef X

    SOA_MEMBER(Expression, late_expressions);
    SOA_MEMBER(Expressions, block_data);
    SOA_MEMBER(Members, memberss);
    SOA_MEMBER(Initializers, initializerss);
    SOA_MEMBER(Parameters, parameterss);

    Core::ErrorOr<Block> create_block()
    {
        return Block { TRY(append(TRY(Expressions::create(8)))) };
    }

    Core::ErrorOr<RValue> create_rvalue()
    {
        return RValue { TRY(append(TRY(Expressions::create(8)))) };
    }

#undef SOA_MEMBER
    Expressions expressions;
};

}
