#pragma once

// This module contains the Symbol Table.

#include "hash-table/ht.h"
#include "token.h"
#include "utils.h"

#include <stdint.h>

typedef enum {
    // Global variables.
    GlobalScope,

    // Local variables.
    LocalScope,

    // This scope contains only the name of the `CompiledFunction` the
    // `SymbolTable` belongs to.
    FunctionScope,

    // Variables defined in parent functions.
    FreeScope,
} SymbolScope;

typedef struct {
    // `const char *` if [BuiltinScope] and `Token *` otherwise
    void *name;

    SymbolScope scope;

    // The index into the local or global variable list.
    int index;
} Symbol;

typedef struct SymbolTable {
    struct SymbolTable *outer;

    ht *store;
    int num_definitions;

    Buffer free_symbols;
} SymbolTable;

SymbolTable *symbol_table_new();
SymbolTable *enclosed_symbol_table(SymbolTable *outer);

void symbol_table_free(SymbolTable *st);

Symbol *sym_define(SymbolTable *st, Token *name, uint64_t hash);

Symbol *sym_function_name(SymbolTable *st, Token *name, uint64_t hash);

Symbol *sym_resolve(SymbolTable *st, uint64_t hash);
