#pragma once
#include "Token.h"
#include <Ty/Move.h>
#include <Ty/Vector.h>
#include <stdio.h>

namespace He {

struct ParsedExpressions;

#define EXPRESSIONS                                             \
    X(Uninitialized, uninitialized)                             \
    X(Literal, literal)                                         \
                                                                \
    X(PrivateConstantDeclaration, private_constant_declaration) \
    X(PrivateVariableDeclaration, private_variable_declaration) \
    X(PublicConstantDeclaration, public_constant_declaration)   \
    X(PublicVariableDeclaration, public_variable_declaration)   \
                                                                \
    X(VariableAssignment, variable_assignment)                  \
                                                                \
    X(StructDeclaration, struct_declaration)                    \
    X(StructInitializer, struct_initializer)                    \
                                                                \
    X(MemberAccess, member_access)                              \
    X(ArrayAccess, array_access)                                \
                                                                \
    X(EnumDeclaration, enum_declaration)                        \
    X(UnionDeclaration, union_declaration)                      \
    X(VariantDeclaration, variant_declaration)                  \
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

constexpr StringView expression_type_string(ExpressionType type)
{
    switch (type) {
#define X(T, ...) \
    case ExpressionType::T: return #T##sv;
        EXPRESSIONS
#undef X
    }
}

struct Expression;
using Expressions = Vector<Expression>;

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

struct Uninitialized {
    static void dump(ParsedExpressions const&, StringView source,
        u32 indent);
};

struct Variable {
    Token name {};
    Token type {};
};

struct Parameter {
    Token name {};
    Token type {};
};
using Parameters = Vector<Parameter>;

struct Member {
    Token name {};
    Token type {};
};
using Members = Vector<Member>;

struct StructDeclaration {
    Token name {};
    Id<Members> members;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct EnumDeclaration {
    Token name {};
    Token underlying_type {};
    Id<Members> members;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct UnionDeclaration {
    Token name {};
    Id<Members> members;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct VariantDeclaration {
    Token name {};
    Token tag_underlying_type {};
    Id<Members> members;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct Initializer {
    Token name {};
    Id<RValue> value {};
};
using Initializers = Vector<Initializer>;

struct StructInitializer {
    Token type {};
    Id<Initializers> initializers;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct MemberAccess {
    Id<Tokens> members;

    void dump(ParsedExpressions const&, StringView source,
        u32 indent) const;
};

struct ArrayAccess {
    Token name;
    Id<RValue> index;

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

struct VariableAssignment {
    Token name {};
    Id<RValue> value;

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
    static void dump(ParsedExpressions const&, StringView, u32) { }
};

struct Invalid {
    static void dump(ParsedExpressions const&, StringView, u32);
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

    constexpr static Expression garbage(u32 start, u32 end)
    {
        return Expression {
            ExpressionType::Invalid,
            start,
            end + 1,
        };
    }

private:
    constexpr Expression(ExpressionType type, u32 start, u32 end)
        : start_token_index(start)
        , end_token_offset(end - start)
        , m_type(type)
    {
    }

    ExpressionType m_type { ExpressionType::Invalid };
};

struct ParsedExpressions {
public:
    static ErrorOr<ParsedExpressions> create()
    {
#define X(T, name, ...) .name##s = TRY(Vector<T>::create()),
        // clang-format off
        using BlockData = Vector<Expressions>;
        using Initializerss = Vector<Initializers>;
        using Memberss = Vector<Members>;
        using Parameterss = Vector<Parameters>;
        using MemberAccessData = Vector<Tokens>;
        using InlineCS = Vector<InlineC>;
        using PrivateVariableDeclarations = Vector<PrivateVariableDeclaration>;
        using PublicVariableDeclarations = Vector<PublicVariableDeclaration>;
        using PrivateConstantDeclarations = Vector<PrivateConstantDeclaration>;
        using PublicConstantDeclarations = Vector<PublicConstantDeclaration>;
        return ParsedExpressions {
            EXPRESSIONS
            .late_expressions = TRY(Expressions::create()),
            .block_data = TRY(BlockData::create()),
            .initializerss = TRY(Initializerss::create()),
            .memberss = TRY(Memberss::create()),
            .parameterss = TRY(Parameterss::create()),
            .member_access_data = TRY(MemberAccessData::create()),
            .top_level_inline_cs = TRY(InlineCS::create()),
            .top_level_private_variables = TRY(PrivateVariableDeclarations::create()),
            .top_level_public_variables = TRY(PublicVariableDeclarations::create()),
            .top_level_private_constants = TRY(PrivateConstantDeclarations::create()),
            .top_level_public_constants = TRY(PublicConstantDeclarations::create()),
        };
        // clang-format on
#undef X
    }

#define SOA_MEMBER(T, name)                                \
    constexpr T& operator[](Id<T> id) { return name[id]; } \
    constexpr T const& operator[](Id<T> id) const          \
    {                                                      \
        return name[id];                                   \
    }                                                      \
    constexpr ErrorOr<Id<T>> append(T value)               \
    {                                                      \
        static_assert(std::is_trivially_copyable_v<T>);    \
        return name.append(value);                         \
    }                                                      \
    Vector<T> name

#define NONTRIVIAL_SOA_MEMBER(T, name)                     \
    constexpr T& operator[](Id<T> id) { return name[id]; } \
    constexpr T const& operator[](Id<T> id) const          \
    {                                                      \
        return name[id];                                   \
    }                                                      \
    constexpr ErrorOr<Id<T>> append(T&& value)             \
    {                                                      \
        static_assert(!std::is_trivially_copyable_v<T>);   \
        return name.append(move(value));                   \
    }                                                      \
    Vector<T> name

#define X(T, name, ...) SOA_MEMBER(T, name##s);
    EXPRESSIONS
#undef X

    SOA_MEMBER(Expression, late_expressions);
    NONTRIVIAL_SOA_MEMBER(Expressions, block_data);
    NONTRIVIAL_SOA_MEMBER(Initializers, initializerss);
    NONTRIVIAL_SOA_MEMBER(Members, memberss);
    NONTRIVIAL_SOA_MEMBER(Parameters, parameterss);
    NONTRIVIAL_SOA_MEMBER(Tokens, member_access_data);

    ErrorOr<Block> create_block()
    {
        return Block { TRY(append(TRY(Expressions::create(8)))) };
    }

    ErrorOr<RValue> create_rvalue()
    {
        return RValue { TRY(append(TRY(Expressions::create(8)))) };
    }

    ErrorOr<MemberAccess> create_member_access()
    {
        return MemberAccess { TRY(append(TRY(Tokens::create(8)))) };
    }

    static constexpr Id<Uninitialized> uninitialized_expression()
    {
        return Id<Uninitialized> { 0 };
    }

#undef NONTRIVIAL_SOA_MEMBER
#undef SOA_MEMBER

    void dump(StringView source) const;

    Vector<InlineC> top_level_inline_cs;

    Vector<PrivateVariableDeclaration> top_level_private_variables;
    Vector<PublicVariableDeclaration> top_level_public_variables;

    Vector<PrivateConstantDeclaration> top_level_private_constants;
    Vector<PublicConstantDeclaration> top_level_public_constants;
};

}
