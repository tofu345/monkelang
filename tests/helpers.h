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
} ExpectedConstants;

#define _C(...) constants(__VA_ARGS__, NULL)
#define _I(...) concat(__VA_ARGS__, NULL)

Instructions *concat(Instructions cur, ...);
ExpectedConstants constants(Test *t, ...);

Program test_parse(char *input);

// print and free any errors, set [errs.length] to 0
void print_errors(ErrorBuffer* errs);
void check_parser_errors(Parser* p);
void check_compiler_errors(Compiler* c);
