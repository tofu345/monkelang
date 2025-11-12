#pragma once

// This modules contains definitions for the Instructions in the Bytecode.

#include "ast.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
    // OpConstant: push the constant at specified index in the constant buffer
    // onto the stack.
    OpConstant = 1,

    // OpPop: remove the value at the top of the stack.
    OpPop,

    // Pop two Objects off the stack, perform a binary operation and push the
    // result back onto on the stack.
    OpAdd,
    OpSub,
    OpMul,
    OpDiv,
    OpEqual,
    OpNotEqual,
    OpGreaterThan,

    // Pop an Object off the stack, perform an operation and push the result
    // onto the stack.
    OpMinus,
    OpBang,

    OpTrue, // Push a [true] Boolean Object.
    OpFalse, // Push a [false] Boolean Object.
    OpNull, // Push a Null Object.

    // OpJumpNotTruthy: pop an Object, and jump to specified position in the
    // current functions instructions if the Object [is_truthy], see vm.h.
    OpJumpNotTruthy,

    // OpJump: jump to specified position in the current functions
    // instructions.
    OpJump,

    // Get or set the Object at the specified index in the global variable list.
    OpGetGlobal,
    OpSetGlobal,

    // Get or set the Object at the specified index in the current function
    // local variables.
    OpGetLocal,
    OpSetLocal,

    // Get or set the Object at the specified index in the current function
    // free variable list.
    OpGetFree,
    OpSetFree,

    // OpArray: create an Array Object with specified number of elements on the
    // stack.
    OpArray,

    // OpTable: create a Table Object with specified number of elements on the
    // stack, stored in key-value pairs.
    OpTable,

    // OpIndex:
    //
    // Pop two Objects `A` and `B`.
    // - get the value of array `A` at index `B`, or
    // - get the value of table `A` at key `B`.
    OpIndex,

    // OpSetIndex:
    //
    // Pop three Objects `A`, `B` and `C`.
    //
    // - set the value of array `A` at index `B` to `C`, or
    //
    // - set the value of table `A` at key `B` to `C`.
    //   NOTE: if `C` is null the key `B` is removed from table `A`.
    OpSetIndex,

    // OpCall: call a Compiled or Builtin function with specified number of
    // arguments.
    //
    // The calling convention for a function is as follows: the Object
    // containing the function is pushed onto the stack, followed by its
    // arguments and an OpCall with `N` arguments.
    OpCall,

    // OpReturnValue: pop an Object of the top of stack, return to the previous
    // Frame and push the popped Object onto the stack.
    OpReturnValue,

    // OpReturn: return to the previous Frame and push a Null Object onto the
    // stack.
    OpReturn,

    // OpGetBuiltin: Push the Builtin Function with specified index onto the
    // stack.
    OpGetBuiltin,

    // OpClosure: Create a Closure with the Compiled Function at specified
    // index in the constants list with specified number of free variable on
    // the stack.
    OpClosure,

    // OpCurrentClosure: Push an Object containing the Closure of the current
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

// Contains information needed for mapping a Node in the AST to a position in a
// Functions bytecode.  It is used for printing which line of the source code
// an error occured in the VM.
typedef struct {
    // the position in bytes in a Functions Instructions.
    int position;

    // The node in the AST with a Token which points to the source code.
    Node node;
} SourceMapping;

BUFFER(SourceMapping, SourceMapping)

// Find [SourceMapping] [ip] occurs at, the source mapping with highest
// [position] that is less than [ip] with binary search.
SourceMapping *find_mapping(SourceMappingBuffer maps, int ip);

// fprint_instructions() with SourceMapping of each Instruction.
int fprint_instructions_mappings(FILE *out, SourceMappingBuffer mappings,
                                 Instructions ins);
