#pragma once

#include <stdbool.h>

#include "../src/code.h"

typedef enum {
    test_null,
    test_int,
    test_float,
    test_str,
    test_bool,
} Type;

typedef union {
    long _int;
    double _float;
    const char* _str;
    bool _bool;
} Value;

typedef struct {
    Type typ;
    Value val;
} Test;

#define TEST(t, v) (Test){ test_##t, { ._##t = v } }
#define TEST_NULL (Test){}
