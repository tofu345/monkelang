#pragma once

#include "../src/utils.h"
#include "../src/code.h"
#include "../src/ast.h"
#include "../src/parser.h"
#include "../src/compiler.h"

typedef struct {
    Constant *data;
    int length;
} Constants;

#define STR(v) (Constant){ c_String, { .string = v } }
#define INT(v) (Constant){ c_Integer, { .integer = v } }
// #define INS(...) (Constant){ c_ins, { ._ins = _I(__VA_ARGS__) } }

// This works.
#define _C(...) constants(__VA_ARGS__, NULL)
#define _I(...) concat(__VA_ARGS__, NULL)

// append all Instructions after [out] into [out]
Instructions concat(Instructions out, ...);
// return [Constants] of [Constant]'s of arbitrary length
Constants constants(Constant c, ...);

Program test_parse(char *input);
// print and free any errors, set [errs.length] to 0
void print_errors(ErrorBuffer* errs);
void check_parser_errors(Parser* p);
void check_compiler_errors(Compiler* c);
