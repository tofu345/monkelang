#include "unity/unity.h"

#include "../src/object.h"
#include "../src/table.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

static void expect_set(Object key, Object val);
static void expect_get(Object key, Object expected_val);
static void expect_remove(Object key);

Table tbl;

void setUp(void) {
    if (table_init(&tbl) == NULL) {
        die("malloc");
    }
}

void tearDown(void) {
    table_free(&tbl);
}

static void
test_table_set_get(void) {
    CharBuffer *string = &(CharBuffer){ .data = "test"};
    Object objs[] = {
        // These have the same ObjectData
        OBJ(o_Integer, 1), OBJ(o_Boolean, true),
        OBJ(o_Integer, 0), OBJ(o_Boolean, false),

        // test other types
        OBJ(o_Float, .floating = 1.5),
        OBJ(o_String, .string = string),
    };
    int i, len = sizeof(objs) / sizeof(objs[0]);

    for (i = 0; i < len; ++i) {
        expect_set(objs[i], objs[i]);
    }

    if (tbl.length != (size_t) len) {
        printf("wrong table length, want=%d, got=%zu", len, tbl.length);
        TEST_FAIL();
    }

    for (i = 0; i < len; ++i) {
        expect_get(objs[i], objs[i]);
    }
}

// Tests:
// - tbl_iterator returns all values in Table.
// - table_expand() copies all data.
static void
test_table_iterator_and_expand(void) {
    int i,
        num = 129; // This should call table_expand() three times.

    for (i = 0; i < num; i++) {
        expect_set(OBJ(o_Integer, i), OBJ(o_Integer, i));
    }

    for (i = 0; i < num; ++i) {
        Object obj = OBJ(o_Integer, i);
        expect_get(obj, obj);
    }

    if (tbl.length != (size_t) num) {
        printf("wrong table length, want=%d, got=%zu", num, tbl.length);
        TEST_FAIL();
    }

    bool found[num];
    memset(found, 0, num);

    tbl_it it;
    tbl_iterator(&it, &tbl);
    while (tbl_next(&it)) {
        found[it.cur_key.data.integer] = true;
    }

    bool all_found = true;
    for (i = 0; i < num; i++) {
        if (!found[i]) {
            all_found = false;
            printf("iterator could not find object %d\n", i);
        }
    }
    TEST_ASSERT(all_found);
}

static void
test_table_remove(void) {
    int i, num = 50;
    for (i = 0; i < num; i++) {
        Object obj = OBJ(o_Integer, i);
        expect_get(obj, obj);
    }

    bool found[num];
    memset(found, 0, num);

    tbl_it it;

    // starting from the last number, remove and check if all other numbers are
    // still present.
    for (i = num - 1; i >= 0; --i) {
        expect_remove(OBJ(o_Integer, i));

        tbl_iterator(&it, &tbl);
        while (tbl_next(&it)) {
            found[it.cur_key.data.integer] = true;
        }

        bool all_found = true;
        for (int j = 0; j < i; ++j) {
            if (!found[j]) {
                all_found = false;
                printf("iterator could not find object %d\n", j);
            }
        }
        TEST_ASSERT(all_found);
    }

    // table_remove() on empty table
    Object res = table_remove(&tbl, OBJ(o_Integer, 0));
    if (res.type != o_Null) {
        printf("got object: ");
        object_fprint(res, stdout);
        printf("after table_remove() on empty table\n");
        TEST_FAIL();
    }

    if (tbl.length != 0) {
        printf("expected table length of 0 after table_remove() on empty table, got=%zu",
                tbl.length);
        TEST_FAIL();
    }
}

static void
expect_set(Object key, Object val) {
    Object res = table_set(&tbl, key, val);
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
expect_get(Object key, Object expected_val) {
    Object res = table_get(&tbl, key);

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
expect_remove(Object key) {
    int expected_length = tbl.length;
    if (expected_length > 0) { --expected_length; }

    Object res = table_remove(&tbl, key);

    if (res.type == o_Null) {
        printf("could not assign object: ");
        object_fprint(res, stdout);
        putc('\n', stdout);
        TEST_FAIL();
    }

    if (tbl.length != (size_t) expected_length) {
        printf("after ");
        object_fprint(res, stdout);
        printf(" is removed, length is incorrect, want=%d, got=%zu\n",
                expected_length, tbl.length);
        TEST_FAIL();
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_table_set_get);
    RUN_TEST(test_table_iterator_and_expand);
    RUN_TEST(test_table_remove);
    return UNITY_END();
}
