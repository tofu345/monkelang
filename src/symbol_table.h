#pragma once

#include "hash-table/ht.h"

typedef enum {
    GlobalScope,
    LocalScope,
    BuiltinScope,
} SymbolScope;

typedef struct {
    char *name;
    SymbolScope scope;
    int index;
} Symbol;

typedef struct SymbolTable {
    struct SymbolTable *outer;

    ht *store;
    int num_definitions;
} SymbolTable;

void symbol_table_init(SymbolTable *st);
void symbol_table_free(SymbolTable *st);

void enclosed_symbol_table(SymbolTable *st, SymbolTable *outer);

// copy [name] and define [Symbol]
Symbol *sym_define(SymbolTable *st, char *name);
Symbol *sym_resolve(SymbolTable *st, char *name);

Symbol *define_builtin(SymbolTable *st, int index, const char *name);
