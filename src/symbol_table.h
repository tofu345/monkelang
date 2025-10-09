#pragma once

#include "hash-table/ht.h"

typedef enum {
    GlobalScope
} SymbolScope;

typedef struct {
    char *name;
    SymbolScope scope;
    int index;
} Symbol;

typedef struct {
    ht *store; // num_definitions in [store.length]
} SymbolTable;

void symbol_table_init(SymbolTable *st);
void symbol_table_free(SymbolTable *st);

Symbol *sym_define(SymbolTable *st, char *name);
Symbol *sym_resolve(SymbolTable *st, char *name);
