#pragma once
#include <Types.h>

typedef enum TokenType : u8 {
    TokenType_Identifier,
} TokenType;

typedef struct Token {
    u32 start_index;
    u16 size;
    TokenType type;
} Token;

typedef struct Tokens {
    Token* data;
    u32 size;
    u32 capacity;
} Tokens;

i32 tokens_init(Tokens* tokens);
void tokens_deinit(Tokens* tokens);
