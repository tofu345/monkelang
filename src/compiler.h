#pragma once

#include "constants.h"
#include "parser.h"
#include "code.h"
#include "object.h"
#include "symbol_table.h"

typedef struct {
    Opcode opcode;
    int position;
} EmittedInstruction;

typedef struct {
    Instructions instructions;
    EmittedInstruction last_instruction;
    EmittedInstruction previous_instruction;
} CompilationScope;

BUFFER(Scope, CompilationScope);

typedef struct {
    SymbolTable *data;
    int length, capacity;
} SymbolTables;

typedef struct {
    ConstantBuffer constants;

    SymbolTable *cur_symbol_table;
    SymbolTables symbol_tables;

    Instructions *current_instructions;
    ScopeBuffer scopes;
    int scope_index;
} Compiler;

void compiler_init(Compiler *c);
void compiler_free(Compiler *c);

error compile(Compiler *c, Program *prog);

// append [Instruction] with [Opcode] and [int] operands
int emit(Compiler *c, Opcode op, ...);

void enter_scope(Compiler *c);
Instructions *leave_scope(Compiler *c);

typedef struct {
    Instructions *instructions;
    ConstantBuffer *constants;
} Bytecode;

Bytecode bytecode(Compiler *c);
