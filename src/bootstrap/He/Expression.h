#pragma once
#include <He/Token.h>
#include <string_view>
#include <variant>
#include <vector>

namespace He {

struct ParsedExpressions;

enum class ExpressionType {
    Literal,

    PublicVariableDeclaration,
    PrivateVariableDeclaration,
    PublicConstantDeclaration,
    PrivateConstantDeclaration,

    StructDeclaration,

    LValue,
    RValue,

    If,
    While,

    Block,

    PrivateCFunction,
    PublicCFunction,
    PrivateFunction,
    PublicFunction,
    FunctionCall,

    Return,

    ImportC,
    InlineC,

    Invalid,
};

std::string_view expression_type_string(ExpressionType type);

struct Expression;
using Expressions = std::vector<Expression>;
struct Return;

struct Literal {
    Token token {};

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] Block {
    std::vector<Expression> expressions;

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
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

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] PrivateFunction {
    Parameters parameters {};
    Token name {};
    Token return_type {};
    Block block;

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] PublicFunction {
    Parameters parameters {};
    Token name {};
    Token return_type {};
    Block block;

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] PrivateCFunction {
    Parameters parameters {};
    Token name {};
    Token return_type {};
    Block block;

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] PublicCFunction {
    Parameters parameters {};
    Token name {};
    Token return_type {};
    Block block;

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] LValue {
    Token token {};

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] RValue {
    std::vector<Expression> expressions;

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] PrivateVariableDeclaration {
    RValue value;
    Token name {};
    Token type {};

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] PublicVariableDeclaration {
    RValue value;
    Token name {};
    Token type {};

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] PrivateConstantDeclaration {
    RValue value;
    Token name {};
    Token type {};

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] PublicConstantDeclaration {
    RValue value;
    Token name {};
    Token type {};

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] If {
    RValue condition;
    Block block;

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] While {
    RValue condition;
    Block block;

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] Return {
    RValue rvalue;

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] FunctionCall {
    Expressions arguments;
    Token name {};

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] ImportC {
    Token filename {};

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct [[gnu::packed]] InlineC {
    Token literal {};

    void dump(ParsedExpressions const&, std::string_view source, u32 indent) const;
};

struct Expression {
    constexpr Expression(Id<Literal> value, u32 start_index,
        u32 end_index)
        : m_storage(value)
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(PrivateVariableDeclaration&& value,
        u32 start_index, u32 end_index)
        : m_storage(std::move(value))
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(PublicVariableDeclaration&& value,
        u32 start_index, u32 end_index)
        : m_storage(std::move(value))
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(PrivateConstantDeclaration&& value,
        u32 start_index, u32 end_index)
        : m_storage(std::move(value))
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(PublicConstantDeclaration&& value,
        u32 start_index, u32 end_index)
        : m_storage(std::move(value))
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(StructDeclaration&& value, u32 start_index,
        u32 end_index)
        : m_storage(std::move(value))
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(LValue value, u32 start_index,
        u32 end_index)
        : m_storage(value)
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(RValue&& value, u32 start_index,
        u32 end_index)
        : m_storage(std::move(value))
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(If&& value, u32 start_index, u32 end_index)
        : m_storage(std::move(value))
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(While&& value, u32 start_index,
        u32 end_index)
        : m_storage(std::move(value))
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(Block&& value, u32 start_index,
        u32 end_index)
        : m_storage(std::move(value))
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(PrivateFunction&& value, u32 start_index,
        u32 end_index)
        : m_storage(std::move(value))
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(PublicFunction&& value, u32 start_index,
        u32 end_index)
        : m_storage(std::move(value))
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(PrivateCFunction&& value, u32 start_index,
        u32 end_index)
        : m_storage(std::move(value))
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(PublicCFunction&& value, u32 start_index,
        u32 end_index)
        : m_storage(std::move(value))
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(FunctionCall value, u32 start_index,
        u32 end_index)
        : m_storage(value)
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(Return value, u32 start_index,
        u32 end_index)
        : m_storage(value)
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(ImportC value, u32 start_index,
        u32 end_index)
        : m_storage(value)
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(InlineC value, u32 start_index,
        u32 end_index)
        : m_storage(value)
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr ExpressionType type() const
    {
        return (ExpressionType)m_storage.index();
    }

    constexpr Id<Literal> as_literal() const
    {
        return std::get<Id<Literal>>(m_storage);
    }

    constexpr PrivateVariableDeclaration const&
    as_private_variable_declaration() const
    {
        return std::get<PrivateVariableDeclaration>(m_storage);
    }

    constexpr PrivateVariableDeclaration
    release_as_private_variable_declaration()
    {
        return std::get<PrivateVariableDeclaration>(
            std::move(m_storage));
    }

    constexpr PublicVariableDeclaration const&
    as_public_variable_declaration() const
    {
        return std::get<PublicVariableDeclaration>(m_storage);
    }

    constexpr PublicVariableDeclaration
    release_as_public_variable_declaration()
    {
        return std::get<PublicVariableDeclaration>(
            std::move(m_storage));
    }

    constexpr PublicConstantDeclaration const&
    as_public_constant_declaration() const
    {
        return std::get<PublicConstantDeclaration>(m_storage);
    }

    constexpr PublicConstantDeclaration
    release_as_public_constant_declaration()
    {
        return std::get<PublicConstantDeclaration>(
            std::move(m_storage));
    }

    constexpr PrivateConstantDeclaration const&
    as_private_constant_declaration() const
    {
        return std::get<PrivateConstantDeclaration>(m_storage);
    }

    constexpr PrivateConstantDeclaration
    release_as_private_constant_declaration()
    {
        return std::get<PrivateConstantDeclaration>(
            std::move(m_storage));
    }

    constexpr StructDeclaration const& as_struct_declaration() const
    {
        return std::get<StructDeclaration>(m_storage);
    }

    constexpr LValue const& as_lvalue() const
    {
        return std::get<LValue>(m_storage);
    }

    constexpr RValue const& as_rvalue() const
    {
        return std::get<RValue>(m_storage);
    }

    constexpr RValue release_as_rvalue()
    {
        return std::get<RValue>(std::move(m_storage));
    }

    constexpr If const& as_if() const
    {
        return std::get<If>(m_storage);
    }

    constexpr While const& as_while() const
    {
        return std::get<While>(m_storage);
    }

    constexpr PrivateFunction const& as_private_function() const
    {
        return std::get<PrivateFunction>(m_storage);
    }

    constexpr PublicFunction const& as_public_function() const
    {
        return std::get<PublicFunction>(m_storage);
    }

    constexpr PrivateCFunction const& as_private_c_function() const
    {
        return std::get<PrivateCFunction>(m_storage);
    }

    constexpr PublicCFunction const& as_public_c_function() const
    {
        return std::get<PublicCFunction>(m_storage);
    }

    constexpr FunctionCall const& as_function_call() const
    {
        return std::get<FunctionCall>(m_storage);
    }

    constexpr Block const& as_block() const
    {
        return std::get<Block>(m_storage);
    }

    constexpr Block release_as_block()
    {
        return std::get<Block>(std::move(m_storage));
    }

    constexpr Return const& as_return() const
    {
        return std::get<Return>(m_storage);
    }

    constexpr ImportC const& as_import_c() const
    {
        return std::get<ImportC>(m_storage);
    }

    constexpr ImportC release_as_import_c()
    {
        auto import_c = std::get<ImportC>(std::move(m_storage));
        auto import_c_copy = import_c;
        import_c_copy.filename.start_index = 0;
        import_c_copy.filename.size = 0;
        m_storage = import_c_copy;
        return import_c;
    }

    constexpr InlineC const& as_inline_c() const
    {
        return std::get<InlineC>(m_storage);
    }

    constexpr InlineC release_as_inline_c()
    {
        auto inline_c = std::get<InlineC>(std::move(m_storage));
        auto inline_c_copy = inline_c;
        inline_c_copy.literal.start_index = 0;
        inline_c_copy.literal.size = 0;
        m_storage = inline_c_copy;
        return inline_c;
    }

    void dump(ParsedExpressions const&, std::string_view source,
        u32 indent = 0) const;

private:
    // clang-format off
    std::variant<
        Id<Literal>,
        PublicVariableDeclaration,
        PrivateVariableDeclaration,
        PublicConstantDeclaration,
        PrivateConstantDeclaration,
        StructDeclaration,
        LValue,
        RValue,
        If,
        While,
        Block,
        PrivateCFunction,
        PublicCFunction,
        PrivateFunction,
        PublicFunction,
        FunctionCall,
        Return,
        ImportC,
        InlineC
    > m_storage {};
    // clang-format on

public:
    u32 start_token_index { 0 };
    u32 end_token_index { 0 };
};

struct ParsedExpressions {
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
    std::vector<T> name

    SOA_MEMBER(Literal, literals);

#undef SOA_MEMBER
    std::vector<Expression> expressions;
};

}
