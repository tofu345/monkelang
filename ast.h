#ifndef AST_H
#define AST_H

#include "utils.h"
#include "token.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// no interfaces, so.. everything is a node, separated by order
enum NodeType {
    // Expressions
    n_Identifier = 1,
    n_IntegerLiteral,
    n_FloatLiteral,
    n_PrefixExpression,
    n_InfixExpression,
    n_BooleanLiteral,
    n_IfExpression,
    n_FunctionLiteral,
    n_CallExpression,

    // Statements
    n_LetStatement,
    n_ReturnStatement,
    n_ExpressionStatement,
    n_BlockStatement,
};

typedef struct {
    enum NodeType typ;
    void* obj; // if NULL, err
} Node;

bool is_expression(Node n);
bool is_statement(Node n);

// Retrieve token_literal of `obj.tok.literal` the object must contain a
// `Token` as its first field
char* token_literal(Node n);

// Returns -1 on write to FILE err
int node_fprint(const Node n, FILE* fp);

// free `n.obj`
void node_destroy(Node n);

// Ast Root Node, created by parser
typedef struct {
    Node* stmts;
    size_t len;
    size_t cap;
} Program;

char* program_token_literal(const Program* p);

// Returns -1 on write to FILE err
// TODO: return index of statement where fprint failed on to resume later
int program_fprint(const Program* p, FILE* fp);

typedef struct {
    Token tok; // the 't_Ident' token
    char* value; // same as tok.literal
} Identifier;

typedef struct {
    Token tok; // the 't_Let' token
    Identifier* name;
    Node value; // Expression
} LetStatement;

typedef struct {
    Token tok; // the 't_Return' token
    Node return_value; // expression
} ReturnStatement;

typedef struct {
    Token tok; // the first token of the expression
    Node expression;
} ExpressionStatement;

typedef struct {
    Token tok;
    long value;
} IntegerLiteral;

typedef struct {
    Token tok;
    double value;
} FloatLiteral;

typedef struct {
    Token tok; // the prefix token, e.g !
    char* op; // same as tok.literal
    Node right;
} PrefixExpression;

typedef struct {
    Token tok; // the prefix token, e.g *
    Node left;
    char* op; // same as tok.literal
    Node right;
} InfixExpression;

typedef struct {
    Token tok;
    bool value;
} BooleanLiteral;

typedef struct {
    Token tok; // the '{' token
    Node* statements;
    size_t len;
    size_t cap;
} BlockStatement;

typedef struct {
    Token tok; // the 'if' token
    Node condition;
    BlockStatement* consequence;
    BlockStatement* alternative;
} IfExpression;

typedef struct {
    Token tok; // the 'fn' token
    Identifier** params;
    size_t params_len;
    size_t params_cap;
    BlockStatement* body;
} FunctionLiteral;

typedef struct {
    Token tok; // the '(' token
    Node function; // Identifier or FunctionLiteral
    Node* args;
    size_t args_len;
    size_t args_cap;
} CallExpression;

#endif
