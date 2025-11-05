#pragma once

// This module contains definitions for constants created by the
// Compiler.

#include "ast.h"
#include "code.h"
#include "token.h"
#include "utils.h"

// Contains information needed for mapping a Node in the AST to a position in a
// Functions bytecode.  It is used for printing which line of the source code
// an error occured in the VM.
typedef struct {
    // the position in bytes in a Functions Instructions.
    int position;

    // The node in the AST with a Token which points to the source code.
    Node node;
} SourceMapping;

BUFFER(SourceMapping, SourceMapping)

// A Compiled FunctionLiteral.
typedef struct {
    Instructions instructions;
    // The number of local variables.
    int num_locals;
    // The number of parameters.
    int num_parameters;

    // Where in the source code the function was defined.
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
