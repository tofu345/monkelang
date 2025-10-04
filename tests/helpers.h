#pragma once

#include "../src/code.h"
#include "../src/buffer.h"
#include "../src/ast.h"
#include "../src/parser.h"
#include "../src/compiler.h"

// The lengths one must go to, to achieve a measure of convenience in C.
typedef struct {
    enum {
        c_int = 1,
        c_ins,
    } type;
    union {
        long _int;
        Instructions *_ins;
    } data;
} Constant;

typedef struct {
    Constant *data;
    int length;
} Constants;

#define INT(v) (Constant){ c_int, { ._int = v } }
#define INS(...) (Constant){ c_ins, { ._ins = _I(__VA_ARGS__) } }

// This is likely not the best way.
#define _C(...) constants(__VA_ARGS__, NULL)
#define _I(...) concat(__VA_ARGS__, NULL)

// append all Instructions after [ins] into [ins]
Instructions concat(Instructions out, ...);
// return [Constants] of [Constant]'s of arbitrary length
Constants constants(Constant c, ...);

Program parse(char *input);
void print_errors(StringBuffer* errs);
void check_parser_errors(Parser* p);
void check_compiler_errors(Compiler* c);

int test_integer_object(long expected, Object actual);
int test_boolean_object(bool expected, Object actual);
