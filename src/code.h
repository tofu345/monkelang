#pragma once

#include "buffer.h"

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
Instructions make(Opcode op, int operand, ...);

void instructions_fill(Instructions *buf, uint8_t val, int length);
int fprint_instruction(FILE *out, Instructions ins);
