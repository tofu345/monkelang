#pragma once

#include "parser.h"
#include "code.h"
#include "object.h"
#include "symbol_table.h"

typedef enum {
    c_Integer = 1,
    c_Float,
    c_String
} ConstantType;

typedef struct {
    ConstantType type;
    union {
        long integer;
        double floating;
        char *string;
    } data;
} Constant;

BUFFER(Constant, Constant);

typedef struct {
    Opcode opcode;
    int position;
} EmittedInstruction;

typedef struct {
    Instructions instructions;
    ConstantBuffer constants;

    EmittedInstruction last_instruction;
    EmittedInstruction previous_instruction;

    SymbolTable symbol_table;

    ErrorBuffer errors;
} Compiler;

void compiler_init(Compiler *c);
void compiler_with(Compiler *c, SymbolTable *symbol_table,
        ConstantBuffer constants);

void compiler_free(Compiler *c);

// returns 0 on success.
// otherwise non-0 with errors in [c.errors].
int compile(Compiler *c, Program *prog);

typedef struct {
    Instructions instructions;
    ConstantBuffer constants;
} Bytecode;

Bytecode *bytecode(Compiler *c);
