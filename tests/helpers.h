#pragma once

#include "../src/utils.h"
#include "../src/code.h"
#include "../src/ast.h"
#include "../src/parser.h"
#include "../src/compiler.h"

#include "tests.h"

typedef struct {
    Test *data;
    int length;
} Constants;

#define _C(...) constants(__VA_ARGS__, NULL)
#define NO_CONSTANTS (Constants){0}
#define _I(...) concat(__VA_ARGS__, NULL)

Instructions concat(Instructions first, ...);
Constants constants(Test *t, ...);

Program test_parse(const char *input);

void print_parser_errors(Parser *p);
