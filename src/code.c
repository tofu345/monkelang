#include "code.h"
#include "utils.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define DEF(code, operands) { #code, _##operands }
#define DEF_EMPTY(code) { #code, { NULL, 0 } }

const int _two_widths[] = { 2 };
const Operands _two = { .widths = (int *)_two_widths, .length = 1 };

const int _one_widths[] = { 1 };
const Operands _one = { .widths = (int *)_one_widths, .length = 1 };

const int _closure_widths[] = { 2, 1 };
const Operands _closure = { .widths = (int *)_closure_widths, .length = 2 };

const Definition definitions[] = {
    DEF(OpConstant, two), // constant index
    DEF_EMPTY(OpPop),
    DEF_EMPTY(OpAdd),
    DEF_EMPTY(OpSub),
    DEF_EMPTY(OpMul),
    DEF_EMPTY(OpDiv),
    DEF_EMPTY(OpTrue),
    DEF_EMPTY(OpFalse),
    DEF_EMPTY(OpEqual),
    DEF_EMPTY(OpNotEqual),
    DEF_EMPTY(OpGreaterThan),
    DEF_EMPTY(OpMinus),
    DEF_EMPTY(OpBang),
    DEF(OpJumpNotTruthy, two), // instruction index
    DEF(OpJump, two),          // instruction index
    DEF_EMPTY(OpNull),
    DEF(OpGetGlobal, two),    // globals index
    DEF(OpSetGlobal, two),    // globals index
    DEF(OpArray, two), // num elements
    DEF(OpHash, two),  // num pairs
    DEF_EMPTY(OpIndex),
    DEF_EMPTY(OpSetIndex),
    DEF(OpCall, one), // num arguments
    DEF_EMPTY(OpReturnValue),
    DEF_EMPTY(OpReturn),
    DEF(OpGetLocal, one),    // locals index
    DEF(OpSetLocal, one),    // locals index
    DEF(OpGetBuiltin, one), // builtin fn index

    // constant index of Function and number of free variables
    DEF(OpClosure, closure),
    DEF(OpGetFree, one), // free variable index
    DEF_EMPTY(OpCurrentClosure),
};

const Definition *
lookup(Opcode op) {
    static int len = sizeof(definitions) / sizeof(definitions[0]);
    if (op <= 0 || (int)op > len) {
        die("Definition for Opcode '%d' not found", op);
    }
    return definitions + op - 1;
}

int read_big_endian_uint16(uint8_t *arr) {
    return (arr[0] << 8) | arr[1];
}

int read_big_endian_uint8(uint8_t *arr) {
    return arr[0];
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
        .widths = malloc(def->operands.length * sizeof(int)),
        .length = def->operands.length
    };
    if (operands.widths == NULL) { die("read_operands: malloc"); }

    int width, offset = 0;
    ins++; // skip Opcode
    for (int i = 0; i < def->operands.length; i++) {
        width = def->operands.widths[i];
        switch (width) {
            case 2:
                operands.widths[i] = read_big_endian_uint16(ins + offset);
                break;

            case 1:
                operands.widths[i] = read_big_endian_uint8(ins + offset);
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

int
make_valist_into(Instructions *ins, Opcode op, va_list operands) {
    const Definition *def = lookup(op);
    int start = ins->length;

    instructions_allocate(ins, 1);
    ins->data[start] = op;
    int operand, width, offset = start + 1;

    for (int i = 0; i < def->operands.length; i++) {
        operand = va_arg(operands, int);
        width = def->operands.widths[i];
        instructions_allocate(ins, width);
        switch (width) {
            case 2:
                put_big_endian_uint16(ins->data + offset, operand);
                break;

            case 1:
                ins->data[offset] = operand;
                break;

            default:
                die("make: operand width %d not implemented", width);
        }
        offset += width;
    }
    return start;
}

int
make_into(Instructions *ins, Opcode op, ...) {
    va_list ap;
    va_start(ap, op);
    int pos = make_valist_into(ins, op, ap);
    va_end(ap);
    return pos;
}

Instructions
make(Opcode op, ...) {
    va_list ap;
    va_start(ap, op);
    Instructions ins = {};
    make_valist_into(&ins, op, ap);
    va_end(ap);
    return ins;
}

void instructions_allocate(Instructions *buf, int length) {
    if (buf->length + length > buf->capacity) {
        int capacity = power_of_2_ceil(buf->length + length);
        buf->data = realloc(buf->data, capacity * sizeof(uint8_t));
        if (buf->data == NULL) { die("instructions_allocate"); }
        buf->capacity = capacity;
    }
    buf->length += length;
}

static int
fprint_definition_operands(FILE *out, const Definition *def, Operands operands) {
    int operand_count = def->operands.length;
    if (operands.length != operand_count) {
        FPRINTF(out, "ERROR: operand len %d does not match defined %d\n",
            operands.length, operand_count);
        return 0;
    }

    switch (operand_count) {
        case 0:
            FPRINTF(out, "%s\n", def->name);
            return 0;
        case 1:
            FPRINTF(out, "%s %d\n", def->name, operands.widths[0]);
            return 0;
        case 2:
            FPRINTF(out, "%s %d %d\n", def->name, operands.widths[0], operands.widths[1]);
            return 0;
        default:
            FPRINTF(out, "ERROR: unhandled operand_count for %s\n", def->name);
            return 0;
    }
}

int fprint_instructions(FILE *out, Instructions ins) {
    int err, read, i = 0;
    const Definition *def;
    Operands operands;
    while (i < ins.length) {
        def = lookup(ins.data[i]);
        operands = read_operands(&read, def, ins.data + i);
        FPRINTF(out, "%04d ", i);
        err = fprint_definition_operands(out, def, operands);
        free(operands.widths);
        if (err == -1) return -1;
        i += 1 + read;
    }
    return 0;
}
