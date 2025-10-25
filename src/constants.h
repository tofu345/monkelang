#pragma once

#include "ast.h"
#include "code.h"
#include "token.h"
#include "utils.h"

typedef struct {
    Instructions instructions;
    int num_locals;
    int num_parameters;
    Identifier *name; // points to [FunctionLiteral.name]
} Function;

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
        Function *function;
    } data;
} Constant;

BUFFER(Constant, Constant)

void free_constant(Constant c);

void free_function(Constant c);
