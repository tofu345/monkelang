#pragma once

typedef enum __attribute__ ((__packed__)) {
    t_Illegal,
    t_Eof,
    // literals
    t_Ident, // variable names
    t_String,
    t_Int,   // stored as `long`
    t_Float, // stored as `double`
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
    t_If,
    t_Else,
    t_Return, // this must remain the last enum element, and cannot have a
              // `PrefixParseFn` associated with it, see parser.h
} TokenType;

typedef struct {
    TokenType type;
    short col;
    int line;
    char *literal; // a malloced string
} Token;

TokenType lookup_ident(const char* ident);

const char* show_token_type(TokenType t);
