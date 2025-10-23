#include "symbol_table.h"
#include "hash-table/ht.h"
#include "utils.h"

#include <stdbool.h>
#include <string.h>

DEFINE_BUFFER(Symbol, Symbol *);

void symbol_table_init(SymbolTable *st) {
    memset(st, 0, sizeof(SymbolTable));
    st->store = ht_create();
    if (st->store == NULL) { die("symbol_table_init"); }
}

void enclosed_symbol_table(SymbolTable *st, SymbolTable *outer) {
    symbol_table_init(st);
    st->outer = outer;
}

void symbol_table_free(SymbolTable *st) {
    hti it = ht_iterator(st->store);
    while (ht_next(&it)) {
        free(it.current->value);
    }
    ht_destroy(st->store);
    free(st->free_symbols.data);
}

static Symbol *
new_symbol(SymbolTable *st, char *name, int index, SymbolScope scope) {
    Symbol *symbol = malloc(sizeof(Symbol));
    if (symbol == NULL) { die("new_symbol: malloc"); }

    Symbol *ptr = ht_set(st->store, name, symbol);
    if (ptr == NULL) {
        die("new_symbol: ht_set");

    // previous symbol with same [name]
    } else if (ptr != symbol) {
        // if in [LocalScope] or [GlobalScope]
        if (ptr->scope <= LocalScope) {
            // a new variable was not created.
            st->num_definitions--;
            // [symbol] replaced [ptr] in hash-table,
            // copy [ptr] to use previous index.
            memcpy(symbol, ptr, sizeof(Symbol));
            free(ptr);
            return symbol;

        } else {
            free(ptr);
        }
    }

    *symbol = (Symbol) {
        .name = name,
        .index = index,
        .scope = scope,
    };

    return symbol;
}

static Symbol *
define_free(SymbolTable *st, Symbol *original) {
    int index = st->free_symbols.length;
    SymbolBufferPush(&st->free_symbols, original);
    return new_symbol(st, original->name, index, FreeScope);
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
        sym = sym_resolve(st->outer, name);
        if (sym == NULL) { return sym; }

        switch (sym->scope) {
            case GlobalScope:
            case BuiltinScope:
                return sym;
            default:
                return define_free(st, sym);
        }
    }
    return sym;
}

Symbol *sym_function_name(SymbolTable *st, char *name) {
    return new_symbol(st, name, 0, FunctionScope);
}

Symbol *sym_builtin(SymbolTable *st, int index, const char *name) {
    return new_symbol(st, (char *)name, index, BuiltinScope);
}
