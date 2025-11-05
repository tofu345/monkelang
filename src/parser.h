#pragma once

// The modules contains the Parser, it constructs and AST (Program) from the
// source code.

#include "ast.h"
#include "lexer.h"
#include "errors.h"

typedef struct Parser Parser;

typedef Node PrefixParseFn (Parser* p);
typedef Node InfixParseFn (Parser* p, Node left);

// All `parse_*` functions must return with `p->cur_token` in use or freed
struct Parser {
    Lexer l;
    Token cur_token;
    Token peek_token;

    ErrorBuffer errors;

    // TODO: change and reduce array size.

    // [t_Return] must remain the last [TokenType].
    // It cannot have a [PrefixParseFn] or [InfixParseFn].
    PrefixParseFn *prefix_parse_fns[t_Return];
    InfixParseFn *infix_parse_fns[t_Return];
};

void parser_init(Parser* p);
void parser_free(Parser* p);

// Create AST from [program]. Parses until an error is encountered and stored
// in [p.errors].
//
// [p.errors.length] is 0 on success.
Program parse(Parser* p, const char *program);
void program_free(Program* p);

enum Precedence {
    p_Lowest = 1,
    p_Equals,
    p_LessGreater,
    p_Sum,
    p_Product,
    p_Prefix,
    p_Call,
    p_Index,
};
