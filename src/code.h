#pragma once

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

    OpGetGlobal,
    OpSetGlobal,

    OpArray,
    OpTable,
    OpIndex,
    OpSetIndex,

    OpCall,
    OpReturnValue,
    OpReturn,

    OpGetLocal,
    OpSetLocal,

    OpGetBuiltin,

    OpClosure,
    OpGetFree,
    OpCurrentClosure,
} Opcode;

// Operands widths in bytes and number.
typedef struct {
    int *widths, length;
} Operands;

typedef struct {
    const char *name;
    const Operands operands;
} Definition;

const Definition *lookup(Opcode op);

typedef struct {
    uint8_t* data;
    int length, capacity;
} Instructions;

// allocate [length] extra elements and increment [buf.length] by [length].
void instructions_allocate(Instructions *buf, int length);

int fprint_instructions(FILE *out, Instructions ins);

// Reads operands in `ins[1:]` with [def] and store num bytes read in [n].
Operands read_operands(int *n, const Definition *def, uint8_t *ins);

// read `arr[0:2]` big endian.
int read_big_endian_uint16(uint8_t *arr);

// read `arr[0]`
int read_big_endian_uint8(uint8_t *arr);

Instructions make(Opcode op, ...);
int make_into(Instructions *ins, Opcode op, ...);
int make_valist_into(Instructions *ins, Opcode op, va_list operands);
