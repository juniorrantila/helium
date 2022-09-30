#pragma once
#include <Core/Vector.h>
#include <He/Context.h>
#include <He/Lexer.h>
#include <He/Parser.h>

namespace He {

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

struct TypecheckedExpressions {

    static Core::ErrorOr<TypecheckedExpressions> create()
    {
        return TypecheckedExpressions {
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
    }

    Tokens import_cs;
    Tokens inline_cs;

    FunctionForwardDeclarations
        private_function_forwards;

    FunctionForwardDeclarations
        private_c_function_forwards;

    FunctionForwardDeclarations
        public_function_forwards;

    FunctionForwardDeclarations
        public_c_function_forwards;

    EnumForwardDeclarations enum_forwards;
    StructForwardDeclarations struct_forwards;
    UnionForwardDeclarations union_forwards;
    VariantForwardDeclarations variant_forwards;

    void codegen(int out_fd, Context const&) const;
};

}
