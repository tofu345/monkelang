#pragma once

#include "ast.h"
#include "code.h"
#include "token.h"
#include "utils.h"

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

        // not a pointer because [c.functions] calls realloc().
        // which moved data, invalidating old pointers.
        int function_index;
    } data;
} Constant;

BUFFER(Constant, Constant)

// void free_constant(Constant c);
