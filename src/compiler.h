#pragma once

// This module contains the Compiler. It is a single-pass compiler which takes
// the AST created by the Parser and compiles it into Bytecode.

#include "code.h"
#include "constants.h"
#include "errors.h"
#include "object.h"
#include "parser.h"
#include "symbol_table.h"

BUFFER(Scope, struct CompilationScope)

typedef struct {
    // Queue of functions under compilation.
    // After compilation, Functions are moved into [constants].
    ScopeBuffer scopes;
    // current `CompilationScope`, always last member of [scopes].
    struct CompilationScope *cur_scope;
    Instructions *cur_instructions;     // [cur_scope.function.instructions]
    SourceMappingBuffer *cur_mappings;  // [cur_scope.function.mappings]

    // The Symbol Table of the current CompilationScope.
    SymbolTable *cur_symbol_table;

    // Contains constants in the AST: numbers, strings and functions.
    // Shared amongst all compilations.
    ConstantBuffer constants;
    Table constants_table; // Used to check for duplicate constants
} Compiler;

// initialize constants table, is not necessary to be called.
void compiler_init(Compiler *);

void compiler_free(Compiler *);

// Compile `Program` from statement [from], returning `errors` if any.
// The Bytecode is retrieved using `bytecode()`.
error compile(Compiler *, Program *, int from);

// discard all non-global `CompilationScopes` and `SymbolTables`
void compiler_reset(Compiler *);

// make_into(c.cur_instructions, Opcode, `int` operands ...)
int emit(Compiler *, Opcode, ...);

typedef struct {
    Opcode opcode;
    int position;
} EmittedInstruction;

// A Compilation of a Function.
typedef struct CompilationScope {
    CompiledFunction *function;

    EmittedInstruction last_instruction;
    EmittedInstruction previous_instruction;
} CompilationScope;

// Enter new CompilationScope, creating new CompiledFunction and SymbolTable.
void enter_scope(Compiler *);

// Return to previous CompilationScope and SymbolTable.
void leave_scope(Compiler *);

typedef struct {
    // Entry point.
    CompiledFunction *main_function;

    // Number of global variables in [main_function].
    int num_globals;
} Bytecode;

// Retrieve the result of `compile(..)`.
Bytecode bytecode(Compiler *c);
