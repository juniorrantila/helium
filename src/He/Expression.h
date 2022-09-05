#pragma once
#include <vector>
#include <He/Token.h>
#include <variant>

namespace He {

enum class ExpressionType {
    Literal,
    VariableDeclaration,

    LValue,
    RValue,

    If,
    While,

    Block,

    PrivateFunction,
    PublicFunction,
    FunctionCall,

    Return,

    ImportC,
    InlineC,

    Invalid,
};

struct Expression;
using Expressions = std::vector<Expression>;
struct Return;

struct Literal {
    Token token {};

    void dump(std::string_view source, u32 indent) const;
};

struct Block {
    std::vector<Expression> expressions;

    void dump(std::string_view source, u32 indent) const;
};

struct Variable {
    Token name {};
    Token type {};
};
using Parameters = std::vector<Variable>;

struct PrivateFunction {
    Token name {};
    Token return_type {};
    Parameters parameters {};
    Block block;

    void dump(std::string_view source, u32 indent) const;
};

struct PublicFunction {
    Token name {};
    Token return_type {};
    Parameters parameters {};
    Block block;

    void dump(std::string_view source, u32 indent) const;
};

struct LValue {
    Token token {};

    void dump(std::string_view source, u32 indent) const;
};

struct RValue {
    std::vector<Expression> expressions;

    void dump(std::string_view source, u32 indent) const;
};

struct VariableDeclaration {
    Token name {};
    Token type {};
    RValue value;

    void dump(std::string_view source, u32 indent) const;
};

struct If {
    RValue condition;
    Block block;

    void dump(std::string_view source, u32 indent) const;
};

struct While {
    RValue condition;
    Block block;

    void dump(std::string_view source, u32 indent) const;
};

struct Return {
    RValue rvalue;

    void dump(std::string_view source, u32 indent) const;
};

struct FunctionCall {
    Token name {};
    Expressions arguments;

    void dump(std::string_view source, u32 indent) const;
};

struct ImportC {
    Token filename {};

    void dump(std::string_view source, u32 indent) const;
};

struct InlineC {
    Token literal {};

    void dump(std::string_view source, u32 indent) const;
};

struct Expression {
    constexpr Expression(Literal value, u32 start_index,
        u32 end_index)
        : m_storage(value)
        , start_token_index(start_index)
        , end_token_index(end_index)
    {
    }

    constexpr Expression(VariableDeclaration&& value,
        u32 start_index, u32 end_index)
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

    constexpr Literal const& as_literal() const
    {
        return std::get<Literal>(m_storage);
    }

    constexpr Literal release_as_literal()
    {
        return std::get<Literal>(std::move(m_storage));
    }

    constexpr VariableDeclaration const&
    as_variable_declaration() const
    {
        return std::get<VariableDeclaration>(m_storage);
    }

    constexpr VariableDeclaration release_as_variable_declaration()
    {
        return std::get<VariableDeclaration>(std::move(m_storage));
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
        import_c_copy.filename.end_index = 0;
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
        inline_c_copy.literal.end_index = 0;
        m_storage = inline_c_copy;
        return inline_c;
    }

    void dump(std::string_view source, u32 indent = 0) const;

private:
    // clang-format off
    std::variant<
        Literal,
        VariableDeclaration,
        LValue,
        RValue,
        If,
        While,
        Block,
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

}
