#pragma once

// This module contains definitions for constants created by the Compiler.

#include "ast.h"
#include "code.h"
#include "utils.h"

// A Compiled FunctionLiteral.
typedef struct {
    Instructions instructions;
    int num_locals;
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
