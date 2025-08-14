enum TokenType {
    Illegal,
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
};

typedef struct {
    enum TokenType type;
    char *literal;
} Token;
