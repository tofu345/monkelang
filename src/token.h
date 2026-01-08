#pragma once

// The module contains the definition for Token.

#include <stddef.h>

typedef enum __attribute__ ((__packed__)) {
    t_Illegal,
    t_Eof,

    t_Ident, // identifiers (variable names)

    // literals
    t_Nothing,
    t_True,
    t_False,
    t_String,
    t_Integer,
    t_Float,

    // Operators
    t_Plus,
    t_Minus,
    t_Asterisk,
    t_Slash,
    t_Bang,

    // Assignment Operators
    t_Assign,
    t_Add_Assign,
    t_Sub_Assign,
    t_Mul_Assign,
    t_Div_Assign,

    // Comparison Operators
    t_Eq,
    t_Not_eq,
    t_Lt,
    t_Gt,

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
    t_For,
    t_Break,
    t_Continue,
    t_Function,
    t_Let,
    t_If,
    t_Else,
    t_Require,
    t_Return,
} TokenType;

typedef struct {
    TokenType type;

    int line; // 1-based line number

    // from wren: points to the beginning of the [Token] in the source code.
    const char *start;

    // the number of characters of the source code the [Token] represents.
    int length;

    // the index of [start] in the source code.
    int position;
} Token;

// for printf("%.*s") of Tokens
#define LITERAL(t) t.length, t.start

TokenType lookup_ident(const char* ident, int ident_len);

const char* show_token_type(TokenType t);
