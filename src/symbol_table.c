#include "symbol_table.h"
#include "hash-table/ht.h"
#include "utils.h"

void symbol_table_init(SymbolTable *st) {
    st->store = ht_create();
    if (st->store == NULL) die("symbol_table_init");
}

void symbol_table_free(SymbolTable *st) {
    hti it = ht_iterator(st->store);
    while (ht_next(&it)) {
        free(((Symbol *)it.current->value)->name);
        free(it.current->value);
    }
    ht_destroy(st->store);
}

Symbol *sym_define(SymbolTable *st, char *name) {
    Symbol *symbol = malloc(sizeof(Symbol));
    if (symbol == NULL) { die("sym_define: malloc"); }
    name = strdup(name);
    if (name == NULL) { die("sym_define: strdup"); }

    symbol->name = name;
    symbol->index = st->store->length;
    symbol->scope = GlobalScope;

    void *ptr = ht_set(st->store, name, symbol);
    if (ptr == NULL) {
        die("symbol_table define");

    // previous symbol with same [name]
    // NOTE: this is likely to change.
    } else if (ptr != symbol) {
        memcpy(symbol, ptr, sizeof(Symbol));
        free(name);
        free(ptr);
    }
    return symbol;
}

Symbol *sym_resolve(SymbolTable *st, char *name) {
    return ht_get(st->store, name);
}
