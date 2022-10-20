#pragma once
#include <Core/Vector.h>
#include <He/Context.h>
#include <He/Lexer.h>
#include <He/Parser.h>

namespace He {

struct CheckedExpression;
using CheckedExpressions = Core::Vector<CheckedExpression>;
struct TypecheckedExpressions;

using TypeId = Id<StringView>;

#define CHECKED_EXPRESSIONS                           \
    X(CheckedUninitialized, uninitialized)            \
    X(CheckedLiteral, literal)                        \
                                                      \
    X(CheckedPrivateConstantDeclaration,              \
        private_constant_declaration)                 \
    X(CheckedPrivateVariableDeclaration,              \
        private_variable_declaration)                 \
    X(CheckedPublicConstantDeclaration,               \
        public_constant_declaration)                  \
    X(CheckedPublicVariableDeclaration,               \
        public_variable_declaration)                  \
                                                      \
    X(CheckedVariableAssignment, variable_assignment) \
                                                      \
    X(CheckedStructDeclaration, struct_declaration)   \
    X(CheckedStructInitializer, struct_initializer)   \
                                                      \
    X(CheckedMemberAccess, member_access)             \
    X(CheckedArrayAccess, array_access)               \
                                                      \
    X(CheckedEnumDeclaration, enum_declaration)       \
    X(CheckedUnionDeclaration, union_declaration)     \
    X(CheckedVariantDeclaration, variant_declaration) \
                                                      \
    X(CheckedLValue, lvalue)                          \
    X(CheckedRValue, rvalue)                          \
                                                      \
    X(CheckedIf, if_statement)                        \
    X(CheckedReturn, return_statement)                \
    X(CheckedWhile, while_statement)                  \
                                                      \
    X(CheckedBlock, block)                            \
                                                      \
    X(CheckedFunctionCall, function_call)             \
    X(CheckedPrivateCFunction, private_c_function)    \
    X(CheckedPrivateFunction, private_function)       \
    X(CheckedPublicCFunction, public_c_function)      \
    X(CheckedPublicFunction, public_function)         \
                                                      \
    X(CheckedImportC, import_c)                       \
    X(CheckedInlineC, inline_c)                       \
                                                      \
    X(CheckedMoved, moved_value)                      \
    X(CheckedInvalid, invalid)

enum class CheckedExpressionType : u8 {
#define X(T, ...) T,
    CHECKED_EXPRESSIONS
#undef X
};

#define FORWARD_DECLARE(T) struct T
#define X(T, ...) FORWARD_DECLARE(T);
CHECKED_EXPRESSIONS
#undef FORWARD_DECLARE
#undef X

constexpr StringView checked_expression_type_string(
    CheckedExpressionType type)
{
    switch (type) {
#define X(T, ...) \
    case CheckedExpressionType::T: return #T##sv;
        CHECKED_EXPRESSIONS
#undef X
    }
}

struct CheckedLiteral {
    Token token {};
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedBlock {
    Id<CheckedExpressions> expressions;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedUninitialized {
    TypeId type;

    static void dump(CheckedExpressions const&, StringView source,
        u32 indent);
};

struct CheckedVariable {
    Token name {};
    TypeId type;
};

struct CheckedParameter {
    Token name {};
    TypeId type;
};
using CheckedParameters = Core::Vector<CheckedParameter>;

struct CheckedMember {
    Token name {};
    TypeId type;
};
using CheckedMembers = Core::Vector<CheckedMember>;

struct CheckedStructDeclaration {
    Token name {};
    Id<CheckedMembers> members;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedEnumDeclaration {
    Token name {};
    Id<CheckedMembers> members;
    TypeId underlying_type;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedUnionDeclaration {
    Token name {};
    Id<CheckedMembers> members;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedVariantDeclaration {
    Token name {};
    Id<CheckedMembers> members;
    TypeId tag_underlying_type;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedInitializer {
    Token name {};
    Id<CheckedRValue> value {};
    TypeId type;
};
using CheckedInitializers = Core::Vector<Initializer>;

struct CheckedStructInitializer {
    TypeId type;
    Id<CheckedInitializers> initializers;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedMemberAccess {
    Id<CheckedMembers> members;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedArrayAccess {
    Token name;
    Id<CheckedRValue> index;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedPrivateFunction {
    Token name {};
    TypeId return_type;
    Id<CheckedParameters> parameters;
    Id<CheckedBlock> block;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedPublicFunction {
    Token name {};
    TypeId return_type;
    Id<CheckedParameters> parameters;
    Id<CheckedBlock> block;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedPrivateCFunction {
    Token name {};
    TypeId return_type;
    Id<CheckedParameters> parameters;
    Id<CheckedBlock> block;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedPublicCFunction {
    Token name {};
    TypeId return_type;
    Id<CheckedParameters> parameters;
    Id<CheckedBlock> block;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedLValue {
    Token token {};
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedRValue {
    Id<CheckedExpressions> expressions;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedPrivateVariableDeclaration {
    Token name {};
    TypeId type;
    Id<CheckedExpression> value;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedPublicVariableDeclaration {
    Token name {};
    TypeId type;
    Id<CheckedExpression> value;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedPrivateConstantDeclaration {
    Token name {};
    TypeId type;
    Id<CheckedExpression> value;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedPublicConstantDeclaration {
    Token name {};
    TypeId type;
    Id<CheckedExpression> value;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedVariableAssignment {
    Token name {};
    TypeId type;
    Id<CheckedRValue> value;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedIf {
    Id<RValue> condition;
    TypeId type;
    Id<CheckedBlock> block;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedWhile {
    Id<CheckedRValue> condition;
    Id<CheckedBlock> block;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedReturn {
    Id<CheckedExpression> value;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedFunctionCall {
    Token name {};
    Id<CheckedExpressions> arguments;
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedImportC {
    Token filename {};
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedInlineC {
    Token literal {};
    TypeId type;

    void dump(CheckedExpressions const&, StringView source,
        u32 indent) const;
};

struct CheckedMoved {
    static void dump(CheckedExpressions const&, StringView, u32) { }
};

struct CheckedInvalid {
    static void dump(CheckedExpressions const&, StringView, u32);
};

struct FunctionForwardDeclaration {
    Token name {};
    Token return_type {};
    Parameters const& parameters;
};
using FunctionForwardDeclarations
    = Core::Vector<FunctionForwardDeclaration>;

struct StructForwardDeclaration {
    Token name {};
};
using StructForwardDeclarations
    = Core::Vector<StructForwardDeclaration>;

struct EnumForwardDeclaration {
    Token name {};
};
using EnumForwardDeclarations
    = Core::Vector<EnumForwardDeclaration>;

struct UnionForwardDeclaration {
    Token name {};
};
using UnionForwardDeclarations
    = Core::Vector<UnionForwardDeclaration>;

struct VariantForwardDeclaration {
    Token name {};
};
using VariantForwardDeclarations
    = Core::Vector<VariantForwardDeclaration>;

struct CheckedExpression {
#define VARIANT(T, name)                                      \
    constexpr CheckedExpression(Id<T> value, u32 start_index, \
        u32 end_index)                                        \
        : name(value)                                         \
        , start_token_index(start_index)                      \
        , end_token_offset(end_index - start_index)           \
        , m_type(CheckedExpressionType::T)                    \
    {                                                         \
    }                                                         \
    constexpr Id<T> const& as_##name() const { return name; } \
    constexpr Id<T> release_as_##name()                       \
    {                                                         \
        auto value = name;                                    \
        m_type = CheckedExpressionType::CheckedMoved;         \
        return value;                                         \
    }

#define X(T, name, ...) VARIANT(T, name);
    CHECKED_EXPRESSIONS
#undef X
#undef VARIANT

    void dump(ParsedExpressions const&, StringView source,
        u32 indent = 0) const;

private:
    union {
#define X(T, name, ...) Id<T> name;
        CHECKED_EXPRESSIONS
#undef X
    };

public:
    u32 start_token_index { 0 };
    u32 end_token_offset : 24 { 0 };

    constexpr u32 end_token_index() const
    {
        return start_token_index + end_token_offset;
    }

    constexpr CheckedExpressionType type() const { return m_type; }
    constexpr Id<CheckedBlock> parent() const { return m_parent; }

private:
    CheckedExpressionType m_type { ExpressionType::Invalid };
    Id<CheckedBlock> m_parent;
};

struct TypecheckedExpressions {
public:
    static Core::ErrorOr<TypecheckedExpressions> create()
    {
#define X(T, name, ...) .name##s = TRY(Core::Vector<T>::create()),
        // clang-format off
        using BlockData = Core::Vector<CheckedExpressions>;
        using Initializerss = Core::Vector<CheckedInitializers>;
        using Memberss = Core::Vector<CheckedMembers>;
        using Parameterss = Core::Vector<CheckedParameters>;
        using MemberAccessData = Core::Vector<Tokens>;
        return TypecheckedExpressions {
            EXPRESSIONS
            .late_expressions = TRY(CheckedExpressions::create()),
            .block_data = TRY(BlockData::create()),
            .initializerss = TRY(Initializerss::create()),
            .memberss = TRY(Memberss::create()),
            .parameterss = TRY(Parameterss::create()),
            .member_access_data = TRY(MemberAccessData::create()),
            .expressions = TRY(CheckedExpressions::create()),

            TRY(Tokens::create()),
            TRY(Tokens::create()),
            TRY(FunctionForwardDeclarations::create()),
            TRY(FunctionForwardDeclarations::create()),
            TRY(FunctionForwardDeclarations::create()),
            TRY(FunctionForwardDeclarations::create()),
            TRY(EnumForwardDeclarations::create()),
            TRY(StructForwardDeclarations::create()),
            TRY(UnionForwardDeclarations::create()),
            TRY(VariantForwardDeclarations::create()),
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
    constexpr Core::ErrorOr<Id<T>> append(T value)         \
    {                                                      \
        static_assert(std::is_trivially_copyable_v<T>);    \
        return name.append(value);                         \
    }                                                      \
    Core::Vector<T> name

#define NONTRIVIAL_SOA_MEMBER(T, name)                     \
    constexpr T& operator[](Id<T> id) { return name[id]; } \
    constexpr T const& operator[](Id<T> id) const          \
    {                                                      \
        return name[id];                                   \
    }                                                      \
    constexpr Core::ErrorOr<Id<T>> append(T&& value)       \
    {                                                      \
        static_assert(!std::is_trivially_copyable_v<T>);   \
        return name.append(std::move(value));              \
    }                                                      \
    Core::Vector<T> name

#define X(T, name, ...) SOA_MEMBER(T, name##s);
    EXPRESSIONS
#undef X

    SOA_MEMBER(CheckedExpression, late_expressions);
    NONTRIVIAL_SOA_MEMBER(CheckedExpressions, block_data);
    NONTRIVIAL_SOA_MEMBER(CheckedInitializers, initializerss);
    NONTRIVIAL_SOA_MEMBER(CheckedMembers, memberss);
    NONTRIVIAL_SOA_MEMBER(CheckedParameters, parameterss);
    NONTRIVIAL_SOA_MEMBER(Tokens, member_access_data);

    Core::ErrorOr<CheckedBlock> create_block()
    {
        return CheckedBlock {
            TRY(append(TRY(CheckedExpressions::create(8)))),
            TypeId::invalid(),
        };
    }

    Core::ErrorOr<CheckedRValue> create_rvalue()
    {
        return CheckedRValue {
            TRY(append(TRY(CheckedExpressions::create(8)))),
            TypeId::invalid(),
        };
    }

    Core::ErrorOr<CheckedMemberAccess> create_member_access()
    {
        return CheckedMemberAccess {
            TRY(append(TRY(CheckedMembers::create(8)))),
            TypeId::invalid(),
        };
    }

    static constexpr Id<CheckedUninitialized>
    uninitialized_expression()
    {
        return Id<CheckedUninitialized> { 0 };
    }

#undef NONTRIVIAL_SOA_MEMBER
#undef SOA_MEMBER

    CheckedExpressions expressions;

    Tokens import_c_quoted_filenames;
    Tokens inline_c_texts;

    FunctionForwardDeclarations private_function_forwards;
    FunctionForwardDeclarations private_c_function_forwards;
    FunctionForwardDeclarations public_function_forwards;
    FunctionForwardDeclarations public_c_function_forwards;

    EnumForwardDeclarations enum_forwards;
    StructForwardDeclarations struct_forwards;
    UnionForwardDeclarations union_forwards;
    VariantForwardDeclarations variant_forwards;

    void codegen(int out_fd, Context const&) const;
};

}
