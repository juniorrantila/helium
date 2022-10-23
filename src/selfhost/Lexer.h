#pragma once
#include "SourceFile.h"
#include "Token.h"

typedef struct Lexer {
    c_string source;
    u32 source_size;
    Tokens tokens;
} Lexer;

i32 lexer_init(Lexer* lexer, SourceFile source_file);
void lexer_deinit(Lexer* lexer);
i32 lexer_run(Lexer* lexer);
