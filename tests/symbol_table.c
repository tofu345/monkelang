#include "unity/unity.h"

#include "../src/symbol_table.h"

#include <string.h>

SymbolTable *global = NULL,
            *first_local = NULL,
            *second_local = NULL;

void setUp(void) {}

inline static void
free_sym_tbl(SymbolTable **tbl) {
    if (*tbl) {
        symbol_table_free(*tbl);
        *tbl = NULL;
    }
}

void tearDown(void) {
    free_sym_tbl(&global);
    free_sym_tbl(&first_local);
    free_sym_tbl(&second_local);
}

static bool symbol_eq(Symbol *a, Symbol *b);
static bool test_sym_define(SymbolTable *table, Symbol *expected, char *name);
static bool test_sym_resolve(SymbolTable *table, Symbol expected[], int len);
static bool test_sym_resolve_free(SymbolTable *table, Symbol expected[],
        int len, Symbol free[], int free_len);

void test_define(void) {
    Symbol expected[] = {
        {.name = "a", .scope = GlobalScope, .index = 0},
        {.name = "b", .scope = GlobalScope, .index = 1},
        {.name = "c", .scope = LocalScope, .index = 0},
        {.name = "d", .scope = LocalScope, .index = 1},
        {.name = "e", .scope = LocalScope, .index = 0},
        {.name = "f", .scope = LocalScope, .index = 1},
    };

    global = symbol_table_new();
    TEST_ASSERT(test_sym_define(global, &expected[0], "a"));
    TEST_ASSERT(test_sym_define(global, &expected[1], "b"));

    first_local = enclosed_symbol_table(global);
    TEST_ASSERT(test_sym_define(first_local, &expected[2], "c"));
    TEST_ASSERT(test_sym_define(first_local, &expected[3], "d"));

    second_local = enclosed_symbol_table(global);
    TEST_ASSERT(test_sym_define(second_local, &expected[4], "e"));
    TEST_ASSERT(test_sym_define(second_local, &expected[5], "f"));
}

void test_resolve_global(void) {
    global = symbol_table_new();
    sym_define(global, "a", hash_fnv1a("a"));
    sym_define(global, "b", hash_fnv1a("b"));

    Symbol expected[] = {
        {.name = "a", .scope = GlobalScope, .index = 0},
        {.name = "b", .scope = GlobalScope, .index = 1},
    };
    int len = sizeof(expected) / sizeof(expected[0]);

    TEST_ASSERT(test_sym_resolve(global, expected, len));
}

void test_resolve_local(void) {
    global = symbol_table_new();
    sym_define(global, "a", hash_fnv1a("a"));
    sym_define(global, "b", hash_fnv1a("b"));

    first_local = enclosed_symbol_table(global);
    sym_define(first_local, "c", hash_fnv1a("c"));
    sym_define(first_local, "d", hash_fnv1a("d"));

    Symbol expected[] = {
        {.name = "a", .scope = GlobalScope, .index = 0},
        {.name = "b", .scope = GlobalScope, .index = 1},
        {.name = "c", .scope = LocalScope, .index = 0},
        {.name = "d", .scope = LocalScope, .index = 1},
    };
    int len = sizeof(expected) / sizeof(expected[0]);

    TEST_ASSERT(test_sym_resolve(first_local, expected, len));
}

void test_resolve_nested_local(void) {
    global = symbol_table_new();
    sym_define(global, "a", hash_fnv1a("a"));
    sym_define(global, "b", hash_fnv1a("b"));

    first_local = enclosed_symbol_table(global);
    sym_define(first_local, "c", hash_fnv1a("c"));
    sym_define(first_local, "d", hash_fnv1a("d"));

    second_local = enclosed_symbol_table(global);
    sym_define(second_local, "e", hash_fnv1a("e"));
    sym_define(second_local, "f", hash_fnv1a("f"));

    Symbol first_expected[] = {
        {.name = "a", .scope = GlobalScope, .index = 0},
        {.name = "b", .scope = GlobalScope, .index = 1},
        {.name = "c", .scope = LocalScope, .index = 0},
        {.name = "d", .scope = LocalScope, .index = 1},
    };
    int first_len = sizeof(first_expected) / sizeof(first_expected[0]);

    TEST_ASSERT(test_sym_resolve(first_local, first_expected, first_len));

    Symbol second_expected[] = {
        {.name = "a", .scope = GlobalScope, .index = 0},
        {.name = "b", .scope = GlobalScope, .index = 1},
        {.name = "e", .scope = LocalScope, .index = 0},
        {.name = "f", .scope = LocalScope, .index = 1},
    };
    int second_len = sizeof(second_expected) / sizeof(second_expected[0]);

    TEST_ASSERT(test_sym_resolve(second_local, second_expected, second_len));
}

