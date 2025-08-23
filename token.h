#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    t_Illegal,
    t_Eof,
    // identifiers + literals
    t_Ident,
	t_Int,
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
	t_Comma,
	t_Semicolon,
	t_Lparen,
	t_Rparen,
	t_Lbrace,
	t_Rbrace,
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
    char *literal; // a malloced string
} Token;

TokenType lookup_ident(const char* ident);

const char* show_token_type(TokenType t);

#endif
