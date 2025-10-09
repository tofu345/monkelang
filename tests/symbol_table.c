#include "unity/unity.h"
#include <string.h>

#include "../src/symbol_table.h"

void setUp(void) {}
void tearDown(void) {}

static bool
symbol_eq(Symbol *a, Symbol *b) {
    return !strcmp(a->name, b->name)
        && a->scope == b->scope
        && a->index == b->index;
}

void test_define(void) {
    Symbol expected[] = {
        {.name = "a", .scope = GlobalScope, .index = 0},
        {.name = "b", .scope = GlobalScope, .index = 1},
    };

    SymbolTable global;
    symbol_table_init(&global);

    Symbol *a = sym_define(&global, "a");
    if (!symbol_eq(a, &expected[0])) {
        printf("expected Symbol(%s, %d, %d) got Symbol(%s, %d, %d)",
                a->name, a->scope, a->index,
                expected[0].name, expected[0].scope, expected[0].index);
        TEST_FAIL();
    }

    Symbol *b = sym_define(&global, "b");
    if (!symbol_eq(b, &expected[1])) {
        printf("expected Symbol(%s, %d, %d) got Symbol(%s, %d, %d)",
                a->name, a->scope, a->index,
                expected[1].name, expected[1].scope, expected[1].index);
        TEST_FAIL();
    }

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

    Symbol *result;
    for (int i = 0; i < len; i++) {
        result = sym_resolve(&global, expected[i].name);
        if (result == NULL) {
            printf("name %s not resolvable", expected[i].name);
            TEST_FAIL();
        }
        if (!symbol_eq(result, &expected[i])) {
            printf("expected %s to sym_resolve to Symbol(%s, %d, %d) got Symbol(%s, %d, %d)",
                    expected[i].name,
                    expected[i].name, expected[i].scope, expected[i].index,
                    result->name, result->scope, result->index);
            TEST_FAIL();
        }
    }

    symbol_table_free(&global);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_define);
    RUN_TEST(test_resolve_global);
    return UNITY_END();
}
