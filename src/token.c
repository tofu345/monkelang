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

// TODO: replace with hashmap
TokenType lookup_ident(const char* ident, int ident_len) {
    static size_t num_keywords = sizeof(keywords) / sizeof(keywords[0]);
    char *keyword;
    int len;
    for (size_t i = 0; i < num_keywords; i++) {
        keyword = keywords[i].name;
        len = strlen(keyword);
        if (len == ident_len && strncmp(keyword, ident, len) == 0) {
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
    "Digit",
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