void test_define_resolve_builtins(void) {
    global = symbol_table_new();
    first_local = enclosed_symbol_table(global);
    second_local = enclosed_symbol_table(global);

    Symbol expected[] = {
        {.name = "a", .scope = BuiltinScope, .index = 0},
        {.name = "c", .scope = BuiltinScope, .index = 1},
        {.name = "e", .scope = BuiltinScope, .index = 2},
        {.name = "f", .scope = BuiltinScope, .index = 3},
    };
    int len = sizeof(expected) / sizeof(expected[0]);

    for (int i = 0; i < len; i++) {
        sym_builtin(global, i, expected[i].name);
    }

    TEST_ASSERT(test_sym_resolve(global, expected, len));
    TEST_ASSERT(test_sym_resolve(first_local, expected, len));
    TEST_ASSERT(test_sym_resolve(second_local, expected, len));
}

void test_resolve_free(void) {
    global = symbol_table_new();
    sym_define(global, "a", hash_fnv1a("a"));
    sym_define(global, "b", hash_fnv1a("b"));

    first_local = enclosed_symbol_table(global);
    sym_define(first_local, "c", hash_fnv1a("c"));
    sym_define(first_local, "d", hash_fnv1a("d"));

    second_local = enclosed_symbol_table(first_local);
    sym_define(second_local, "e", hash_fnv1a("e"));
    sym_define(second_local, "f", hash_fnv1a("f"));

    Symbol expected_symbols[] = {
        {.name = "a", .scope = GlobalScope, .index = 0},
        {.name = "b", .scope = GlobalScope, .index = 1},
        {.name = "c", .scope = LocalScope, .index = 0},
        {.name = "d", .scope = LocalScope, .index = 1},
    };
    int len = sizeof(expected_symbols) / sizeof(expected_symbols[0]);

    TEST_ASSERT(test_sym_resolve_free(first_local, expected_symbols, len,
                NULL, 0));

    Symbol second_expected_symbols[] = {
        {.name = "a", .scope = GlobalScope, .index = 0},
        {.name = "b", .scope = GlobalScope, .index = 1},
        {.name = "c", .scope = FreeScope, .index = 0},
        {.name = "d", .scope = FreeScope, .index = 1},
        {.name = "e", .scope = LocalScope, .index = 0},
        {.name = "f", .scope = LocalScope, .index = 1},
    };
    int second_len =
        sizeof(second_expected_symbols) / sizeof(second_expected_symbols[0]);

    Symbol second_free[] = {
        {.name = "c", .scope = LocalScope, .index = 0},
        {.name = "d", .scope = LocalScope, .index = 1},
    };
    int len_free = sizeof(second_free) / sizeof(second_free[0]);

    TEST_ASSERT(test_sym_resolve_free(second_local,
                second_expected_symbols, second_len,
                second_free, len_free));
}

void test_resolve_unresolvable_free(void) {
    global = symbol_table_new();
    sym_define(global, "a", hash_fnv1a("a"));

    first_local = enclosed_symbol_table(global);
    sym_define(first_local, "c", hash_fnv1a("c"));

    second_local = enclosed_symbol_table(first_local);
    sym_define(second_local, "e", hash_fnv1a("e"));
    sym_define(second_local, "f", hash_fnv1a("f"));

    Symbol expected[] = {
        {.name = "a", .scope = GlobalScope, .index = 0},
        {.name = "c", .scope = FreeScope, .index = 0},
        {.name = "e", .scope = LocalScope, .index = 0},
        {.name = "f", .scope = LocalScope, .index = 1},
    };
    int len = sizeof(expected) / sizeof(expected[0]);

    TEST_ASSERT(test_sym_resolve(second_local, expected, len));

    char *expected_unresolvable[] = {
        "b",
        "d",
    };
    int unresolvable_len =
        sizeof(expected_unresolvable) / sizeof(expected_unresolvable[0]);

    bool pass = true;
    for (int i = 0; i < unresolvable_len; i++) {
        Symbol *result =
            sym_resolve(second_local, hash_fnv1a(expected_unresolvable[i]));

        if (result != NULL) {
            printf("name %s resolved, but was expected not to\n",
                    expected_unresolvable[i]);
            pass = false;
        }
    }
    TEST_ASSERT(pass);
}

