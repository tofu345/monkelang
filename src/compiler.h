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
    CompiledFunction *function;
    EmittedInstruction last_instruction;
    EmittedInstruction previous_instruction;
} CompilationScope;

BUFFER(Scope, CompilationScope)
BUFFER(SymbolTable, SymbolTable *)
BUFFER(Function, CompiledFunction *)

typedef struct {
    ConstantBuffer constants;

    SymbolTable *current_symbol_table;
    // TODO: cleanup [SymbolTables] when [leave_scope()].
    SymbolTableBuffer symbol_tables;

    // all functions currently being compiled.
    ScopeBuffer scopes;
    CompilationScope *cur_scope;
    int cur_scope_index; // index of [cur_scope]
    Instructions *current_instructions; // instructions of [cur_scope]

    // compiled functions.
    FunctionBuffer functions;
} Compiler;

void compiler_init(Compiler *c);
void compiler_free(Compiler *c);

Error *compile(Compiler *c, Program *prog);

// append [Instruction] with [Opcode] and [int] operands
int emit(Compiler *c, Opcode op, ...);

void enter_scope(Compiler *c);
void leave_scope(Compiler *c);

typedef struct {
    CompiledFunction *main_function;
    ConstantBuffer *constants;
    int num_globals; // [num_definitions] of global symbol table
} Bytecode;

Bytecode bytecode(Compiler *c);
