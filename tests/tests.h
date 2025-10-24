#pragma once

#include <stdbool.h>

#include "../src/code.h"

typedef struct {
    int *data, length;
} TestArray;

typedef enum {
    test_null,
    test_int,
    test_float,
    test_str,
    test_bool,
    test_arr,
    test_table,
    test_ins,
} Type;

typedef union {
    long _int;
    double _float;
    char* _str;
    bool _bool;
    TestArray *_arr;
    Instructions *_ins;
} Value;

typedef struct {
    Type typ;
    Value val;
} Test;

#define TEST(t, v) &(Test){ test_##t, { ._##t = v } }
#define TEST_NULL &(Test){}
