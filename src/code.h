#pragma once

#include "buffer.h"

#include <stdarg.h>
#include <stdint.h>

typedef enum {
    OpConstant = 1,
} Opcode;

typedef struct {
    int *data, length;
} Operands;

typedef struct {
    const char *name;
    Operands widths;
} Definition;

const Definition *lookup(Opcode op);

typedef struct {
    uint8_t* data;
    int length, capacity;
} Instructions;

// Reads operands in `ins[1:]` with [def] and store num bytes read in [n]
Operands read_operands(int *n, const Definition *def, uint8_t *ins);

// Takes [Opcode] and [int] operands
Instructions make(Opcode op, ...);

// Takes [Opcode] and va_list of [int] operands.
// Calls [va_end()] on [operands]
Instructions make_valist(Opcode op, va_list operands);

// allocate [length] extra elements and increment [buf.length] by [length]
void instructions_allocate(Instructions *buf, int length);
int fprint_instruction(FILE *out, Instructions ins);
