#pragma once

#include "hash-table/ht.h"
#include "utils.h"

typedef enum {
    GlobalScope,

    // variables in current function
    LocalScope,

    // contains only the name of the function currently being compiled
    FunctionScope,

    // variables in parent function(s)
    FreeScope,

    // builtin functions
    BuiltinScope,
} SymbolScope;

typedef struct {
    char *name;
    SymbolScope scope;
    int index;
} Symbol;

BUFFER(Symbol, Symbol *);

typedef struct SymbolTable {
    struct SymbolTable *outer;

    ht *store;
    int num_definitions;

    SymbolBuffer free_symbols;
} SymbolTable;

void symbol_table_init(SymbolTable *st);
void enclosed_symbol_table(SymbolTable *st, SymbolTable *outer);

void symbol_table_free(SymbolTable *st);

// copy [name] and define [Symbol]
Symbol *sym_define(SymbolTable *st, char *name);

Symbol *sym_function_name(SymbolTable *st, char *name);
Symbol *sym_builtin(SymbolTable *st, int index, const char *name);

Symbol *sym_resolve(SymbolTable *st, char *name);
