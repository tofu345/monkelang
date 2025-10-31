#pragma once

#include "ast.h"
#include "code.h"
#include "token.h"
#include "utils.h"

// Contains information needed for later printing the source code of
// which statement an error occured in the VM.
typedef struct {
    // the position in bytes in a [CompiledFunction]s [Instructions].
    int position;

    // The statement in the AST with a Token which points to the source code.
    Node statement;
} SourceMapping;

BUFFER(SourceMapping, SourceMapping)

// A successfully compiled [FunctionLiteral].
typedef struct {
    Instructions instructions;
    int num_locals;
    int num_parameters;

    // where in the source code the function was defined.
    FunctionLiteral *literal;

    // [SourceMapping] for all statements in [literal.body].
    SourceMappingBuffer mappings;
} CompiledFunction;

void free_function(CompiledFunction *fn);

typedef enum {
    c_Integer = 1,
    c_Float,
    c_String,
    c_Function,
} ConstantType;

// TODO: remove duplicate constants
typedef struct {
    ConstantType type;
    union {
        long integer;
        double floating;
        Token *string;
        CompiledFunction *function;
    } data;
} Constant;

BUFFER(Constant, Constant)

// void free_constant(Constant c);