void test_define_and_resolve_function_name(void) {
    global = symbol_table_new();
    sym_function_name(global, "a", hash_fnv1a("a"));

    Symbol expected = {.name = "a", .scope = FunctionScope, .index = 0};

    TEST_ASSERT(test_sym_resolve(global, &expected, 1));
}

void test_shadowing_function_name(void) {
    global = symbol_table_new();
    sym_function_name(global, "a", hash_fnv1a("a"));
    sym_define(global, "a", hash_fnv1a("a"));

    Symbol expected = {.name = "a", .scope = GlobalScope, .index = 0};

    TEST_ASSERT(test_sym_resolve(global, &expected, 1));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_define);
    RUN_TEST(test_resolve_global);
    RUN_TEST(test_resolve_local);
    RUN_TEST(test_define_resolve_builtins);
    RUN_TEST(test_resolve_free);
    RUN_TEST(test_resolve_unresolvable_free);
    RUN_TEST(test_define_and_resolve_function_name);
    RUN_TEST(test_shadowing_function_name);
    return UNITY_END();
}

static bool
symbol_eq(Symbol *a, Symbol *b) {
    return !strcmp(a->name, b->name)
        && a->scope == b->scope
        && a->index == b->index;
}

const char *scopes[] = {
    "GlobalScope",
    "LocalScope",
    "FunctionScope",
    "FreeScope",
    "BuiltinScope",
};

static const char *
show_scope(SymbolScope scope) {
    if (scope < 0 || scope > BuiltinScope) { die("invalid scope %d", scope); }
    return scopes[scope];
}

static bool
test_sym_resolve(SymbolTable *table, Symbol expected[], int len) {
    Symbol *result;
    for (int i = 0; i < len; i++) {
        result = sym_resolve(table, hash_fnv1a(expected[i].name));
        if (result == NULL) {
            printf("name '%s' not resolvable\n", expected[i].name);
            return false;
        }

        if (!symbol_eq(result, &expected[i])) {
            printf("expected '%s' to sym_resolve to Symbol(%s, %s, %d) got Symbol(%s, %s, %d)\n",
                    expected[i].name,
                    expected[i].name, show_scope(expected[i].scope), expected[i].index,
                    result->name, show_scope(result->scope), result->index);
            return false;
        }
    }

    return true;
}

static bool
test_sym_resolve_free(SymbolTable *table, Symbol expected[], int len,
        Symbol free_symbols[], int free_len) {
    int err = test_sym_resolve(table, expected, len);
    if (err) { return err; }

    if (table->free_symbols.length != free_len) {
        printf("wrong number of free symbols. got=%d, want=%d",
                table->free_symbols.length, free_len);
        return false;
    }

    Symbol *result;
    for (int i = 0; i < free_len; i++) {
        result = table->free_symbols.data[i];
        if (!symbol_eq(result, free_symbols + i)) {
            printf("wrong free symbol: Symbol(%s, %s, %d) want Symbol(%s, %s, %d)\n",
                    result->name, show_scope(result->scope), result->index,
                    expected[i].name, show_scope(expected[i].scope), expected[i].index);
            return false;
        }
    }
    return true;
}

static bool
test_sym_define(SymbolTable *table, Symbol *expected, char *name) {
    Symbol *a = sym_define(table, name, hash_fnv1a(name));
    if (!symbol_eq(a, expected)) {
        printf("expected Symbol(%s, %s, %d) got Symbol(%s, %s, %d)\n",
                expected->name, show_scope(expected->scope), expected->index,
                a->name, show_scope(a->scope), a->index);
        return false;
    }
    return true;
}
