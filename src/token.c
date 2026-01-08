#include "token.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Keyword {
    const char* name;
    int length;
    TokenType tok_typ;
} const keywords[] = {
    {"if", 2, t_If},
    {"fn", 2, t_Function},
    {"for", 3, t_For},
    {"let", 3, t_Let},
    {"true", 4, t_True},
    {"else", 4, t_Else},
    {"false", 5, t_False},
    {"break", 5, t_Break},
    {"return", 6, t_Return},
    {"require", 7, t_Require},
    {"nothing", 7, t_Nothing},
    {"continue", 8, t_Continue},
};
int num_keywords = sizeof(keywords) / sizeof(keywords[0]);

TokenType lookup_ident(const char* ident, int ident_len) {
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
    "nothing",
    "true",
    "false",
    "String",
    "Integer",
    "Float",

    "+",
    "-",
    "*",
    "/",
    "!",

    "=",
    "+=",
    "-=",
    "*=",
    "/=",

    "==",
    "!=",
    "<",
    ">",

    ":",
    ",",
    ";",
    "(",
    ")",
    "{",
    "}",
    "[",
    "]",

    "for",
    "break",
    "continue",
    "fn",
    "let",
    "if",
    "else",
    "require",
    "return",
};
const int len = sizeof(token_types) / sizeof(token_types[0]);

const char* show_token_type(TokenType t) {
    if (t >= len) {
        fprintf(stderr, "show_token_type: invalid token_type %d", t);
        exit(1);
    }
    return token_types[t];
}
