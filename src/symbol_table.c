#include "symbol_table.h"
#include "hash-table/ht.h"
#include "utils.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

DEFINE_BUFFER(Symbol, Symbol *)

SymbolTable *
symbol_table_new() {
    SymbolTable *st = calloc(1, sizeof(SymbolTable));
    if (st == NULL) { die("malloc:"); }

    st->store = ht_create();
    if (st->store == NULL) { die("symbol_table_init:"); }

    return st;
}

SymbolTable *
enclosed_symbol_table(SymbolTable *outer) {
    SymbolTable *st = symbol_table_new();
    st->outer = outer;
    return st;
}

void symbol_table_free(SymbolTable *st) {
    hti it = ht_iterator(st->store);
    while (ht_next(&it)) {
        free(it.current->value);
    }
    ht_destroy(st->store);
    free(st->free_symbols.data);
    free(st);
}

static Symbol *
new_symbol(SymbolTable *st, void *name, uint64_t hash, int index,
           SymbolScope scope) {

    Symbol *symbol = malloc(sizeof(Symbol));
    if (symbol == NULL) { die("new_symbol: malloc"); }

    Symbol *ptr = ht_set_hash(st->store, (void *) name, symbol, hash);
    if (errno == ENOMEM) {
        die("new_symbol ht_set:");

    // Previous symbol with same [name]
    } else if (ptr != symbol) {
        // If the previous variable is in the `LocalScope` or `GlobalScope`
        // then a new variable was not created, but the previous variable was
        // shadowed.
        if (ptr->scope <= LocalScope) {
            --st->num_definitions; // see new_symbol() call in sym_define()

            // use previous variables index.
            memcpy(symbol, ptr, sizeof(Symbol));
            free(ptr);
            return symbol;

        // Previous variable is assumed to be in the `FreeScope`,
        // a new variable is created to replace it.
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
define_free(SymbolTable *st, Symbol *original, uint64_t hash) {
    int index = st->free_symbols.length;
    SymbolBufferPush(&st->free_symbols, original);
    return new_symbol(st, original->name, hash, index, FreeScope);
}

Symbol *sym_define(SymbolTable *st, Token *name, uint64_t hash) {
    SymbolScope scope;
    if (st->outer == NULL) {
        scope = GlobalScope;
    } else {
        scope = LocalScope;
    }

    return new_symbol(st, name, hash, st->num_definitions++, scope);
}

static Symbol *
resolve(SymbolTable *st, uint64_t hash) {
    Symbol* sym = ht_get_hash(st->store, hash);
    if (sym == NULL && st->outer != NULL) {
        sym = resolve(st->outer, hash);
        if (sym == NULL) { return sym; }

        switch (sym->scope) {
            case GlobalScope:
            case BuiltinScope:
                return sym;
            default:
                return define_free(st, sym, hash);
        }
    }

    return sym;
}

Symbol *sym_resolve(SymbolTable *st, uint64_t hash) {
    return resolve(st, hash);
}

Symbol *sym_function_name(SymbolTable *st, Token *name, uint64_t hash) {
    return new_symbol(st, name, hash, 0, FunctionScope);
}

Symbol *sym_builtin(SymbolTable *st, const char *name, int index) {
    uint64_t hash = hash_fnv1a(name);
    return new_symbol(st, (void *) name, hash, index, BuiltinScope);
}
