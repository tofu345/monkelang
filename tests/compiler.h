#pragma once

#include "../src/code.h"

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

Instructions concat(Instructions out, ...);
Constants constants(Constant c, ...);

#define INT(v) (Constant){ c_int, { ._int = v } }
#define INS(...) (Constant){ c_ins, { ._ins = _I(__VA_ARGS__) } }

// This is likely not the correct way.
#define _C(...) constants(__VA_ARGS__, NULL)
#define _I(...) concat(__VA_ARGS__, NULL)

static void run_compiler_test(
    char *input,
    Instructions expectedInstructions,
    Constants expectedConstants
);
