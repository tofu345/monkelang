#include "symbol_table.h"
#include "hash-table/ht.h"
#include "utils.h"

#include <stdbool.h>

void symbol_table_init(SymbolTable *st) {
    st->num_definitions = 0;
    st->outer = NULL;
    st->store = ht_create();
    if (st->store == NULL) { die("symbol_table_init"); }
}

void enclosed_symbol_table(SymbolTable *st, SymbolTable *outer) {
    st->num_definitions = 0;
    st->outer = outer;
    st->store = ht_create();
    if (st->store == NULL) { die("enclosed_symbol_table"); }
}

void symbol_table_free(SymbolTable *st) {
    hti it = ht_iterator(st->store);
    Symbol *cur;
    while (ht_next(&it)) {
        cur = it.current->value;
        free(cur->name);
        free(cur);
    }
    ht_destroy(st->store);
}

static Symbol *
new_symbol(SymbolTable *st, char *name, int index, SymbolScope scope) {
    Symbol *symbol = malloc(sizeof(Symbol));
    if (symbol == NULL) { die("new_symbol: malloc"); }

    void *ptr = ht_set(st->store, name, symbol);
    if (ptr == NULL) {
        die("new_symbol: ht_set");

    // previous symbol with same [name]
    } else if (ptr != symbol) {
        memcpy(symbol, ptr, sizeof(Symbol));
        free(ptr);
        return symbol;
    }

    name = strdup(name);
    if (name == NULL) { die("new_symbol: strdup"); }

    *symbol = (Symbol) {
        .name = name,
        .index = index,
        .scope = scope,
    };

    return symbol;
}

Symbol *sym_define(SymbolTable *st, char *name) {
    SymbolScope scope;
    if (st->outer == NULL) {
        scope = GlobalScope;
    } else {
        scope = LocalScope;
    }

    return new_symbol(st, name, st->num_definitions++, scope);
}

Symbol *sym_resolve(SymbolTable *st, char *name) {
    Symbol* sym = ht_get(st->store, name);
    if (sym == NULL && st->outer != NULL) {
        return sym_resolve(st->outer, name);
    }
    return sym;
}

Symbol *define_builtin(SymbolTable *st, int index, const char *name) {
    return new_symbol(st, (char *)name, index, BuiltinScope);
}
