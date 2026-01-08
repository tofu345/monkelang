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
    n_NothingLiteral,
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
    n_RequireExpression,
    n_IndexExpression,

    // Statements
    n_LetStatement,
    n_Assignment,
    n_OperatorAssignment,
    n_ReturnStatement,
    n_LoopStatement,
    n_BreakStatement,
    n_ContinueStatement,
    n_ExpressionStatement,
    n_BlockStatement,
};

typedef struct {
    enum NodeType typ;
    void* obj; // if NULL, err
} Node;

// [n.obj] must contain a `Token` as its first field
static inline Token *node_token(Node n) {
    return (Token *) n.obj;
}

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

void program_free(Program* p);

// An identifier (variable name)
typedef struct {
    Token tok; // the 't_Ident' token
} Identifier;

typedef struct {
    Token tok; // the 't_Let' token
    Buffer names; // Identifiers
    NodeBuffer values; // Expression
} LetStatement;

// e.g. a = 1
// NOTE: [left], must already be defined.
typedef struct {
    Token tok; // the token of [right]
    Node left; // Identifier or IndexExpression
    Node right;
} Assignment;

// e.g. a += 1
// NOTE: [left], must already be defined.
typedef struct {
    Token op;
    Node left;
    Node right;
} OperatorAssignment;

// Terminate execution of current Function.
typedef struct {
    Token tok; // the 't_Return' token
    Node return_value; // expression
} ReturnStatement;

// The only statement that leaves a value on the stack.
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
    Token op; // e.g '!'
    Node right;
} PrefixExpression;

// e.g. 1 * 3
typedef struct {
    Token op; // e.g. '+', '-' ...
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

typedef struct {
    Token tok;

    // optional initialization statement
    Node start;

    // expression, assumed `true` if not present.
    Node condition;

    // optional update statement
    Node update;

    BlockStatement* body;
} LoopStatement;

void free_loop_statement(LoopStatement *fs);

typedef struct {
    Token tok; // the 'if' token
    Node condition;
    BlockStatement* consequence;
    BlockStatement* alternative;
} IfExpression;

void free_if_expression(IfExpression* ie);

typedef struct {
    Token tok; // the 'fn' token
    Buffer params;
    BlockStatement* body;

    // if [FunctionLiteral] defined in [LetStatement] or [Assignment],
    // points to corresponding Identifier.
    Identifier *name;
} FunctionLiteral;

void free_function_literal(FunctionLiteral* fl);

typedef struct {
    Token tok; // token of [function]
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
    Token tok; // the 't_Nothing' token
} NothingLiteral;

typedef struct {
    Token tok; // 'require(...)'
    StringLiteral *filename;
    NodeBuffer args;
} RequireExpression;

typedef struct {
    Token tok;
} BreakStatement;

typedef struct {
    Token tok;
} ContinueStatement;
