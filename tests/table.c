#include "unity/unity.h"

#include "../src/object.h"
#include "../src/table.h"

#include <stdio.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void
expect_set(table *tbl, Object key, Object val) {
    Object res = table_set(tbl, key, val);
    if (res.type == o_Null) {
        printf("table_set fail: ");
        object_fprint(key, stdout);
        printf(": ");
        object_fprint(val, stdout);
        putc('\n', stdout);
        TEST_FAIL();
    }
}

static void
expect_get(table *tbl, Object key, Object expected_val) {
    Object res = table_get(tbl, key);
    bool equal =
        res.type == expected_val.type
        && memcmp(&res.data, &expected_val.data, sizeof(ObjectData)) == 0;
    if (!equal) {
        printf("table_get fail: ");
        object_fprint(key, stdout);
        printf(" expected: ");
        object_fprint(expected_val, stdout);
        printf(" got: ");
        object_fprint(res, stdout);
        putc('\n', stdout);
        TEST_FAIL();
    }
}

static void
test_table_set(void) {
    table tbl;
    if (table_init(&tbl) == NULL) die("malloc");

    Object one = OBJ(o_Integer, 1);
    Object two = OBJ(o_Integer, 2);
    Object _true = OBJ(o_Boolean, true);
    Object _false = OBJ(o_Boolean, false);

    expect_set(&tbl, one, one);
    expect_set(&tbl, two, one);
    expect_set(&tbl, _true, two);
    expect_set(&tbl, _false, two);

    if (tbl.length != 4) {
        table_free(&tbl);
        TEST_FAIL_MESSAGE("wrong table length");
    }

    expect_get(&tbl, one, one);
    expect_get(&tbl, two, one);
    expect_get(&tbl, _true, two);
    expect_get(&tbl, _false, two);

    table_free(&tbl);
}

static void
test_table_iterator(void) {
    table tbl;
    if (table_init(&tbl) == NULL) die("malloc");

    int num = 15;
    for (int i = 0; i < num; i++) {
        table_set(&tbl, OBJ(o_Integer, i), OBJ(o_Integer, i));
    }

    bool is_found[num];
    memset(is_found, 0, num);

    tbl_it it;
    tbl_iterator(&it, &tbl);
    while (tbl_next(&it)) {
        int idx = it.cur_key.data.integer;
        if (idx < 0 || idx >= num) continue;
        is_found[idx] = true;
    }

    bool all_found = true;
    for (int i = 0; i < num; i++) {
        if (!is_found[i]) {
            all_found = false;
            printf("could not find %d\n", i);
        }
    }
    table_free(&tbl);
    TEST_ASSERT(all_found);
}

static void
test_table_expand(void) {
    table tbl;
    if (table_init(&tbl) == NULL) die("malloc");

    int num = 129;
    for (int i = 0; i < num; i++) {
        table_set(&tbl, OBJ(o_Integer, i), OBJ(o_Integer, i));
    }

    Object obj;
    for (int i = 0; i < num; i++) {
        obj = table_get(&tbl, OBJ(o_Integer, i));
        if (obj.data.integer != i) {
            printf("could not find number %d\n", i);
            table_free(&tbl);
            TEST_FAIL();
        }
    }

    if (tbl.length != (size_t)num) {
        table_free(&tbl);
        TEST_FAIL_MESSAGE("wrong table length");
    }

    bool is_found[num];
    memset(is_found, 0, num);

    tbl_it it;
    tbl_iterator(&it, &tbl);
    while (tbl_next(&it)) {
        int idx = it.cur_key.data.integer;
        if (idx < 0 || idx >= num) continue;
        is_found[idx] = true;
    }

    bool all_found = true;
    for (int i = 0; i < num; i++) {
        if (!is_found[i]) {
            all_found = false;
            printf("could not find %d\n", i);
        }
    }

    table_free(&tbl);
    TEST_ASSERT(all_found);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_table_set);
    RUN_TEST(test_table_iterator);
    RUN_TEST(test_table_expand);
    return UNITY_END();
}
