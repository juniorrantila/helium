#pragma once
#include <He/Context.h>
#include <He/Lexer.h>
#include <He/Parser.h>
#include <vector>

namespace He {

struct FunctionForwardDeclaration {
    Token name {};
    Token return_type {};
    Parameters const& parameters {};
};
using FunctionForwardDeclarations
    = std::vector<FunctionForwardDeclaration>;

struct StructForwardDeclaration {
    Token name {};
};
using StructForwardDeclarations
    = std::vector<StructForwardDeclaration>;

struct Codegen {
    std::vector<Token> import_c_filenames {};
    std::vector<Token> inline_c_expressions {};

    FunctionForwardDeclarations
        private_function_forward_declarations {};

    FunctionForwardDeclarations
        public_function_forward_declarations {};

    StructForwardDeclarations struct_forward_declarations {};

    void dump(int out_fd, Context const&) const;
};

}
