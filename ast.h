#ifndef AST_H
#define AST_H

#include "token.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// no interfaces, so.. everything is a node, separated by order
enum NodeType {
    // Expressions
    n_Identifier = 1,

    // Statements
    n_LetStatement,
    n_ReturnStatement,
    n_ExpressionStatement,
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

void node_print(const Node n, FILE* fp);

// free `n.obj`
void node_destroy(Node n);

// Ast Root Node, created by parser
typedef struct {
    Node* statements;
    size_t len;
    size_t cap;
} Program;

char* program_token_literal(const Program* p);
void program_destroy(Program* p);
void program_print(const Program* p, FILE* fp);

typedef struct {
    Token tok; // the 't_Ident' token
    char* value;
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

#endif
