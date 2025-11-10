#pragma once

// This module contains the Symbol Table.

#include "hash-table/ht.h"
#include "utils.h"

#include <stdint.h>

typedef enum {
    // Global variables.
    GlobalScope,

    // Local variables.
    LocalScope,

    // This scope contains only the name of the function currently being
    // compiled.
    FunctionScope,

    // Variables defined in parent functions.
    FreeScope,

    // Builtin functions.
    BuiltinScope,
} SymbolScope;

typedef struct {
    // NOTE: [name] is not copied.
    const char *name;

    SymbolScope scope;

    // The index into a functions local variables or the global variable list.
    int index;
} Symbol;

BUFFER(Symbol, Symbol *)

typedef struct SymbolTable {
    struct SymbolTable *outer;

    ht *store;
    int num_definitions;

    SymbolBuffer free_symbols;
} SymbolTable;

SymbolTable *symbol_table_new();
SymbolTable *enclosed_symbol_table(SymbolTable *outer);

void symbol_table_free(SymbolTable *st);

Symbol *sym_define(SymbolTable *st, const char *name, uint64_t hash);

Symbol *sym_function_name(SymbolTable *st, const char *name, uint64_t hash);
Symbol *sym_builtin(SymbolTable *st, int index, const char *name);

Symbol *sym_resolve(SymbolTable *st, uint64_t hash);
