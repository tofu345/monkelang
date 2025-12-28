#pragma once

#include "../src/utils.h"
#include "../src/code.h"
#include "../src/ast.h"
#include "../src/parser.h"
#include "../src/compiler.h"

#include <stdbool.h>

typedef struct {
    int *data, length;
} IntArray;

typedef enum {
    test_nothing,
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
    IntArray _arr;
    Instructions _ins;
} Value;

typedef struct {
    Type typ;
    Value val;
} Test;

#define TEST(t, v) &(Test){ test_##t, { ._##t = v } }
#define TEST_NOTHING &(Test){0}

typedef struct {
    Test *data;
    int length;
} Tests;

#define _C(...) constants(__VA_ARGS__, NULL)
#define NO_CONSTANTS (Tests){0}
#define _I(...) concat(__VA_ARGS__, NULL)

Instructions concat(Instructions first, ...);
Tests constants(Test *t, ...);

Program test_parse(const char *input);

void print_parser_errors(Parser *p);
