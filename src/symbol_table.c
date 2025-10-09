#include "symbol_table.h"
#include "hash-table/ht.h"
#include "utils.h"

void symbol_table_init(SymbolTable *st) {
    st->store = ht_create();
    if (st->store == NULL) die("malloc");
}

void symbol_table_free(SymbolTable *st) {
    hti it = ht_iterator(st->store);
    while (ht_next(&it)) {
        free(it.current->value);
    }
    ht_destroy(st->store);
}

Symbol *sym_define(SymbolTable *st, char *name) {
    Symbol *symbol = malloc(sizeof(Symbol));
    if (symbol == NULL) { die("sym_define: malloc"); }
    symbol->name = name;
    symbol->index = st->store->length;
    symbol->scope = GlobalScope;

    void *ptr = ht_set(st->store, name, symbol);
    if (ptr == NULL) {
        die("symbol_table define");

    // free previous symbol with same [name]
    } else if (ptr != symbol) {
        free(ptr);
    }
    return symbol;
}

Symbol *sym_resolve(SymbolTable *st, char *name) {
    return ht_get(st->store, name);
}
