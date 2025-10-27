#pragma once

#include "constants.h"
#include "parser.h"
#include "code.h"
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

BUFFER(Scope, CompilationScope)
BUFFER(SymbolTable, SymbolTable *)

typedef struct {
    ConstantBuffer constants;

    SymbolTable *current_symbol_table;
    SymbolTableBuffer symbol_tables;

    ScopeBuffer scopes;
    int scope_index; // index of current [CompilationScope]
    // instructions of current [CompilationScope]
    Instructions *current_instructions;
} Compiler;

void compiler_init(Compiler *c);
void compiler_free(Compiler *c);

Error *compile(Compiler *c, Program *prog);

// append [Instruction] with [Opcode] and [int] operands
int emit(Compiler *c, Opcode op, ...);

void enter_scope(Compiler *c);
Instructions *leave_scope(Compiler *c);

typedef struct {
    Instructions *instructions;
    ConstantBuffer *constants;
    int num_globals;
} Bytecode;

Bytecode bytecode(Compiler *c);
