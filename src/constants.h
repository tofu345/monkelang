#pragma once

// This module contains definitions for constants created by the Compiler.

#include "ast.h"
#include "code.h"
#include "object.h"
#include "table.h"
#include "utils.h"

typedef enum {
    c_Integer = 1,
    c_Float,
    c_String,
    c_Function,
} ConstantType;

typedef union {
    long integer;
    double floating;
    Token *string;
    CompiledFunction *function;
} ConstantData;

typedef struct {
    ConstantType type;
    ConstantData data;
} Constant;

BUFFER(Constant, Constant)
