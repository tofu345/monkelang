#pragma once

#include "hash-table/ht.h"
#include "utils.h"

#include <stdint.h>

typedef enum {
    GlobalScope,

    // variables in current function
    LocalScope,

    // contains only the name of the function currently being compiled
    FunctionScope,

    // variables in parent function(s).
    // free variable are shallow copied into child functions.
    FreeScope,

    // builtin functions
    BuiltinScope,
} SymbolScope;

typedef struct {
    const char *name;
    SymbolScope scope;
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

// copy [name] and define [Symbol]
Symbol *sym_define(SymbolTable *st, const char *name, uint64_t hash);

Symbol *sym_function_name(SymbolTable *st, const char *name, uint64_t hash);
Symbol *sym_builtin(SymbolTable *st, int index, const char *name);

Symbol *sym_resolve(SymbolTable *st, uint64_t hash);
