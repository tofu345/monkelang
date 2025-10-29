#pragma once

#include "constants.h"
#include "object.h"
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
BUFFER(Function, CompiledFunction)

typedef struct {
    ConstantBuffer constants;

    SymbolTable *current_symbol_table;
    SymbolTableBuffer symbol_tables;

    // functions currently being compiled.
    ScopeBuffer scopes;
    // index of current [CompilationScope]
    int scope_index;
    // instructions of current [CompilationScope]
    Instructions *current_instructions;

    // compiled functions.
    FunctionBuffer functions;
} Compiler;

void compiler_init(Compiler *c);
void compiler_free(Compiler *c);

Error *compile(Compiler *c, Program *prog);

// append [Instruction] with [Opcode] and [int] operands
int emit(Compiler *c, Opcode op, ...);

void enter_scope(Compiler *c);
Instructions *leave_scope(Compiler *c);

typedef struct {
    FunctionBuffer *functions;
    ConstantBuffer *constants;
    int num_globals; // [num_definitions] of global symbol table
} Bytecode;

Bytecode bytecode(Compiler *c);
