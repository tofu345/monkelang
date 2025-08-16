#ifndef AST_H
#define AST_H

#include "token.h"

#include <stdbool.h>
#include <stddef.h>

// no interfaces, so.. everything is a node, separated by order
enum NodeType {
    // Expressions
    n_Identifier = 1,
    // Statements
    n_LetStatement
};

typedef struct {
    enum NodeType typ;
    void* obj; // if NULL, err
} Node;

bool is_expression(Node* n);
bool is_statement(Node* n);

// Retrieve token_literal of `obj.tok.literal` the object must contain a
// `Token` as its first field
char* token_literal(Node* n);

// free node `n` object, `n.obj` and `n.tok.literal`
void node_destroy(Node* n);

// Ast Root Node, created by parser
typedef struct {
    Node* statements;
    size_t len;
    size_t cap;
} Program;

char* program_token_literal(Program* p);
void program_destroy(Program* p);

typedef struct {
    Token tok;
    char* value;
} Identifier;

typedef struct {
    Token tok;
    Identifier* name;
    Node value; // Expression
} LetStatement;

#endif
