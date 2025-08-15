#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    Illegal = 1,
    Eof,
    // identifiers + literals
    Ident,
	Int,
    // Operators
	Assign,
	Plus,
	Minus,
	Bang,
	Asterisk,
	Slash,
	Lt,
	Gt,
	Eq,
	Not_eq,
    // Delimeters
	Comma,
	Semicolon,
	Lparen,
	Rparen,
	Lbrace,
	Rbrace,
    // Keywords
	Function,
	Let,
	True,
	False,
	If,
	Else,
	Return,
} TokenType;

typedef struct {
    TokenType type;
    char *literal; // a malloced string
} Token;

TokenType lookup_ident(char* ident);

char* show_token_type(TokenType t);

#endif
