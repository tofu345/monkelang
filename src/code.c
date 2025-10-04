#include "code.h"
#include "utils.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define DEF(code) \
    { #code, { _##code, sizeof(_##code) / sizeof(_##code[0]) } }

int _OpConstant[] = { 2 };

const Definition definitions[] = {
    DEF(OpConstant),
};

const Definition *
lookup(Opcode op) {
    static int len = sizeof(definitions) / sizeof(definitions[0]);
    if (op <= 0 || (int)op > len) {
        die("Definition for Opcode '%d' not found", op);
    }
    return definitions + op - 1;
}

// read `arr[0:2]` big endian.
static int
read_big_endian_uint16(uint8_t *arr) {
    int out = (arr[0] << 8) | arr[1];
    return out;
}

// store n in `arr[0:2]` big endian.
static void
put_big_endian_uint16(uint8_t *arr, uint16_t n) {
// https://stackoverflow.com/a/9154012
#if defined(__BYTE_ORDER__)&&(__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#else
    // https://stackoverflow.com/a/2182184
    n = (n >> 8) | (n << 8); // swap to big endian
#endif

    arr[0] = n;
    arr[1] = n >> 8;
}

Operands read_operands(int *n, const Definition *def, uint8_t* ins) {
    Operands operands = {
        .data = allocate(def->widths.length * sizeof(int)),
        .length = def->widths.length
    };

    int width, offset = 0;
    for (int i = 0; i < def->widths.length; i++) {
        width = def->widths.data[i];
        switch (width) {
            case 2:
                // offset + 1 to skip Opcode
                operands.data[i] = read_big_endian_uint16(ins + offset + 1);
                break;
            default:
                die("read_operands: operand width %d not implemented for %s",
                        width, def->name);
        }
        offset += width;
    }
    *n = offset;
    return operands;
}

Instructions
make_valist(Opcode op, va_list operands) {
    const Definition *def = lookup(op);
    Instructions ins = {};
    instructions_allocate(&ins, 1);
    ins.data[0] = op;
    int operand, width, offset = 1;

    for (int i = 0; i < def->widths.length; i++) {
        operand = va_arg(operands, int);
        width = def->widths.data[i];
        instructions_allocate(&ins, width);
        switch (width) {
            case 2:
                put_big_endian_uint16(ins.data + offset, operand);
                break;
            default:
                die("make: operand width %d not implemented", width);
        }
        offset += width;
    }
    va_end(operands);
    return ins;
}

Instructions
make(Opcode op, ...) {
    va_list ap;
    va_start(ap, op);
    return make_valist(op, ap);
}

void instructions_allocate(Instructions *buf, int length) {
    if (buf->length + length >= buf->capacity) {
        int capacity = power_of_2_ceil(buf->length + length);
        buf->data = reallocate(buf->data, capacity * sizeof(uint8_t));
        buf->capacity = capacity;
    }
    buf->length += length;
}

static int
fprint_definition_operands(FILE *out, const Definition *def, Operands operands) {
    int operand_count = def->widths.length;
    if (operands.length != operand_count) {
        FPRINTF(out, "ERROR: operand len %d does not match defined %d\n",
            operands.length, operand_count);
        return 0;
    }

    switch (operand_count) {
        case 1:
            FPRINTF(out, "%s %d\n", def->name, operands.data[0]);
            return 0;
    }

    FPRINTF(out, "ERROR: unhandled operand_count for %s\n", def->name);
    return 0;
}

int fprint_instruction(FILE *out, Instructions ins) {
    int err, read, i = 0;
    const Definition *def;
    Operands operands;
    while (i < ins.length) {
        def = lookup(ins.data[i]);
        operands = read_operands(&read, def, ins.data + i);
        FPRINTF(out, "%04d ", i);
        err = fprint_definition_operands(out, def, operands);
        free(operands.data);
        if (err == -1) return -1;
        i += 1 + read;
    }
    return 0;
}
