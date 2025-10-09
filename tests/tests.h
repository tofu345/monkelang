#pragma once

#include <stdbool.h>

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
} Type;

typedef union {
    long _int;
    double _float;
    char* _str;
    bool _bool;
    TestArray *_arr;
} Value;

typedef struct {
    Type typ;
    Value val;
} Test;

#define TEST(t, v) (Test){ test_##t, { ._##t = v } }
#define TEST_NULL (Test){}
