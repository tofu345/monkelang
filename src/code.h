#pragma once

// This modules contains definitions for the Instructions in the Bytecode.

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
    // `OpConstant N`: push the constant at index `N` in the constant buffer
    // onto the stack.
    OpConstant = 1,

    // `OpPop`: remove the value at the top of the stack.
    OpPop,

    // These Opcodes pop two Objects off the stack, perform a binary operation
    // on them and push the result back onto on the stack.
    OpAdd,
    OpSub,
    OpMul,
    OpDiv,
    OpEqual,
    OpNotEqual,
    OpGreaterThan,

    // These Opcodes pop an Object off the stack, perform an operation and then
    // push the result onto the stack.
    OpMinus,
    OpBang,

    OpTrue, // Push a [true] Boolean Object.
    OpFalse, // Push a [false] Boolean Object.
    OpNull, // Push a Null Object.

    // `OpJumpNotTruthy N`: pop an Object, and jump to position `N` in the
    // current functions instructions if the Object [is_truthy], see vm.h.
    OpJumpNotTruthy,

    // `OpJump N`: jump to position `N` in the current functions instructions.
    OpJump,

    // Get or set the Object in the global variable list.
    OpGetGlobal,
    OpSetGlobal,

    // Get or set the Object in the current function local variables.
    OpGetLocal,
    OpSetLocal,

    // Get or set the Object in the current function free variable list.
    OpGetFree,
    OpSetFree,

    // `OpArray N`: create an Array Object with `N` elements on the stack.
    OpArray,

    // `OpTable N`: create a Table Object with `N` elements on the stack,
    // stored in key-value pairs.
    OpTable,

    // `OpIndex` Pop two Objects `A` and `B`. Get:
    //
    // - the value of array `A` at index `B`, or
    // - the value of table `A` at key `B`.
    OpIndex,

    // `OpSetIndex` Pop three Objects `A`, `B` and `C`. Set:
    //
    // - the value of array `A` at index `B` to `C`, or
    // - the value of table `A` at key `B` to `C`.
    //   NOTE: if `C` is null the key `B` is removed from the table.
    OpSetIndex,

    // `OpCall N`: call a Compiled or Builtin function with `N` arguments.
    //
    // The calling convention for a function is as follows: the Object
    // containing the function is pushed onto the stack, followed by its
    // arguments and an OpCall with `N` arguments.
    OpCall,

    // `OpReturnValue`: pop an Object of the top of stack, return to the
    // previous Frame and push the popped Object onto the stack.
    OpReturnValue,

    // `OpReturn`: return to the previous Frame and push a Null Object onto the
    // stack.
    OpReturn,

    // `OpGetBuiltin N`: Push the Builtin Function with index `N` onto the
    // stack.
    OpGetBuiltin,

    // `OpClosure M N`: Create a Closure with the Compiled Function at index
    // `M` in the constants list with `N` free variable on the stack.
    OpClosure,

    // `OpCurrentClosure`: Push an Object containing the Closure of the current
    // Frame.
    OpCurrentClosure,
} Opcode;

// Operands of Opcodes.
typedef struct {
    // Widths in bytes of each operand, operands are stored in big endian.
    int *widths;

    // number of operands.
    int length;
} Operands;

// Name and Operands of all Opcodes.
typedef struct {
    const char *name;
    const Operands operands;
} Definition;

// Return Definition for Opcode
const Definition *lookup(Opcode op);

typedef struct {
    uint8_t* data;
    int length, capacity;
} Instructions;

// allocate [length] extra elements and increment [buf.length] by [length].
void instructions_allocate(Instructions *buf, int length);

int fprint_instructions(FILE *out, Instructions ins);

// Read integer operands from `ins[1:]` with Operand widths according in [def].
Operands read_operands(int *n, const Definition *def, uint8_t *ins);

// read `arr[0:2]` big endian.
int read_big_endian_uint16(uint8_t *arr);

// read `arr[0]`
int read_big_endian_uint8(uint8_t *arr);

// Make Instructions from Opcode and [int] operands.
Instructions make(Opcode op, ...);
int make_into(Instructions *ins, Opcode op, ...);
int make_valist_into(Instructions *ins, Opcode op, va_list operands);
