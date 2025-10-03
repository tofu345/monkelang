#pragma once

#include "ast.h"
#include "code.h"
#include "object.h"
#include "parser.h"

// TODO FIXME: Change ObjectBuffer to hold concrete [Objects]
BUFFER(Constant, Object);

typedef struct {
    Instructions instructions;
    ConstantBuffer constants;

    StringBuffer errors;
} Compiler;

// Init with `Compiler c = {}`.
// void compiler_init(Compiler *c);

// free [compiler.errors]
void compiler_destroy(Compiler *c);

// returns non-0 on err
int compile(Compiler *c, Program *prog);

typedef struct {
    Instructions instructions;
    ConstantBuffer constants;
} Bytecode;

Bytecode *bytecode(Compiler *c);
