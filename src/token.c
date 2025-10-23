#include "token.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Keyword {
    char* name;
    TokenType tok_typ;
} const keywords[] = {
    {"fn", t_Function},
    {"let", t_Let},
    {"true", t_True},
    {"false", t_False},
    {"null", t_Null},
    {"if", t_If},
    {"else", t_Else},
    {"return", t_Return},
};

TokenType lookup_ident(const char* ident) {
    size_t len = sizeof(keywords) / sizeof(keywords[0]);
    for (size_t i = 0; i < len; i++) {
        if (strcmp(keywords[i].name, ident) == 0) {
            return keywords[i].tok_typ;
        }
    }
    return t_Ident;
}

// for printing
const char* token_types[] = {
    "Illegal",
    "Eof",
    "Identifier",
    "String",
    "Int",
    "Float",
    "=",
    "+",
    "-",
    "!",
    "*",
    "/",
    "<",
    ">",
    "=",
    "!=",
    ":",
    ",",
    ";",
    "(",
    ")",
    "{",
    "}",
    "[",
    "]",
    "fn",
    "let",
    "true",
    "false",
    "null",
    "if",
    "else",
    "return",
};

const char* show_token_type(TokenType t) {
    static size_t len = sizeof(token_types) / sizeof(token_types[0]);
    if (t >= len) {
        fprintf(stderr, "show_token_type: invalid token_type %d", t);
        exit(1);
    }
    return token_types[t];
}
