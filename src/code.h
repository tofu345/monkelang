#pragma once

#include "buffer.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
    OpConstant = 1,
    OpPop,
    OpAdd,
    OpSub,
    OpMul,
    OpDiv,

    OpTrue,
    OpFalse,

    OpEqual,
    OpNotEqual,
    OpGreaterThan,

    OpMinus,
    OpBang,

    OpJumpNotTruthy,
    OpJump,
    OpNull,
} Opcode;

typedef struct {
    int *data, length;
} Operands;

typedef struct {
    const char *name;
    const int *widths, length;
} Definition;

const Definition *lookup(Opcode op);

typedef struct {
    uint8_t* data;
    int length, capacity;
} Instructions;

// Reads operands in `ins[1:]` with [def] and store num bytes read in [n]
Operands read_operands(int *n, const Definition *def, uint8_t *ins);

// read `arr[0:2]` big endian.
int read_big_endian_uint16(uint8_t *arr);

// Takes [Opcode] and [int] operands
Instructions make(Opcode op, ...);

// Takes [Opcode] and va_list of [int] operands.
// Calls [va_end()] on [operands]
Instructions make_valist(Opcode op, va_list operands);

// allocate [length] extra elements and increment [buf.length] by [length]
void instructions_allocate(Instructions *buf, int length);
int fprint_instructions(FILE *out, Instructions ins);
