#include "token.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

struct Keyword {
    char* name;
    TokenType tok_typ;
} keywords[] = {
    {"fn", t_Function},
    {"let", t_Let},
    {"true", t_True},
    {"false", t_False},
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
char* token_types[] = {
    "Illegal",
    "Eof",
    "Ident",
	"Int",
	"Assign",
	"Plus",
	"Minus",
	"Bang",
	"Asterisk",
	"Slash",
	"Lt",
	"Gt",
	"Eq",
	"Not_eq",
	"Comma",
	"Semicolon",
	"Lparen",
	"Rparen",
	"Lbrace",
	"Rbrace",
	"Function",
	"Let",
	"True",
	"False",
	"If",
	"Else",
	"Return",
};

char* show_token_type(TokenType t) {
    return token_types[t - 1];
}
