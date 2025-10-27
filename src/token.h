#pragma once

#include <stddef.h>

typedef enum __attribute__ ((__packed__)) {
    t_Illegal,
    t_Eof,

    t_Ident, // identifiers (variable names)

    // literals
    t_String,
    t_Digit, // integer and floating point numbers.

    // Operators
    t_Assign,
    t_Plus,
    t_Minus,
    t_Bang,
    t_Asterisk,
    t_Slash,
    t_Lt,
    t_Gt,
    t_Eq,
    t_Not_eq,

    // Delimeters
    t_Colon,
    t_Comma,
    t_Semicolon,
    t_Lparen,
    t_Rparen,
    t_Lbrace,
    t_Rbrace,
    t_Lbracket,
    t_Rbracket,

    // Keywords
    t_Function,
    t_Let,
    t_True,
    t_False,
    t_Null,
    t_If,
    t_Else,
    t_Return,
} TokenType;

typedef struct {
    TokenType type;

    int line; // 1-based line number

    // from wren: points to the beginning of the [Token] in the source code.
    const char *start;

    // the number of characters of the source code the [Token] represents.
    int length;
} Token;

#define LITERAL(t) t.length, t.start

TokenType lookup_ident(const char* ident, int ident_len);

const char* show_token_type(TokenType t);
