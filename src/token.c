#include "token.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Keyword {
    char* name;
    int length;
    TokenType tok_typ;
} const keywords[] = {
    {"fn", 2, t_Function},
    {"let", 3, t_Let},
    {"true", 4, t_True},
    {"false", 5, t_False},
    {"null", 4, t_Null},
    {"if", 2, t_If},
    {"else", 4, t_Else},
    {"return", 6, t_Return},
};

TokenType lookup_ident(const char* ident, int ident_len) {
    static int num_keywords = sizeof(keywords) / sizeof(keywords[0]);
    int i, len;
    for (i = 0; i < num_keywords; i++) {
        len = keywords[i].length;
        if (len == ident_len && strncmp(keywords[i].name, ident, len) == 0) {
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
    "null",
    "true",
    "false",
    "String",
    "Integer",
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
