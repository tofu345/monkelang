#include "unity/unity.h"

#include "../src/symbol_table.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static bool symbol_eq(Symbol *a, Symbol *b);
static int test_sym_define(SymbolTable *table, Symbol *expected, char *name);
static int test_sym_resolve(SymbolTable *table, Symbol expected[], int len);

void test_define(void) {
    Symbol expected[] = {
        {.name = "a", .scope = GlobalScope, .index = 0},
        {.name = "b", .scope = GlobalScope, .index = 1},
        {.name = "c", .scope = LocalScope, .index = 0},
        {.name = "d", .scope = LocalScope, .index = 1},
        {.name = "e", .scope = LocalScope, .index = 0},
        {.name = "f", .scope = LocalScope, .index = 1},
    };

    SymbolTable global;
    symbol_table_init(&global);

    int err = test_sym_define(&global, &expected[0], "a");
    if (err != 0) { TEST_FAIL(); }

    err = test_sym_define(&global, &expected[1], "b");
    if (err != 0) { TEST_FAIL(); }

    SymbolTable first_local;
    enclosed_symbol_table(&first_local, &global);

    err = test_sym_define(&first_local, &expected[2], "c");
    if (err != 0) { TEST_FAIL(); }

    err = test_sym_define(&first_local, &expected[3], "d");
    if (err != 0) { TEST_FAIL(); }

    SymbolTable second_local;
    enclosed_symbol_table(&second_local, &global);

    err = test_sym_define(&second_local, &expected[4], "e");
    if (err != 0) { TEST_FAIL(); }

    err = test_sym_define(&second_local, &expected[5], "f");
    if (err != 0) { TEST_FAIL(); }

    symbol_table_free(&first_local);
    symbol_table_free(&second_local);
    symbol_table_free(&global);
}

void test_resolve_global(void) {
    SymbolTable global;
    symbol_table_init(&global);

    sym_define(&global, "a");
    sym_define(&global, "b");

    Symbol expected[] = {
        {.name = "a", .scope = GlobalScope, .index = 0},
        {.name = "b", .scope = GlobalScope, .index = 1},
    };
    int len = sizeof(expected) / sizeof(expected[0]);

    int err = test_sym_resolve(&global, expected, len);

    symbol_table_free(&global);

    if (err != 0) { TEST_FAIL(); }
}

void test_resolve_local(void) {
    SymbolTable global;
    symbol_table_init(&global);

    sym_define(&global, "a");
    sym_define(&global, "b");

    SymbolTable local;
    enclosed_symbol_table(&local, &global);

    sym_define(&local, "c");
    sym_define(&local, "d");

    Symbol expected[] = {
        {.name = "a", .scope = GlobalScope, .index = 0},
        {.name = "b", .scope = GlobalScope, .index = 1},
        {.name = "c", .scope = LocalScope, .index = 0},
        {.name = "d", .scope = LocalScope, .index = 1},
    };
    int len = sizeof(expected) / sizeof(expected[0]);

    int err = test_sym_resolve(&local, expected, len);

    symbol_table_free(&local);
    symbol_table_free(&global);

    if (err != 0) { TEST_FAIL(); }
}

void test_resolve_nested_local(void) {
    SymbolTable global;
    symbol_table_init(&global);

    sym_define(&global, "a");
    sym_define(&global, "b");

    SymbolTable first_local;
    enclosed_symbol_table(&first_local, &global);

    sym_define(&first_local, "c");
    sym_define(&first_local, "d");

    SymbolTable second_local;
    enclosed_symbol_table(&second_local, &global);

    sym_define(&second_local, "e");
    sym_define(&second_local, "f");

    Symbol first_expected[] = {
        {.name = "a", .scope = GlobalScope, .index = 0},
        {.name = "b", .scope = GlobalScope, .index = 1},
        {.name = "c", .scope = LocalScope, .index = 0},
        {.name = "d", .scope = LocalScope, .index = 1},
    };
    int first_len = sizeof(first_expected) / sizeof(first_expected[0]);

    int err = test_sym_resolve(&first_local, first_expected, first_len);
    if (err != 0) { TEST_FAIL_MESSAGE("first_expected wrong"); }

    Symbol second_expected[] = {
        {.name = "a", .scope = GlobalScope, .index = 0},
        {.name = "b", .scope = GlobalScope, .index = 1},
        {.name = "e", .scope = LocalScope, .index = 0},
        {.name = "f", .scope = LocalScope, .index = 1},
    };
    int second_len = sizeof(second_expected) / sizeof(second_expected[0]);

    err = test_sym_resolve(&second_local, second_expected, second_len);
    if (err != 0) { TEST_FAIL_MESSAGE("second_expected wrong"); }

    symbol_table_free(&global);
    symbol_table_free(&first_local);
    symbol_table_free(&second_local);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_define);
    RUN_TEST(test_resolve_global);
    RUN_TEST(test_resolve_local);
    return UNITY_END();
}

static bool
symbol_eq(Symbol *a, Symbol *b) {
    return !strcmp(a->name, b->name)
        && a->scope == b->scope
        && a->index == b->index;
}

static int
test_sym_resolve(SymbolTable *table, Symbol expected[], int len) {
    Symbol *result;
    for (int i = 0; i < len; i++) {
        result = sym_resolve(table, expected[i].name);
        if (result == NULL) {
            printf("name '%s' not resolvable\n", expected[i].name);
            return -1;
        }

        if (!symbol_eq(result, &expected[i])) {
            printf("expected '%s' to sym_resolve to Symbol(%s, %d, %d) got Symbol(%s, %d, %d)\n",
                    expected[i].name,
                    expected[i].name, expected[i].scope, expected[i].index,
                    result->name, result->scope, result->index);
            return -1;
        }
    }

    return 0;
}

static int
test_sym_define(SymbolTable *table, Symbol *expected, char *name) {
    Symbol *a = sym_define(table, name);
    if (!symbol_eq(a, expected)) {
        printf("expected Symbol('%s', %d, %d) got Symbol('%s', %d, %d)\n",
                expected->name, expected->scope, expected->index,
                a->name, a->scope, a->index);
        return -1;
    }
    return 0;
}
