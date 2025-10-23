#pragma once

#include "utils.h"
#include "token.h"
#include "hash-table/ht.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define NODE(t, p) (Node){ t, p }

// no interfaces, so.. everything is a node, separated by order
enum NodeType {
    // Expressions
    n_Identifier = 1,
    n_NullLiteral,
    n_IntegerLiteral,
    n_BooleanLiteral,
    n_FloatLiteral,
    n_FunctionLiteral,
    n_StringLiteral,
    n_ArrayLiteral,
    n_HashLiteral,
    n_PrefixExpression,
    n_InfixExpression,
    n_IfExpression,
    n_CallExpression,
    n_IndexExpression,
    n_AssignExpression,

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

// Retrieve token_literal of `n.obj.tok.literal`, [n.obj] must contain a
// `Token` as its first field
char* token_literal(Node n);

// Returns -1 on write to FILE err
int node_fprint(const Node n, FILE* fp);

// free `n.obj`
void node_free(Node n);

BUFFER(Node, Node);

typedef struct {
    NodeBuffer stmts;
} Program;

// Returns -1 on write to FILE err
int program_fprint(Program* p, FILE* fp);

typedef struct {
    Token tok; // the 't_Ident' token
} Identifier;

typedef struct {
    Token tok; // the 't_Let' token
    Identifier* name;
    Node value; // Expression
} LetStatement;

typedef struct {
    Token tok; // the 't_Assign' token
    Node left;
    Node right;
} AssignExpression;

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
    NodeBuffer stmts;
} BlockStatement;

void free_block_statement(BlockStatement* bs);
int fprint_block_statement(BlockStatement* bs, FILE* fp);

typedef struct {
    Token tok; // the 'if' token
    Node condition;
    BlockStatement* consequence;
    BlockStatement* alternative;
} IfExpression;

BUFFER(Param, Identifier*);

typedef struct {
    Token tok; // the 'fn' token
    ParamBuffer params;
    BlockStatement* body;
    // points to [LetStatement].name.tok.literal if in [LetStatement]
    char *name;
} FunctionLiteral;

void free_function_literal(FunctionLiteral* fl);

typedef struct {
    Token tok; // the '(' token
    Node function; // Identifier or FunctionLiteral
    NodeBuffer args;
} CallExpression;

typedef struct {
    Token tok; // the 't_String' token
} StringLiteral;

typedef struct {
    Token tok; // the '[' token
    NodeBuffer elements;
} ArrayLiteral;

typedef struct {
    Token tok; // the '[' token
    Node left;
    Node index;
} IndexExpression;

typedef struct {
    Node key;
    Node val;
} Pair;

BUFFER(Pair, Pair);

typedef struct {
    Token tok; // the '{' token
    PairBuffer pairs;
} HashLiteral;

void free_hash_literal(HashLiteral* hl);

typedef struct {
    Token tok; // the 'null' token
} NullLiteral;
