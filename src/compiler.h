#pragma once

// This module contains the Compiler. It is a single-pass compiler which takes
// the AST created by the Parser and compiles it into Bytecode.

#include "constants.h"
#include "object.h"
#include "symbol_table.h"
#include "errors.h"

typedef struct {
    Opcode opcode;
    int position;
} EmittedInstruction;

// A CompilationScope for a function.
typedef struct {
    CompiledFunction *function;
    EmittedInstruction last_instruction;
    EmittedInstruction previous_instruction;
} CompilationScope;

BUFFER(Scope, CompilationScope)

typedef struct {
    // Constants in the AST: numbers, strings and functions.
    Constants constants;

    // The Symbol Table of the current CompilationScope.
    // A new SymbolTable is created for the each function encountered and
    // destroyed when its compilation is completed.
    SymbolTable *current_symbol_table;

    // Functions currently being compiled.
    ScopeBuffer scopes;
    int cur_scope_index; // index of current CompilationScope.
    CompilationScope *cur_scope;
    Instructions *current_instructions; // instructions of [cur_scope]
    SourceMappingBuffer *cur_mappings;  // [SourceMappings] of [cur_scope.function]

    // List of all Compiled functions.
    Buffer functions;
} Compiler;

void compiler_init(Compiler *c);
void compiler_free(Compiler *c);

// Compiles the Program (AST) and returns an Error if any.
// The Bytecode is retrieved using [bytecode()].
error compile(Compiler *c, Program *prog);

// make [Instruction] with Opcode and [int] operands into
// [c.current_instructions].
int emit(Compiler *c, Opcode op, ...);

// Enter new CompilationScope, creating now CompiledFunction and SymbolTable.
void enter_scope(Compiler *c);

// Return to previous CompilationScope and SymbolTable.
//
// This function returns the value of [c.current_symbol_table] before it was
// called.
SymbolTable *leave_scope(Compiler *c);

typedef struct {
    CompiledFunction *main_function;
    ConstantBuffer *constants;
    int num_globals; // [num_definitions] of global symbol table
} Bytecode;

// Retrieve the result of the Compilation. Must only be called after a
// successful compile().
Bytecode bytecode(Compiler *);

void fprint_compiler_instructions(FILE *stream, Compiler *c,
                                  bool print_mappings);
