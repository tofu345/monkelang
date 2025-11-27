#pragma once

// This module contains the definitions for the AST produced by the Parser.

#include "utils.h"
#include "token.h"
#include "hash-table/ht.h"

#include <stdbool.h>

#define NODE(t, p) (Node){ t, p }

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
    n_TableLiteral,
    n_PrefixExpression,
    n_InfixExpression,
    n_IfExpression,
    n_CallExpression,
    n_IndexExpression,

    // Statements
    n_LetStatement,
    n_Assignment,
    n_OperatorAssignment,
    n_ReturnStatement,
    n_ForStatement,
    n_ExpressionStatement,
    n_BlockStatement,
};

typedef struct {
    enum NodeType typ;
    void* obj; // if NULL, err
} Node;

// [n.obj] must contain a `Token` as its first field
Token *node_token(Node n);

// Returns -1 on write to FILE err
int node_fprint(const Node n, FILE* fp);

// free [n.obj] if [n.obj] not NULL.
void node_free(Node n);

BUFFER(Node, Node)

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

// e.g. a = 1
typedef struct {
    Token tok; // the token of [right]
    Node left; // Identifier or IndexExpression
    Node right;
} Assignment;

// e.g. a += 1
typedef struct {
    Token tok; // the operator
    Node left;
    Node right;
} OperatorAssignment;

typedef struct {
    Token tok; // the 't_Return' token
    Node return_value; // expression
} ReturnStatement;

typedef struct {
    Token tok; // the token of [expression]
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

// e.g. !true
typedef struct {
    Token tok; // the prefix token, e.g '!'
    Node right;
} PrefixExpression;

// e.g. 1 * 3
typedef struct {
    Token tok; // the infix token, e.g. '+'
    Node left;
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

// for (loop) statement
typedef struct {
    Token tok; // the 'for' token
    Node init_statement;
    Node condition; // expression
    Node update_statement;
    BlockStatement* body;
} ForStatement;

void free_for_statement(ForStatement *fs);

typedef struct {
    Token tok; // the 'if' token
    Node condition;
    BlockStatement* consequence;
    BlockStatement* alternative;
} IfExpression;

BUFFER(Param, Identifier*)

typedef struct {
    Token tok; // the 'fn' token
    ParamBuffer params;
    BlockStatement* body;

    // if [FunctionLiteral] is in [LetStatement] or [AssignStatement],
    // points to corresponding Identifier.
    Identifier *name;
} FunctionLiteral;

void free_function_literal(FunctionLiteral* fl);

typedef struct {
    Token tok; // the 't_Ident' token
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

BUFFER(Pair, Pair)

typedef struct {
    Token tok; // the '{' token
    PairBuffer pairs;
} TableLiteral;

void free_table_literal(TableLiteral* hl);

typedef struct {
    Token tok; // the 'null' token
} NullLiteral;
