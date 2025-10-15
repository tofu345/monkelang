#pragma once

#include "code.h"
#include "utils.h"

typedef struct {
    char *data;
    int length;
} StringConstant;

typedef struct {
    Instructions instructions;
    int num_locals;
    int num_parameters;
} CompiledFunction;

typedef enum {
    c_Integer = 1,
    c_Float,

    c_String, // points to literal in AST.
    c_Instructions,
} ConstantType;

// TODO: remove duplicate constants
typedef struct {
    ConstantType type;
    union {
        long integer;
        double floating;
        StringConstant *string;
        CompiledFunction *function;
    } data;
} Constant;

BUFFER(Constant, Constant);

void free_constant(Constant c);
